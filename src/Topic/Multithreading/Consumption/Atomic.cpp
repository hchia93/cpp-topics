#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// 和 Race.cpp 同样的场景,把 HP 换成 std::atomic<int>。
// fetch_sub 是一次原子读改写,所有核都能看到每一次扣减,
// 每次运行总伤害都精确等于攻击次数。
//
// 这里用 memory_order_relaxed 就够了:我们只关心计数正确,
// 不需要它和其它变量之间的顺序关系。

int main()
{
    constexpr int AttackerCount = 8;
    constexpr int HitsPerAttacker = 1000000;

    std::atomic<int> BossHP{AttackerCount * HitsPerAttacker};
    const int StartHP = BossHP.load();

    auto AttackerFn = [&]
    {
        for (int i = 0; i < HitsPerAttacker; ++i)
        {
            BossHP.fetch_sub(1, std::memory_order_consume);
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

    const int Damage = StartHP - BossHP.load();
    std::cout << "Damage dealt: " << Damage
              << " (expected " << AttackerCount * HitsPerAttacker << ")\n";
    return 0;
}
