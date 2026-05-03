// 练习题 01:线程安全的计数器
//
// 用法:
//   1. 把 // TODO 标记的部分实现完整
//   2. 不要修改测试代码部分
//   3. 跑一遍看 assertion 是否通过
//
// 参考:同目录下的 AtomicOps.cpp 是完整可跑的速查样本
//
// 难度:基础
// 预计:5 分钟

// =====================================================================
// 题目
// =====================================================================
//
// 实现 TThreadSafeCounter:
//   - Increment():计数 +1
//   - Decrement():计数 -1
//   - Get():返回当前值
//
// 多线程并发调用 Increment / Decrement,要保证最终计数精确无丢失。
//
// 提示:
//   - 把 Value 改成 std::atomic<int>
//   - 用 fetch_add / fetch_sub / load
//   - 单变量计数,memory_order 用 relaxed 即可

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

class TThreadSafeCounter
{
public:
    void Increment()
    {
        // Value += 1;
        Value.fetch_add(1, std::memory_order_relaxed);
    }

    void Decrement()
    {
        //Value -= 1;
        Value.fetch_sub(1, std::memory_order_relaxed);
        // TODO
    }

    int Get() const
    {
        return Value.load(std::memory_order_relaxed);
    }

private:
    // TODO:把这里改成 std::atomic<int>
    // int Value = 0;
    std::atomic<int> Value{ 0 };
};

int main()
{
    std::cout << "--- Test 01: TThreadSafeCounter ---\n";

    TThreadSafeCounter Counter;
    constexpr int IncThreads = 4;
    constexpr int DecThreads = 1;
    constexpr int IterPerThread = 10000;

    std::vector<std::thread> Workers;
    for (int i = 0; i < IncThreads; ++i)
    {
        Workers.emplace_back([&]
        {
            for (int j = 0; j < IterPerThread; ++j) Counter.Increment();
        });
    }
    for (int i = 0; i < DecThreads; ++i)
    {
        Workers.emplace_back([&]
        {
            for (int j = 0; j < IterPerThread; ++j) Counter.Decrement();
        });
    }
    for (auto& T : Workers) T.join();

    const int Expected = (IncThreads - DecThreads) * IterPerThread;
    const int Actual = Counter.Get();
    std::cout << "Expected: " << Expected << ", Actual: " << Actual << "\n";
    assert(Actual == Expected);
    std::cout << "PASSED\n";
    return 0;
}
