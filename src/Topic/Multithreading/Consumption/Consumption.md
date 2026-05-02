# Consumption

游戏场景:boss 被多个 attacker 并行攻击。围绕"消耗 HP"演示多线程读改写、原子操作、CAS 与互斥锁的取舍。

建议阅读顺序:

| # | 文件 | 概念 | 一句话 |
|---|------|------|--------|
| 1 | [Race.cpp](Race.cpp) | race condition | raw `int` 扣血,总伤害 < 攻击次数 |
| 2 | [Atomic.cpp](Atomic.cpp) | `std::atomic` + `fetch_sub` | 单变量计数,伤害守恒 |
| 3 | [LastHit.cpp](LastHit.cpp) | `compare_exchange_weak` (CAS) | 多攻击者抢"击杀者"归属 |
| 4 | [OverKill.cpp](OverKill.cpp) | `std::mutex` + 不变式 | `bAlive == (HP > 0)`,`OnDeath` 仅触发一次 |

主线:从无同步 → 单变量原子 → 多步原子 (CAS) → 多变量不变式 (mutex)。
