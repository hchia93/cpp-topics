// 练习题 10:一次性事件标志(One-Shot Flag)
//
// 目的:多 acquirer 共同 spin 等单一 producer 的发布
// 难度:中等(15-25 分钟)
//
// 这是 Test_09 的镜像:
//   - Test_09:多个 release(N 个 worker CountDown) + 1 个 acquire(主线程 Wait)
//   - 本题: 1 个 release(producer Set) + 多个 acquire(N 个 consumer Wait)
//
// 也是 Test_03 的扩写版:Test_03 是 1 producer + 1 consumer 裸 load,
// 本题封装成 TryWait/Wait API,且多个 consumer 共享一次 Set 的发布。

// =====================================================================
// 题目 1:实现 TOneShotFlag
// =====================================================================
//
// 场景:
//   producer 准备好一份数据,然后 Set() 标志位发布。
//   N 个 consumer 各自 Wait() 等标志位被 Set,然后读那份数据。
//   类比:店门口的 OPEN 牌子,翻过来排队的人才能进。
//   "一次性"的意思:Set 之后不会再变回 false(单调)。
//
// API:
//   - Set():发布(单调:从 false 到 true,只调一次)
//   - TryWait():单次检查
//   - Wait():spin 直到 TryWait 为 true
//
// 提示:
//   - Set:store(true, release)
//   - TryWait:load(acquire)
//   - Wait:while (!TryWait()) yield()
//
//   memory_order 配对:
//     Set(release) ←→ TryWait/Wait(acquire)
//     1 个 release 同时配 N 个 acquire——所有 consumer 都能看到 producer 的写。
//     这就是"广播式发布"。

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class TOneShotFlag
{
public:
    void Set()
    {
        bRaised.store(true, std::memory_order_release);
    }

    bool TryWait() const
    {
        return bRaised.load(std::memory_order_acquire) == true;
    }

    void Wait() const
    {
        while(!TryWait())
        {
            std::this_thread::yield();
        }
    }

private:
    std::atomic<bool> bRaised{false};
};

// =====================================================================
// 题目 2:写测试主体
// =====================================================================
//
// N 个 consumer,每个:
//   1. Flag.Wait()
//   2. Observed[i] = Payload     ← 读 producer 写的数据
//
// 主线程(扮演 producer):
//   3. 先睡一小段时间,让 consumer 真的进入 spin(不是必需,只是为了让 race
//      暴露得更明显——没这步的话 Set 可能在 consumer 还没起来前就发布了)
//   4. Payload = 42              ← producer 写数据
//   5. Flag.Set()                ← 发布
//
// 注意顺序:**producer 必须先写 Payload,再 Set**。
// 反了的话 release 屏障保护不到 Payload 的那次写,consumer 可能读到旧值。
//
// Payload 是普通 int 不是 atomic,靠 release/acquire 配对保护。

int main()
{
    std::cout << "--- Test 10: TOneShotFlag ---\n";

    constexpr int Consumers = 8;
    TOneShotFlag Flag;
    int Payload = 0;
    std::vector<int> Observed(Consumers, -1);

    std::vector<std::thread> Threads;
    for (int i = 0; i < Consumers; ++i)
    {
        Threads.emplace_back([&, i]
        {
            Flag.Wait();
            Observed[i] = Payload;
        });
    }

    // Producer 的行为： 拖 -> 设置 -> 公布
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Payload = 42;
    Flag.Set();

    for (auto& T : Threads) T.join();

    for (int i = 0; i < Consumers; ++i)
    {
        std::cout << "Observed[" << i << "] = " << Observed[i] << "\n";
        assert(Observed[i] == 42);
    }

    std::cout << "PASSED\n";
    return 0;
}
