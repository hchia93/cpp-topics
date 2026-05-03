// 练习题 07:lock-free 占槽 + 写数据
//
// 目的:CAS 占槽 + 后续普通写,综合两种 CAS 变种
// 难度:中等偏上(20-30 分钟)
//
// 这道题把 Test_06 的"条件 CAS"和"占完之后还要做事"组合在一起,
// 是真实 lock-free 数据结构(SPMC 队列、ring buffer、free list)的
// 最小骨架。

// =====================================================================
// 题目
// =====================================================================
//
// 场景:
//   一个固定大小的 Buffer(Capacity 个 int),多线程并发 Append。
//   Append 的两步流程:
//     1. CAS 占槽:把 Size 从 N 改到 N+1,我就拥有了第 N 号槽
//     2. 普通写:Buffer[N] = Value
//
// 实现 TLockFreeAppender:
//   - TryAppend(int Value):占槽 + 写入。返回是否成功
//   - GetSize():当前已占用的槽位数
//   - GetAt(int Idx):读取第 Idx 号槽的值
//
// 关键:**两个线程不能同时占住同一号槽**。占槽必须是原子的。
// CAS 失败的线程不能写 Buffer,否则可能踩别人的写。
//
// 提示:
//   1. load 当前 Size 为 OldSize
//   2. 如果 OldSize >= Capacity,return false(满)
//   3. CAS(OldSize → OldSize + 1)占住第 OldSize 号槽
//   4. 成功:Buffer[OldSize] = Value,return true
//   5. 失败:OldSize 已被刷新,回到第 2 步重试
//
// 注意第 4 步:**CAS 成功之后才能写 Buffer**。
// 一旦 CAS 失败,你没占住任何槽,绝不能写。

#include <atomic>
#include <cassert>
#include <iostream>
#include <set>
#include <thread>
#include <vector>

class TLockFreeAppender
{
public:
    static constexpr int Capacity = 100;

    bool TryAppend(int Value)
    {
        // TODO:占槽 + 写入
        return false;
    }

    int GetSize() const
    {
        // TODO
        return 0;
    }

    int GetAt(int Idx) const
    {
        // TODO
        return 0;
    }

private:
    std::atomic<int> Size{0};
    int Buffer[Capacity] = {};   // 普通数组,**靠 CAS 占槽保证不冲突**
};

int main()
{
    std::cout << "--- Test 07: TLockFreeAppender ---\n";

    TLockFreeAppender Appender;
    std::atomic<int> SuccessCount{0};

    constexpr int Threads = 8;
    constexpr int IterPerThread = 50;   // 总尝试 400 次,只能成功 100 次

    std::vector<std::thread> Workers;
    for (int i = 0; i < Threads; ++i)
    {
        Workers.emplace_back([&, i]
        {
            for (int j = 0; j < IterPerThread; ++j)
            {
                int Value = i * 1000 + j;   // 全局唯一值
                if (Appender.TryAppend(Value))
                {
                    SuccessCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& T : Workers) T.join();

    const int FinalSize = Appender.GetSize();
    const int Success = SuccessCount.load();

    std::cout << "Final size: " << FinalSize
              << " (expected " << TLockFreeAppender::Capacity << ")\n";
    std::cout << "Successes:  " << Success << "\n";

    assert(FinalSize == TLockFreeAppender::Capacity);
    assert(Success == TLockFreeAppender::Capacity);

    // 验证:每个槽都被占了一次,Buffer 里的值全部唯一
    std::set<int> Unique;
    for (int i = 0; i < FinalSize; ++i)
    {
        Unique.insert(Appender.GetAt(i));
    }
    std::cout << "Unique values in buffer: " << Unique.size()
              << " (expected " << TLockFreeAppender::Capacity << ")\n";
    assert(Unique.size() == TLockFreeAppender::Capacity);

    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检
// =====================================================================
//
// bool TryAppend(int Value)
// {
//     int OldSize = Size.load(std::memory_order_relaxed);
//     while (true)
//     {
//         if (OldSize >= Capacity) return false;
//
//         if (Size.compare_exchange_weak(
//                 OldSize, OldSize + 1,
//                 std::memory_order_relaxed,
//                 std::memory_order_relaxed))
//         {
//             // CAS 成功,我占住了第 OldSize 号槽
//             Buffer[OldSize] = Value;          // 普通写,不会和别人撞
//             return true;
//         }
//         // CAS 失败,OldSize 已被刷新,loop 重判
//     }
// }
//
// int GetSize() const  { return Size.load(std::memory_order_relaxed); }
// int GetAt(int Idx) const { return Buffer[Idx]; }
//
// =====================================================================
// 关键点:为什么 Buffer[OldSize] = Value 是安全的普通写
// =====================================================================
//
// 你可能问:Buffer 不是 atomic,为什么多线程并发写不冲突?
//
// 答:**因为 CAS 占槽保证了不会有两个线程拿到同一个 OldSize**。
//
// 想象 8 个线程同时进入 TryAppend,Size 当前是 50:
//   - 8 个线程都 load 到 OldSize = 50
//   - 8 个 CAS(50 → 51)同时发起
//   - **只有一个会成功**,其余 7 个失败,OldSize 被刷成 51
//   - 那 1 个胜者写 Buffer[50],其他 7 个继续重试
//
// 每个槽位**最多被一个线程**写,所以 Buffer 元素之间没有 race。
// CAS 把"占槽"这件事原子化,后续写就是安全的。
//
// =====================================================================
// 实战意义
// =====================================================================
//
// 这个模式是 SPMC / MPMC ring buffer、free list、
// 一些 work-stealing job system 的核心套路:
//
//   - 用一个原子计数 + CAS 占索引
//   - 占到索引后,索引指向的内存就是"我私有的"
//   - 私有写不需要原子化,只需要保证没人能再"路过"占走
//
// memory_order 在这个 toy 例子里用 relaxed 都行,
// 因为测试 join 后才读。但**真实场景下**(读者和写者并发),
// CAS 的 success 端要 release(发布数据指针),
// 读者 load Size 时要 acquire,这样才能看到 Buffer[N] 的写。
