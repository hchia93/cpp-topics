#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// 单靠 atomic 无法维护跨变量的不变式。假设 HP 是 atomic,另外
// 还有 bAlive 标记和 OnDeath 回调:两个线程各自看到 HP == 1,
// 都调 fetch_sub,都看到 Before == 1,都触发 OnDeath——boss 死了两次。
//
// 修法:把"判活 + 扣血 + 改 bAlive + 触发 OnDeath"这串复合操作
// 全部放进同一把 mutex 里,保证只有一个线程能观察到从生到死的
// 那次跃迁。不变式 bAlive == (HP > 0) 在所有可观察点都成立,
// 因为 HP 和 bAlive 只会在锁内一起被改。

int main()
{
    constexpr int AttackerCount = 8;
    constexpr int BossMaxHP = 1000;

    std::mutex Mutex;
    int BossHP = BossMaxHP;
    bool bAlive = true;
    int OnDeathFired = 0;
    int TotalHits = 0;

    auto AttackerFn = [&]
    {
        while (true)
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            if (!bAlive)
            {
                return;
            }

            BossHP -= 1;
            ++TotalHits;
            if (BossHP <= 0)
            {
                bAlive = false;
                ++OnDeathFired;  // 锁保证这里恰好执行一次
                return;
            }
        }
    };

    std::vector<std::thread> Attackers;
    Attackers.reserve(AttackerCount);
    for (int i = 0; i < AttackerCount; ++i)
    {
        Attackers.emplace_back(AttackerFn);
    }
    for (auto& T : Attackers)
    {
        T.join();
    }

    std::cout << "Total hits:    " << TotalHits
              << " (expected " << BossMaxHP << ")\n";
    std::cout << "OnDeath fired: " << OnDeathFired << " (expected 1)\n";
    std::cout << "Final HP:      " << BossHP << "\n";
    return 0;
}
