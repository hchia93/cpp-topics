#include <iostream>
#include <thread>
#include <vector>

// 场景:boss 被 N 个 attacker 并行攻击,每次扣 1 HP。
// 表达式 `BossHP -= 1` 会编译成 load-modify-store 三步,跨核非原子:
// 两个线程同时 load 到同一个值,各自减 1,再写回,
// 其中一次扣减就被悄悄覆盖丢失。
//
// 多跑几次,实际伤害会小于攻击次数,且每次运行结果会抖动。

int main()
{
    constexpr int AttackerCount = 8;
    constexpr int HitsPerAttacker = 1000000;

    int BossHP = AttackerCount * HitsPerAttacker;
    const int StartHP = BossHP;

    auto AttackerFn = [&]
    {
        for (int i = 0; i < HitsPerAttacker; ++i)
        {
            BossHP -= 1;
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

    const int Expected = AttackerCount * HitsPerAttacker;
    const int Actual = StartHP - BossHP;

    std::cout << "Expected damage: " << Expected << "\n";
    std::cout << "Actual damage:   " << Actual << "\n";
    std::cout << "Lost hits:       " << (Expected - Actual) << "\n";
    return 0;
}
