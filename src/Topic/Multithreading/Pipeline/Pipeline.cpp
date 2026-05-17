#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

// 三段流水线:Producer -> Worker -> Sink。每段一种线程,段间用带关闭语义的
// 有界队列连接。这是第一个"3+ 角色协调"的样本,前面都是 1:1 或 1:N。
//
// 本样本的核心是关闭协议 + join 拓扑序:
//   join Producer -> Q1.Close() -> join Worker -> Q2.Close() -> join Sink
//
// 这个顺序是关闭依赖链的拓扑排序,不是随便排的。把任意一步提前都可能死锁:
// 例如先 join Sink,Sink 卡在 Q2.Pop() 等数据或关闭信号,但 Q2 永远不会被
// Close(因为 Worker 没结束,因为 Q1 没 Close,因为 Producer 还没 join),
// 整条链循环等待。
//
// 判据:绝不 join 一个正阻塞的消费者,除非它的唤醒条件(队列关闭)已经安排好。

// 段间队列:Push 投递,Pop 取一个,Close 宣告 end-of-stream。
// 队列空且已 Close 时 Pop 返回 nullopt,消费者据此优雅退出。
template<typename T>
class TPipeQueue
{
public:
    void Push(T Value)
    {
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            Q.push(std::move(Value));
        }
        NotEmpty.notify_one();
    }

    std::optional<T> Pop()
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        NotEmpty.wait(Lock, [&] { return !Q.empty() || bClosed; });

        if (Q.empty())
        {
            return std::nullopt;   // 已关闭且排空,告诉消费者收工
        }

        T Value = std::move(Q.front());
        Q.pop();
        return Value;
    }

    void Close()
    {
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            bClosed = true;
        }
        NotEmpty.notify_all();   // 唤醒所有卡在空队列上的消费者,让它们看到 bClosed
    }

private:
    std::queue<T> Q;
    std::mutex Mutex;
    std::condition_variable NotEmpty;
    bool bClosed = false;
};

int main()
{
    TPipeQueue<int> Q1;   // Producer -> Worker
    TPipeQueue<int> Q2;   // Worker   -> Sink

    constexpr int ItemCount = 8;

    // Stage 1:产出 1..ItemCount
    std::thread Producer([&]
    {
        for (int i = 1; i <= ItemCount; ++i)
        {
            std::cout << "[Producer] 产出 " << i << "\n";
            Q1.Push(i);
        }
    });

    // Stage 2:平方后转交下一段
    std::thread Worker([&]
    {
        while (std::optional<int> In = Q1.Pop())
        {
            int Out = *In * *In;
            std::cout << "[Worker]   " << *In << " -> " << Out << "\n";
            Q2.Push(Out);
        }
        std::cout << "[Worker]   Q1 end-of-stream,退出\n";
    });

    // Stage 3:汇总
    int Sum = 0;
    std::thread Sink([&]
    {
        while (std::optional<int> In = Q2.Pop())
        {
            Sum += *In;
            std::cout << "[Sink]     +" << *In << ",running sum = " << Sum << "\n";
        }
        std::cout << "[Sink]     Q2 end-of-stream,退出\n";
    });

    // 关闭顺序 = 拓扑序,从源头流向汇点
    Producer.join();   // 1. 等生产者产完所有输入
    Q1.Close();        // 2. 告诉 Worker:没有更多输入了
    Worker.join();     // 3. Worker drain Q1、处理完最后一批才退出
    Q2.Close();        // 4. 告诉 Sink:没有更多结果了
    Sink.join();       // 5. Sink drain Q2、汇总完才退出

    std::cout << "[Main] 最终 Sum = " << Sum
              << "(1^2 + 2^2 + ... + " << ItemCount << "^2,应为 204)\n";
    return 0;
}
