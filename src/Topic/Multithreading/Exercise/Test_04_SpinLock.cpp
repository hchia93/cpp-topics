// 练习题 04:实现一个 Spinlock
//
// 用法:
//   1. 实现 TSpinLock 的 Lock / TryLock / Unlock
//   2. 测试用它保护一个普通 int Counter,4 个线程并发 +1
//   3. 跑一遍看 assertion 是否通过
//
// 难度:中等偏上(从 0 实现完整的同步原语)
// 预计:25-35 分钟
//
// 这道题把前几题学的全部串起来:
//   - CAS 模板(Test_02)
//   - memory_order 配对(Test_03)
//   - 加上"自旋等待"这个新动作

// =====================================================================
// 题目
// =====================================================================
//
// 实现一个最简单的 spinlock(自旋锁):
//
//   bLocked = false  →  锁空闲
//   bLocked = true   →  某个线程占用中
//
// 接口三个:
//
//   Lock()      阻塞直到拿到锁。如果已被占,**自旋**等待(不睡)
//   TryLock()   试一次:成功返回 true,失败立即返回 false
//   Unlock()    释放锁
//
// 关键决策:
//   1. TryLock 用 compare_exchange_weak 还是 strong?
//   2. 拿锁(成功 CAS)的 memory_order 是什么?
//   3. CAS 失败时的 memory_order 是什么?
//   4. 释放锁(Unlock store)的 memory_order 是什么?
//   5. Lock() 里 spin 等待时,可以做什么"礼貌"动作降低空转代价?
//
// 不要求线程公平,不要求支持递归(同一个线程二次 Lock 是 UB)。
//
// 提示:
//   - TryLock 的本质就是"原子地把 false 翻成 true,翻成功就拿到了锁"
//   - Unlock 是"原子地把 true 写回 false"
//   - 这是一对经典的 release / acquire 配对

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

class TSpinLock
{
public:
    void Lock()
    {
        // TODO:阻塞直到 TryLock() 成功
        //   提示:while 循环不停 TryLock,直到拿到为止
        //   bonus:每次失败后调一次 std::this_thread::yield() 或
        //          x86 上 _mm_pause() 让 CPU 知道这是 spin loop

        while(!TryLock())
        {
            std::this_thread::yield();
        }
    }

