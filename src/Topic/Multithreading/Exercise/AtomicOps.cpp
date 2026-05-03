// atomic 操作家族速查与示例
//
// 涵盖:
//   - load / store / exchange
//   - fetch_add / fetch_sub
//   - fetch_and / fetch_or / fetch_xor
//   - compare_exchange_strong / weak
//   - AtomicTransform 通用 CAS loop 模板(用 lambda 包装任意复合 RMW)
//   - release / acquire publish-subscribe pattern
//
// 每个 Demo 函数演示一个主题,直接从 main 串起来跑。
// 看不懂某段时,对照 progress.md 里的概念覆盖表回查。

#include <atomic>
#include <bitset>
#include <iostream>
#include <thread>
#include <vector>

// 通用 CAS loop 模板:任意"读 → 算 → 条件写"组合成原子事务。
// lambda 必须是纯函数(可重入),CAS 失败会重试整段。
template<typename T, typename FCompute>
T AtomicTransform(std::atomic<T>& Target, FCompute ComputeNew)
{
    T Old = Target.load(std::memory_order_relaxed);
    T New;
    do
    {
        New = ComputeNew(Old);
    }
    while (!Target.compare_exchange_weak(
        Old, New,
        std::memory_order_acq_rel,
        std::memory_order_acquire));
    return Old;   // 返回旧值,模仿 fetch_X 的语义
}

// =====================================================================
void DemoLoadStore()
// =====================================================================
{
    std::cout << "\n--- load / store / exchange ---\n";

    std::atomic<int> X{0};
    X.store(42);                            // 原子写
    int V = X.load();                       // 原子读
    int Old = X.exchange(99);               // 原子写新值,返回旧值

    std::cout << "load: " << V
              << " | exchange returned: " << Old
              << " | current: " << X.load() << "\n";
}

// =====================================================================
void DemoFetchArith()
// =====================================================================
{
    std::cout << "\n--- fetch_add / fetch_sub ---\n";

    std::atomic<int> Counter{100};
    int Before = Counter.fetch_sub(30);     // -30,返回旧值
    std::cout << "fetch_sub returned old=" << Before
              << ", new=" << Counter.load() << "\n";

    // 多线程并发计数,单变量、无跨变量依赖,relaxed 够
    std::atomic<int> Shared{0};
    std::vector<std::thread> Workers;
    for (int i = 0; i < 4; ++i)
    {
        Workers.emplace_back([&]
        {
            for (int j = 0; j < 1000; ++j)
            {
                Shared.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& T : Workers) T.join();

    std::cout << "Concurrent count: " << Shared.load()
              << " (expected 4000)\n";
}

// =====================================================================
void DemoFetchBitwise()
// =====================================================================
{
    std::cout << "\n--- fetch_and / fetch_or / fetch_xor ---\n";

    std::atomic<unsigned> Flags{0b0000u};

    Flags.fetch_or(0b0011u);
    std::cout << "After fetch_or(0b0011): 0b"
              << std::bitset<4>(Flags.load()) << "\n";

    Flags.fetch_and(~0b0001u);
    std::cout << "After fetch_and(~0b0001): 0b"
              << std::bitset<4>(Flags.load()) << "\n";

    Flags.fetch_xor(0b1100u);
    std::cout << "After fetch_xor(0b1100): 0b"
              << std::bitset<4>(Flags.load()) << "\n";
}

// =====================================================================
void DemoCAS()
// =====================================================================
{
    std::cout << "\n--- compare_exchange_strong ---\n";

    std::atomic<int> X{10};
    int Expected = 10;

    bool bSuccess = X.compare_exchange_strong(Expected, 20);
    std::cout << "CAS(10->20): success=" << bSuccess
              << " | X=" << X.load()
              << " | Expected=" << Expected
              << "  (成功时 Expected 不动)\n";

    Expected = 99;   // 故意错的期望值
    bSuccess = X.compare_exchange_strong(Expected, 30);
    std::cout << "CAS(99->30): success=" << bSuccess
              << " | X=" << X.load()
              << " | Expected=" << Expected
              << "  (失败时 Expected 被刷新成当前真值)\n";
}

// =====================================================================
void DemoAtomicTransform()
// =====================================================================
{
    std::cout << "\n--- AtomicTransform (CAS loop 通用模板) ---\n";

    // 原子乘:4 个线程每个执行一次 *= 2,期望 1 * 2^4 = 16
    std::atomic<float> Multiplier{1.0f};
    std::vector<std::thread> Workers;
    for (int i = 0; i < 4; ++i)
    {
        Workers.emplace_back([&]
        {
            AtomicTransform(Multiplier, [](float V) { return V * 2.0f; });
        });
    }
    for (auto& T : Workers) T.join();
    std::cout << "1.0 * 2^4 = " << Multiplier.load()
              << " (expected 16.0)\n";

    // 原子 max:并发上报分数,只保留最高
    std::atomic<int> HighScore{0};
    std::vector<int> Scores = {50, 200, 30, 150, 80, 175, 90, 220, 60, 110};
    std::vector<std::thread> Reporters;
    for (int Score : Scores)
    {
        Reporters.emplace_back([&, Score]
        {
            AtomicTransform(HighScore, [Score](int V)
            {
                return V > Score ? V : Score;
            });
        });
    }
    for (auto& T : Reporters) T.join();
    std::cout << "Max of {50,200,30,150,80,175,90,220,60,110} = "
              << HighScore.load() << " (expected 220)\n";
}

// =====================================================================
void DemoPublishSubscribe()
// =====================================================================
{
    std::cout << "\n--- release / acquire (publish-subscribe) ---\n";

    int Data = 0;                              // 普通 int,不是 atomic
    std::atomic<bool> bReady{false};

    std::thread Producer([&]
    {
        Data = 42;                                          // (1) 普通写
        bReady.store(true, std::memory_order_release);     // (2) 公布
        // release 保证 (1) 在 (2) 之前对其它线程可见
    });

    std::thread Consumer([&]
    {
        while (!bReady.load(std::memory_order_acquire))    // (3) 等公布
        {
            std::this_thread::yield();
        }
        // acquire 保证 (4) 不会被重排到 (3) 之前;
        // 配上 producer 的 release,(4) 必能看到 (1) 写入的 42
        std::cout << "Consumer saw Data = " << Data
                  << " (expected 42)\n";                    // (4)
    });

    Producer.join();
    Consumer.join();
}

int main()
{
    DemoLoadStore();
    DemoFetchArith();
    DemoFetchBitwise();
    DemoCAS();
    DemoAtomicTransform();
    DemoPublishSubscribe();

    std::cout << "\n=== All demos completed ===\n";
    return 0;
}
