#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

// 餐厅取餐器场景:一个顾客等一个厨师做菜。
//
// 三件套:
//   - bFoodReady     共享状态,真相
//   - Mutex          保护共享状态读写
//   - CV             顾客睡觉等待时的取餐器
//
// 顾客 wait(Lock, Pred):
//   1. 进 wait 时原子地放下锁 + 挂起,避免 lost wakeup
//   2. 被 notify_one 唤醒后,自动重新拿锁,检查 Pred,
//      Pred 仍假就继续睡(防 spurious wakeup)
//
// 厨师 notify_one 之前必须先拿同一把锁改 bFoodReady,
// 这把"先持锁再 notify"的姿势是 CV 模式的硬性要求,
// 不然顾客可能错过通知。

int main()
{
    std::mutex Mutex;
    std::condition_variable CV;
    bool bFoodReady = false;

    // 顾客线程:点完单后睡等取餐
    std::thread Customer([&]
    {
        std::cout << "[Customer] 下单完成,坐下等取餐\n";

        std::unique_lock<std::mutex> Lock(Mutex);
        CV.wait(Lock, [&] { return bFoodReady; });
        // 到这一行时:bFoodReady == true,Lock 仍持有

        std::cout << "[Customer] 取餐器响了,过去拿菜\n";
    });

    // 厨师线程:做菜需要时间,做完按取餐器
    std::thread Chef([&]
    {
        std::cout << "[Chef] 开始做菜...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        {
            std::lock_guard<std::mutex> Lock(Mutex);
            bFoodReady = true;
            std::cout << "[Chef] 菜做好,按下取餐器\n";
        }
        // 注意:notify_one 放在锁外面也合法,且通常更优,
        // 因为先放锁让顾客醒来时能立刻拿到。但放在锁内更直观,
        // 学习阶段先用清晰的版本。这里把 notify_one 放在锁外:
        CV.notify_one();
    });

    Customer.join();
    Chef.join();

    std::cout << "[Main] 顾客离开,餐厅打烊\n";
    return 0;
}