    bool TryLock()
    {
       bool Expected = false;
       return bLocked.compare_exchange_strong(Expected, true, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void Unlock()
    {
        bLocked.store(false, std::memory_order_release); 
    }

private:
    std::atomic<bool> bLocked{false};
};

// =====================================================================
// 验证测试
// =====================================================================
//
// 用 spinlock 保护一个普通 int Counter,4 个线程各 +1 一万次。
// 没有保护 → race condition,最终值 < 40000。
// 保护正确 → 精确等于 40000。

int main()
{
    std::cout << "--- Test 04: TSpinLock ---\n";

    TSpinLock Lock;
    int Counter = 0;   // 注意是普通 int,不是 atomic。靠 spinlock 保护。

    constexpr int Threads = 4;
    constexpr int IterPerThread = 10000;

    std::vector<std::thread> Workers;
    for (int i = 0; i < Threads; ++i)
    {
        Workers.emplace_back([&]
        {
            for (int j = 0; j < IterPerThread; ++j)
            {
                Lock.Lock();
                ++Counter;        // 临界区
                Lock.Unlock();
            }
        });
    }
    for (auto& T : Workers) T.join();

    const int Expected = Threads * IterPerThread;
    std::cout << "Expected: " << Expected
              << ", Actual: " << Counter << "\n";
    assert(Counter == Expected);
    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检(填完之后再看)
// =====================================================================
//
// 标准答案:
//
//   bool TryLock()
//   {
//       bool Expected = false;
//       return bLocked.compare_exchange_strong(
//           Expected, true,
//           std::memory_order_acquire,    // 成功:接收上次 Unlock 的 release
//           std::memory_order_relaxed);   // 失败:只看一眼,无需同步
//   }
//
//   void Lock()
//   {
//       while (!TryLock())
//       {
//           std::this_thread::yield();    // 礼貌让出 CPU,可改 _mm_pause
//       }
//   }
//
//   void Unlock()
//   {
//       bLocked.store(false, std::memory_order_release);
//       //                                ↑ 发布我在临界区改的所有东西
//   }
//
// =====================================================================
// 关键点解析
// =====================================================================
//
// 1. TryLock 用 strong 还是 weak?
//    - 这里用 strong,因为外层有 Lock() 的 while 兜底重试
//    - weak 也可以,在 spurious failure 时,Lock 会多 spin 一次,无害
//    - 但用 strong 让 TryLock 的契约更清晰:"我说不行就是真不行"
//
// 2. TryLock 成功侧为什么用 acquire?
//    - 成功 = 我刚拿到锁。锁是上一个 Unlocker 用 release 释放的。
//    - 我用 acquire 接收他的 release,**配对成 happens-before**
//    - 配对完成,他在临界区改的所有东西,我现在能看见
//    - 这正是 release/acquire pair 的经典用法
//
// 3. TryLock 失败侧为什么 relaxed 够?
//    - 失败 = 我没拿到锁。我**没有**进入临界区,**没有**要"接收"任何东西
//    - 只是看了一眼锁的状态,relaxed load 完全足够
//    - 标准要求:failure order 不能强于 success order(acquire ≥ relaxed,合法)
//
// 4. Unlock 为什么 release?
//    - 临界区里的写(++Counter)必须在 bLocked=false 之前**对其他线程可见**
//    - 下一个拿到锁的人 acquire 会看到这次 release 公布的写
//    - 顺序:`修改临界区 → release store false → 其他人 acquire 拿到 → 看到刚才的修改`
//
// 5. Lock 里 spin 时该不该让出 CPU?
//    - 空跑 while:浪费 CPU,在多核高争抢下还会让 cache line 互相打斗
//    - _mm_pause():x86 上"我在 spin"的提示,降功耗 + 减少分支预测惩罚
//    - std::this_thread::yield():主动让一个调度周期给别人
//    - sleep_for(微秒级):太久,反应迟钝
//    - 实际工业代码常用"先 spin N 次再 yield"的混合策略
//
// =====================================================================
// 思考题:这个 spinlock 在哪些场景会爆炸?
// =====================================================================
//
// 1. 临界区**长**:你 ++Counter 是几纳秒,如果换成读写文件 / 锁内调函数,
//    所有等待线程会 spin 几毫秒到几秒,**整个系统的 CPU 全在烧空轮**
//
// 2. 争抢**激烈**:8 个核全在抢一个锁,7 个核 100% spin,只有 1 个真干活
//
// 3. 持锁线程被 OS 切走:你拿着锁正干到一半被 scheduler 切走,
//    其他线程 spin 等你回来,但 OS 不会优先调度你回去(因为它不知道你持锁)
//    → 等待者把 CPU 占满,持锁者被挤得更难调度上来,**死锁式空转**
//
// 4. 优先级反转:高优先级线程 spin 等低优先级线程的锁,
//    低优先级线程被 OS 排队不起来,系统永远不前进
//
// 工程结论:
//   - 应用层默认 **std::mutex**,profile 出热点再考虑 spinlock
//   - 真要用 spinlock,临界区必须**短到几条指令**,且争抢必须**少**
//   - 工业实现(Boost / folly / tbb)的 spinlock 都加了 backoff、yield、
//     spin-then-block 等改良,自己手写的版本通常只能用作学习
//
// =====================================================================
// 进阶:这道题的"无锁版本"是什么?
// =====================================================================
//
// 注意:用 spinlock 保护 ++Counter 是**有锁**的写法。
// 真正的"无锁"等价物是:把 Counter 直接换成 std::atomic<int>,
// 用 fetch_add(1, relaxed)。完全不需要 spinlock。
//
// 这告诉你两件事:
//   1. 能用 fetch_X 做的事,不要再套 spinlock,直接 atomic 更快
//   2. spinlock 的真正用武之地是**多步操作 / 多变量不变式**(OverKill.cpp 那种)
