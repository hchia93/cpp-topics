#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// 用 CAS 抢"击杀者"归属。多个 attacker 争最后一击,
// 只有把 HP 从 1 扣到 0 的那一次才记为击杀。
//
// 套路:读当前 HP → 算出新 HP → 仅当 HP 没被别人改过时才提交。
// compare_exchange_weak 在竞争失败时会顺便把 Current 重新加载
// 为最新值,所以失败分支什么都不写,loop 自然重试。

int main()
{
    constexpr int AttackerCount = 4;
    constexpr int BossMaxHP = 1000;

    std::atomic<int> BossHP{BossMaxHP};
    std::atomic<int> Killer{-1};
    std::atomic<int> Hits{0};

    auto AttackerFn = [&](int Id)
    {
        while (true)
        {
            int Current = BossHP.load(std::memory_order_acquire);
            if (Current <= 0)
            {
                return;
            }

            const int Next = Current - 1;
            if (BossHP.compare_exchange_weak(
                    Current, Next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
            {
                Hits.fetch_add(1, std::memory_order_relaxed);
                if (Next == 0)
                {
                    Killer.store(Id, std::memory_order_release);
                    return;
                }
            }
            // 失败:CAS 已经把 Current 更新为最新值,直接重试
        }
    };

    std::vector<std::thread> Attackers;
    Attackers.reserve(AttackerCount);
    for (int i = 0; i < AttackerCount; ++i)
    {
        Attackers.emplace_back(AttackerFn, i);
    }
    for (auto& T : Attackers)
    {
        T.join();
    }

    std::cout << "Killer: attacker " << Killer.load() << "\n";
    std::cout << "Total hits: " << Hits.load()
              << " (expected " << BossMaxHP << ")\n";
    return 0;
}
