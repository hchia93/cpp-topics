// 练习题 09:倒计时门闩(CountdownLatch)
//
// 目的:打 spin-Wait 模式 + acquire/release 经典配对(no CAS)
// 难度:中等(20-30 分钟)
//
// 这是 Test_08 的"无 CAS 版":
//   - Test_08:TryAcquire 用 CAS 条件递减(Old > 0 才 -1)
//   - 本题:CountDown 用 fetch_sub(1),不需要"满了拒绝"的条件分支
//           Wait 这一侧才是这道题的重点——多个 release 配单个 acquire,
//           主线程 acquire 时要保证看见所有 worker 的写
//
// 你说"Acquire 类还生疏"——这道题就是单纯的 spin Acquire 练习,
// 比 Test_08 干净,看得更清楚。

// =====================================================================
// 题目 1:实现 TCountdownLatch
// =====================================================================
//
// 场景:
//   主线程派 N 个 worker 各干一段独立的活。"全部完成"通过倒计时表达:
//   Counter 从 N 一路减到 0,主线程 Wait 阻塞到 0 才返回。
//   类比:发射倒计时,N 个准备工作各自完成时各 -1,到 0 才点火。
//
// API:
//   - CountDown():把计数 -1。每个 worker 完成自己那段活后调一次
//   - TryWait():单次检查 Counter 是否 ≤ 0。返回 true/false
//   - Wait():spin 等,直到 TryWait 为 true
//
// 提示:
//   - CountDown:fetch_sub(1, release)
//                ↑ release 让 worker 完成时写入的结果对 Wait 一侧可见
//   - TryWait:load(acquire) <= 0
//   - Wait:while (!TryWait()) yield()
//
//   memory_order 配对:
//     CountDown(release) ←→ TryWait/Wait(acquire)
//     和 Test_03 是同一个 publish-subscribe 模板,只是这里 publish
//     被压缩到一个倒计时器上,N 个 release 配 1 个 acquire。
//     重点:Wait 返回时,所有 worker 在 CountDown 之前的写都必须可见。

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

class TCountdownLatch
{
public:
    explicit TCountdownLatch(int InitialCount) : Counter(InitialCount) {}

    void CountDown()
    {
        // TODO:fetch_sub(1, release)
    }

    bool TryWait() const
    {
        // TODO:load(acquire) <= 0
        return false;
    }

    void Wait() const
    {
        // TODO:while (!TryWait()) yield()
    }

private:
    std::atomic<int> Counter;
};

// =====================================================================
// 题目 2:写测试主体
// =====================================================================
//
// N 个 worker,每个 worker(只跑一次,不是循环):
//   1. 计算自己的结果,写进 Results[i](用 i*i 当假数据)
//   2. Latch.CountDown()
//
// 主线程:
//   3. Latch.Wait()
//   4. 验证每个 Results[i] 都已经是预期值
//      (acquire 配对 release,Wait 返回时所有 worker 的写都可见)
//
// 注意:
//   - Results[i] 是普通 int 数组,不是 atomic。靠 release/acquire 保护
//   - 顺序很关键:worker 必须**先写 Results[i],再 CountDown**
//     反了的话 release 屏障保护不到这次写

int main()
{
    std::cout << "--- Test 09: TCountdownLatch ---\n";

    constexpr int Workers = 8;
    TCountdownLatch Latch(Workers);

    std::vector<int> Results(Workers, -1);

    std::vector<std::thread> Threads;
    for (int i = 0; i < Workers; ++i)
    {
        Threads.emplace_back([&, i]
        {
            // TODO:写一个 worker:
            //   1. Results[i] = i * i
            //   2. Latch.CountDown()
        });
    }

    Latch.Wait();

    // Wait 返回后,所有 worker 的写都应该可见
    for (int i = 0; i < Workers; ++i)
    {
        std::cout << "Results[" << i << "] = " << Results[i] << "\n";
        assert(Results[i] == i * i);
    }

    for (auto& T : Threads) T.join();

    std::cout << "PASSED\n";
    return 0;
}
