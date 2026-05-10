// 练习题 08:Permit Pool(信号量风格)
//
// 目的:打 TryDo / Do 这一对模式,以及"使用方"的代码
// 难度:中等(25-35 分钟)
//
// 和前面几道的差别:
//   - 不再实现 Get,你已经会了
//   - main() 里也有 TODO,你要补**使用方**的 work cycle
//   - 这道题考的不只是实现,还考你"会不会拿这个原语去用"

// =====================================================================
// 题目 1:实现 TPermitPool
// =====================================================================
//
// 场景:
//   并发限流。最多 K 个 worker 同时执行某段工作,
//   超过 K 的人要排队等(spin 等)。
//   类比:停车场只有 K 个车位,有车位才能进。
//
// 实现:
//   - TryAcquire():试一次。有许可拿走返回 true,没许可返回 false
//   - Acquire():spin 等,直到拿到许可为止
//   - Release():归还一个许可
//
// 提示:
//   - TryAcquire 是 CAS **条件递减**(Old > 0 时 Old → Old-1)
//     完全是 Test_06 的镜像(那道是条件递增,这道是条件递减)
//   - Acquire 在外层 while 调 TryAcquire,失败就 yield
//   - Release 是 fetch_add(1),最简单
//
//   memory_order:
//   - TryAcquire 成功侧用 acquire(配对前一个 Releaser 的 release)
//   - Release 用 release(发布临界区里的写)

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class TPermitPool
{
public:
    static constexpr int Capacity = 3;

    bool TryAcquire()
    {
        int OldCapacity = Available.load(std::memory_order_relaxed);

        // TODO:CAS 条件递减(Old > 0 才能 -1)
        while(OldCapacity > 0)
        {
            if (Available.compare_exchange_weak(OldCapacity, OldCapacity - 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    void Acquire()
    {
        while(!TryAcquire())
        {
            std::this_thread::yield();
        }
    }

    void Release()
    {
        Available.fetch_add(1, std::memory_order_release);
    }

private:
    std::atomic<int> Available{Capacity};
};

// =====================================================================
// 题目 2:写测试主体里的 worker lambda
// =====================================================================
//
// 每个 worker 重复 IterPerWorker 次以下流程:
//   1. Acquire 一个许可(进临界区)
//   2. ++Inside,记录"当前并发数",顺便更新 MaxInside 的历史峰值
//   3. 模拟工作:sleep_for(几百微秒,确保会有多线程并发)
//   4. --Inside
//   5. Release 许可
//   6. ++Completed
//
// 提示:
//   - "更新 MaxInside" 是 atomic max,Test_02 那个 CAS loop 模板
//   - sleep 时长建议 100~500 微秒,够让别的线程也进临界区,但不会拖慢测试
//   - 步骤 1-5 是临界区,Inside 的 ++/-- 必须在 Acquire/Release 之间

int main()
{
    std::cout << "--- Test 08: TPermitPool ---\n";

    TPermitPool Pool;
    std::atomic<int> Inside{0};        // 当前在临界区的 worker 数
    std::atomic<int> MaxInside{0};     // 历史峰值
    std::atomic<int> Completed{0};     // 完成的工作总数

    constexpr int Workers = 8;
    constexpr int IterPerWorker = 30;

    std::vector<std::thread> Threads;
    for (int i = 0; i < Workers; ++i)
    {
        Threads.emplace_back([&]
        {
            for (int j = 0; j < IterPerWorker; ++j)
            {
                Pool.Acquire();
                int Now = Inside.fetch_add(1, std::memory_order_relaxed) + 1;
                
                int OldMax = MaxInside.load(std::memory_order_relaxed);

                while(Now > OldMax)
                {
                    if (MaxInside.compare_exchange_weak(OldMax, Now, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        break;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200)); //强制逗留

                Inside.fetch_sub(1, std::memory_order_relaxed);
                Pool.Release();
                Completed.fetch_add(1, std::memory_order_relaxed);
                // TODO:写一个完整的 work cycle:Acquire → 进临界 → 工作 → 出临界 → Release
                //
                // 提醒:
                //   - 用 Pool.Acquire() 进,Pool.Release() 出
                //   - 进出之间维护 Inside 计数
                //   - 把 MaxInside 更新成 max(MaxInside, Inside) (CAS loop)
                //   - 中间放一个 sleep_for 让并发真的发生
                //   - 最后 ++Completed
            }
        });
    }
    for (auto& T : Threads) T.join();

    std::cout << "Completed:  " << Completed.load() << "\n";
    std::cout << "Max inside: " << MaxInside.load()
              << " (must be <= " << TPermitPool::Capacity << ")\n";
    std::cout << "Inside end: " << Inside.load() << " (should be 0)\n";

    assert(Completed.load() == Workers * IterPerWorker);
    assert(MaxInside.load() <= TPermitPool::Capacity);
    assert(MaxInside.load() > 0);          // 至少要有并发,否则限流没意义
    assert(Inside.load() == 0);            // 所有许可都还回来了

    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检
// =====================================================================
//
// bool TryAcquire()
// {
//     int Old = Available.load(std::memory_order_relaxed);
//     while (Old > 0)
//     {
//         if (Available.compare_exchange_weak(
//                 Old, Old - 1,
//                 std::memory_order_acquire,    // 拿到许可时,接收前任 Release 的 release
//                 std::memory_order_relaxed))
//         {
//             return true;
//         }
//     }
//     return false;
// }
//
// void Acquire()
// {
//     while (!TryAcquire())
//     {
//         std::this_thread::yield();
//     }
// }
//
// void Release()
// {
//     Available.fetch_add(1, std::memory_order_release);
//     //                              ↑ 公布临界区里的写,让下一个 Acquire 的人看到
// }
//
// =====================================================================
// 自检:worker lambda
// =====================================================================
//
// for (int j = 0; j < IterPerWorker; ++j)
// {
//     Pool.Acquire();
//
//     // 进入临界区
//     int Now = Inside.fetch_add(1, std::memory_order_relaxed) + 1;
//
//     // 更新 MaxInside = max(MaxInside, Now)
//     int OldMax = MaxInside.load(std::memory_order_relaxed);
//     while (Now > OldMax)
//     {
//         if (MaxInside.compare_exchange_weak(
//                 OldMax, Now,
//                 std::memory_order_relaxed,
//                 std::memory_order_relaxed))
//         {
//             break;
//         }
//     }
//
//     std::this_thread::sleep_for(std::chrono::microseconds(200));
//
//     // 离开临界区
//     Inside.fetch_sub(1, std::memory_order_relaxed);
//     Pool.Release();
//     Completed.fetch_add(1, std::memory_order_relaxed);
// }
//
// =====================================================================
// 这道题练到的新东西
// =====================================================================
//
// 1. TryDo / Do 这一对的标准长相:
//      - TryDo:单次 CAS,成功返回 true,失败 false
//      - Do:while(!TryDo()) yield  (spin + 礼貌让步)
//      → spinlock 的 TryLock/Lock 也是这套
//
// 2. **使用方代码**的标准长相(进出临界区 + Acquire/Release 包裹):
//      Pool.Acquire();
//        ...临界区...
//      Pool.Release();
//      → 这就是为什么有 RAII lock_guard,把 Acquire/Release 自动配对
//      (但本题没让你写 RAII,留给后面)
//
// 3. 嵌套 CAS loop:你在 worker 里写 MaxInside 的 CAS loop,
//    它和 Test_02 / Test_05 的 CAS loop 是同一个模板
//    → 同一个并发原语(CAS)在不同抽象层上反复出现
