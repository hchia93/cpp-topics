#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

// 场景:为什么 wait 必须带 predicate。
//
// 两个常见坑:
//   1. lost wakeup(本文件强制重现)
//      notify 在 wait 之前就到了,无 predicate 的 wait 等不到下一次通知,死锁。
//   2. spurious wakeup(理论存在,平台依赖,这里不强求重现)
//      wait 没人 notify 也可能返回。无 predicate 就误以为条件成立。
//
// wait(Lock, Pred) 帮你做的两件事:
//   - 进 wait 前先看一眼 Pred,真就直接返回 → 救 lost wakeup
//   - 醒来后再看一眼 Pred,假就继续睡       → 救 spurious wakeup
//
// 结论:不带 Pred 的裸 wait(Lock) 永远不要写。

int main()
{
    std::cout << "--- 错误版:notifier 抢跑,waiter 死等 ---\n";
    {
        std::mutex Mutex;
        std::condition_variable CV;
        bool bFlag = false;

        // notifier 先跑完:改 flag、notify、退出
        std::thread Notifier([&]
        {
            {
                std::lock_guard<std::mutex> Lock(Mutex);
                bFlag = true;
                std::cout << "[Notifier] flag 改成 true,notify_one\n";
            }
            CV.notify_one();
        });
        Notifier.join();
        // 此刻 notify 已发出去,但还没人在 wait,信号丢了

        // waiter 后到。裸 wait(Lock) 不看 flag,直接挂起,
        // 已经丢的信号不会再来一次,会死等。
        // 这里用 wait_for 给 500ms 超时,只为 demo 不卡住。
        std::thread Waiter([&]
        {
            std::unique_lock<std::mutex> Lock(Mutex);
            std::cout << "[Waiter] flag 当前已是 " << bFlag
                      << ",但裸 wait 不看,直接挂起等下一次 notify\n";

            std::cv_status Status =
                CV.wait_for(Lock, std::chrono::milliseconds(500));

            std::cout << "[Waiter] 结果:"
                      << (Status == std::cv_status::timeout
                              ? "超时(信号已丢)"
                              : "被叫醒")
                      << "\n";
        });
        Waiter.join();
    }

    std::cout << "\n--- 正确版:带 predicate,先看一眼 ---\n";
    {
        std::mutex Mutex;
        std::condition_variable CV;
        bool bFlag = false;

        std::thread Notifier([&]
        {
            {
                std::lock_guard<std::mutex> Lock(Mutex);
                bFlag = true;
                std::cout << "[Notifier] flag 改成 true,notify_one\n";
            }
            CV.notify_one();
        });
        Notifier.join();

        std::thread Waiter([&]
        {
            std::unique_lock<std::mutex> Lock(Mutex);
            // 带 predicate:进 wait 前先看 bFlag,真就立刻返回,没挂起
            CV.wait(Lock, [&] { return bFlag; });
            std::cout << "[Waiter] 进 wait 前看到 flag = " << bFlag
                      << ",立刻返回,没死等\n";
        });
        Waiter.join();
    }

    return 0;
}
