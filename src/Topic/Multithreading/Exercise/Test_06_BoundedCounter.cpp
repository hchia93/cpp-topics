// 练习题 06:有上限的计数器
//
// 目的:CAS 循环里**带条件退出**——这是新变种
// 难度:中等(15-20 分钟)
//
// 这是 Test_02 / Test_05 的进阶:不再是无脑 max/min,
// 而是"满了就拒绝"的真实业务逻辑。CAS 循环里加了一个
// 条件分支,需要正确处理"已满"和"未满"两个分支的语义。

// =====================================================================
// 题目
// =====================================================================
//
// 场景:
//   对象池有 MaxValue 个槽位,多线程并发申请。
//   - 申请成功:Counter +1,返回 true
//   - 池已满:**不修改任何状态**,返回 false
//
// 实现 TBoundedCounter:
//   - TryIncrement():尝试 +1。返回是否成功
//   - Get():返回当前值
//
// 关键:Counter 永远不能超过 MaxValue,总成功次数恰好等于 MaxValue,
// 总失败次数 = 总尝试 - MaxValue。
//
// 提示(套路和 Test_02 不一样,注意条件分支位置):
//   1. load 当前值 Old
//   2. **如果 Old >= MaxValue,直接 return false**(不进入 CAS)
//   3. 否则尝试 CAS(Old → Old + 1)
//   4. 成功:return true
//   5. 失败:Old 已被刷新成最新值,**回到第 2 步重新检查**
//      (注意:重试时 Old 可能已经达到 MaxValue,所以一定要重判)

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

class TBoundedCounter
{
public:
    static constexpr int MaxValue = 100;

    bool TryIncrement()
    {
        // TODO:CAS 循环 + 条件退出
        return false;   // 占位:替换掉
    }

    int Get() const
    {
        // TODO
        return 0;
    }

private:
    std::atomic<int> Value{0};
};

int main()
{
    std::cout << "--- Test 06: TBoundedCounter ---\n";

    TBoundedCounter Counter;
    std::atomic<int> SuccessCount{0};
    std::atomic<int> FailureCount{0};

    constexpr int Threads = 8;
    constexpr int IterPerThread = 50;   // 总尝试 400 次,只能成功 100 次

    std::vector<std::thread> Workers;
    for (int i = 0; i < Threads; ++i)
    {
        Workers.emplace_back([&]
        {
            for (int j = 0; j < IterPerThread; ++j)
            {
                if (Counter.TryIncrement())
                {
                    SuccessCount.fetch_add(1, std::memory_order_relaxed);
                }
                else
                {
                    FailureCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& T : Workers) T.join();

    const int FinalCounter = Counter.Get();
    const int Success = SuccessCount.load();
    const int Failure = FailureCount.load();

    std::cout << "Final counter: " << FinalCounter
              << " (expected " << TBoundedCounter::MaxValue << ")\n";
    std::cout << "Successes:     " << Success << "\n";
    std::cout << "Failures:      " << Failure << "\n";

    assert(FinalCounter == TBoundedCounter::MaxValue);
    assert(Success == TBoundedCounter::MaxValue);
    assert(Failure == Threads * IterPerThread - TBoundedCounter::MaxValue);

    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检
// =====================================================================
//
// bool TryIncrement()
// {
//     int Old = Value.load(std::memory_order_relaxed);
//     while (true)
//     {
//         if (Old >= MaxValue) return false;          // ← 条件退出
//
//         if (Value.compare_exchange_weak(
//                 Old, Old + 1,
//                 std::memory_order_relaxed,
//                 std::memory_order_relaxed))
//         {
//             return true;
//         }
//         // CAS 失败,Old 已被刷新成最新值,loop 重判 Old < MaxValue
//     }
// }
//
// =====================================================================
// 关键点:为什么"条件退出"必须放在 loop 里
// =====================================================================
//
// 错误版本:
//
//   bool TryIncrement()
//   {
//       int Old = Value.load();
//       if (Old >= MaxValue) return false;       // ← 只在外面查一次
//       while (!Value.compare_exchange_weak(Old, Old + 1, ...))
//       {
//           // 这里 Old 已经被 CAS 刷新,可能已经达到 MaxValue,但没重判
//       }
//       return true;
//   }
//
// 这个版本会**越界写入**:
//   - 你 load 时 Old = 99
//   - 进入 CAS 循环时,别人已经把 Value 推到 100
//   - 你的 CAS 失败,Old 被刷新为 100
//   - loop 继续,你以为还能 +1,CAS(100 → 101)
//   - 越过 MaxValue,Counter 变 101,assertion 挂
//
// 正解:把"是否超过 MaxValue"的检查放进 loop 内,**每次 CAS 失败后都重判**。
//
// =====================================================================
// 这道题练到的新模式
// =====================================================================
//
// CAS 循环的两种形态:
//
//   形态 A:无条件更新(Test_02 / Test_05)
//      do {
//          New = f(Old);
//      } while (!CAS);
//
//   形态 B:有条件更新(本题)
//      while (true) {
//          if (条件不满足) return failure;
//          New = f(Old);
//          if (CAS(Old, New)) return success;
//      }
//
// 形态 B 比 A 多一步**循环内的条件检查**,这是大量 lock-free
// 算法的核心结构(bounded queue、ring buffer、resource pool 等都用这套)。
