#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// 场景:玩家点"查询排行榜",服务器响应慢,
// client 不能死等,超过 500ms 就放弃,UI 显示"加载失败,请重试"。
//
// 三种等待 API:
//   - wait(Lock, Pred)                    无限等
//   - wait_for(Lock, Timeout, Pred)       等相对时长
//   - wait_until(Lock, Deadline, Pred)    等绝对时间点
//
// 带 Pred 版本的返回值:
//   - true  = predicate 真(没超时)
//   - false = 超时
//
// 实战:
//   - 网络请求一律 wait_for,死等会让整个流程僵住
//   - UI 反馈、输入超时也常用,避免卡帧
//   - 写"总预算 N 秒重试 K 次"时,算 deadline 后用 wait_until 更自然

int main()
{
    std::mutex Mutex;
    std::condition_variable CV;
    bool bResponseReady = false;
    std::string Response;

    // 慢服务器:故意花 800ms,超过 client 的 500ms 上限
    std::thread Server([&]
    {
        std::cout << "[Server] 收到请求,处理中...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        {
            std::lock_guard<std::mutex> Lock(Mutex);
            Response = "Top1: PlayerA";
            bResponseReady = true;
            std::cout << "[Server] 处理完成,notify\n";
        }
        CV.notify_one();
    });

    // Client:等 500ms 拿不到就放弃
    std::thread Client([&]
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        std::cout << "[Client] 等响应,上限 500ms\n";

        bool bGot = CV.wait_for(Lock,
                                std::chrono::milliseconds(500),
                                [&] { return bResponseReady; });

        if (bGot)
        {
            std::cout << "[Client] 收到:" << Response << "\n";
        }
        else
        {
            std::cout << "[Client] 超时,放弃。UI 显示'加载失败,请重试'\n";
        }
    });

    Client.join();
    // Server 还没跑完,join 它清理资源(此时它会写 Response,但已无人读)
    Server.join();

    return 0;
}
