#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// 用 CAS 抢"击杀者"归属。多个 attacker 争最后一击,
// 只有把 HP 从 1 扣到 0 的那一次才记为击杀。
//
// 套路(和 Test_05 / 06 同模板):
//   1. load 一次 Current(放在 loop 外)
//   2. while (Current > 0):算 Next → CAS → 成功就处理,失败 CAS 已自动刷新 Current
//   3. CAS 成功且击杀(Next == 0)→ publish Killer 后返回
//
// 关键:初始 load 放 loop 外。失败时 CAS 的 failure-acquire 已经把 Current
//      刷成最新值,loop 顶部不需要再 load 一次。

int main()
{
    constexpr int AttackerCount = 4;
    constexpr int BossMaxHP = 1000;

    std::atomic<int> BossHP{BossMaxHP};
    std::atomic<int> Killer{-1};
    std::atomic<int> Hits{0};

    auto AttackerFn = [&](int Id)
    {
        int Current = BossHP.load(std::memory_order_acquire);

        while (Current > 0)
        {
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
            // CAS 失败:Current 已被刷成最新值,while 自己重判 Current > 0
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
