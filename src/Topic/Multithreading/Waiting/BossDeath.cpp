#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// 场景:boss 死亡后,多个 looter 一起冲进来分赃。
//
// 重点:notify_one vs notify_all。
//   - notify_one  只唤醒一个等待者(顾客取餐用这个,Order.cpp)
//   - notify_all  唤醒所有等待者(boss 死了所有 looter 一起冲)
//
// 用错就 bug:N 个 looter 都在 wait,只 notify_one 的话,
// 剩下 N-1 个永远等不到下一次通知,卡死。
//
// 何时用哪个:
//   - 同一事件触发多人继续              → notify_all
//   - 信号代表"有一份资源"(典型生产消费) → notify_one
//   - 不确定时,notify_all 不会错,顶多多醒几个

int main()
{
    std::mutex Mutex;
    std::condition_variable CV;
    bool bBossDead = false;
    std::atomic<int> Loots{0};

    constexpr int LooterCount = 5;

    std::vector<std::thread> Looters;
    for (int i = 0; i < LooterCount; ++i)
    {
        Looters.emplace_back([&, i]
        {
            std::unique_lock<std::mutex> Lock(Mutex);
            CV.wait(Lock, [&] { return bBossDead; });
            // 醒来时:bBossDead == true,Lock 仍持有

            // 把抢战利品挪出锁,临界区只用来判 boss 是否死
            Lock.unlock();

            int MyLoot = Loots.fetch_add(1) + 1;
            std::cout << "[Looter " << i << "] 拿到战利品 #" << MyLoot << "\n";
        });
    }

    // 攻击者线程:一段时间后击杀 boss
    std::thread Killer([&]
    {
        std::cout << "[Killer] 开打...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        {
            std::lock_guard<std::mutex> Lock(Mutex);
            bBossDead = true;
            std::cout << "[Killer] boss 倒地\n";
        }
        // 关键:notify_all,不是 notify_one。
        // 把这行换成 CV.notify_one() 跑跑看,只有 1 个 Looter 拿到战利品,
        // 其他人卡在 wait,程序死锁(join 永远不返回)。
        CV.notify_all();
    });

    Killer.join();
    for (auto& T : Looters) T.join();

    std::cout << "[Main] 总计分赃 " << Loots.load()
              << " 份(应为 " << LooterCount << ")\n";
    return 0;
}
