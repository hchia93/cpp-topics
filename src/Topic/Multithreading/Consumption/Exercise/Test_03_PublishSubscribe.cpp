// 练习题 03:发布-订阅 + memory_order
//
// 用法:
//   1. 把两处 // TODO 的 memory_order 改成最弱、最便宜的、能保证正确的值
//   2. 跑一遍看 assertion 是否通过
//   3. 用完之后回来看下面的"自检"段,对答案
//
// 参考:同目录下 AtomicOps.cpp 的 DemoPublishSubscribe 段
//
// 难度:中等(主要是判断,不是写代码)
// 预计:10-15 分钟

// =====================================================================
// 场景
// =====================================================================
//
// 模拟引擎里的经典 pattern:
//   - 逻辑线程(producer):算完一帧的玩家状态,然后发布"准备好了"
//   - 渲染线程(consumer):看到"准备好了"后读取玩家状态
//
// 关键约束:consumer 看到 bReady=true 时,**必须**也能看到 producer
// 写入的所有字段(PlayerX、PlayerY、Health)。否则就会渲染出"半截"
// 状态(比如 X 已更新但 Y 没,玩家瞬移)。
//
// 你的任务:选对 producer 端 store 的 memory_order 和
// consumer 端 load 的 memory_order,让两端**配对**形成 happens-before
// 桥梁。
//
// 当前两处都用了 seq_cst(默认、最贵、肯定对),你需要把它们各自
// 降到最弱够用的 order。

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

struct TFrameData
{
    int PlayerX = 0;
    int PlayerY = 0;
    int Health  = 0;
    std::atomic<bool> bReady{false};
};

void Publish(TFrameData& Frame, int Round)
{
    Frame.PlayerX = Round * 10;     // 普通写
    Frame.PlayerY = Round * 20;     // 普通写
    Frame.Health  = Round * 30;     // 普通写

    // TODO:把 seq_cst 改成最弱、能让"上面三行写"被一起公布的 order
    //   思路:这一刀承担"发布"角色,我之前的写都要让读端看到
    Frame.bReady.store(true, std::memory_order_release);
}

bool TryConsume(const TFrameData& Frame, int Round)
{
    // TODO:把 seq_cst 改成最弱、能让"下面三个字段读"看到 producer 写的 order
    //   思路:这一刀承担"接收"角色,看到 true 之后的读必须能看到 producer 的写
    while (!Frame.bReady.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }

    // 走到这里,bReady = true。如果 memory_order 选对了,
    // 下面三个字段必能看到 producer 写的值。
    return Frame.PlayerX == Round * 10
        && Frame.PlayerY == Round * 20
        && Frame.Health  == Round * 30;
}

int main()
{
    std::cout << "--- Test 03: Publish-Subscribe (memory_order) ---\n";

    constexpr int Rounds = 10000;
    int FailedRounds = 0;

    for (int Round = 0; Round < Rounds; ++Round)
    {
        TFrameData Frame;   // 每轮一个新 Frame,在 main 栈上

        std::thread Producer([&] { Publish(Frame, Round); });
        std::thread Consumer([&]
        {
            if (!TryConsume(Frame, Round))
            {
                ++FailedRounds;   // 1 对 1,不会有竞争
            }
        });

        Producer.join();
        Consumer.join();
    }

    std::cout << "Rounds: " << Rounds
              << ", Failed: " << FailedRounds << "\n";
    assert(FailedRounds == 0);
    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检(填完之后再看)
// =====================================================================
//
// 标准答案:
//   producer:std::memory_order_release
//   consumer:std::memory_order_acquire
//
// 为什么?
//
// 1. release(producer 端):
//    "我之前的所有写,在这条 store 之前必须可见"。
//    所以 PlayerX / PlayerY / Health 三个普通写,**绝对**会在
//    bReady=true 公布出去之前完成。读端只要看到 bReady=true,
//    就一定能看到那三个字段的最终值。
//
// 2. acquire(consumer 端):
//    "我之后的所有读,不能被重排到这条 load 之前"。
//    所以读到 bReady=true 之后才读 PlayerX / Y / Health,
//    保证这些读不会被编译器或 CPU 偷偷提前到 load 之前去。
//
// 3. release + acquire 配对(在同一个 atomic 上)形成
//    happens-before 桥梁:producer 的写 happens-before consumer 的读。
//    这是 C++ 内存模型最经典的同步形态。
//
// =====================================================================
// 思考题:为什么 relaxed 不行?
// =====================================================================
//
// 如果两端都用 relaxed,在 x86 上你这个测试**大概率仍然过**——
// 因为 x86 是强内存模型,普通的 atomic store 已经接近 release。
// 但在 ARM / ARM64(手机、苹果 silicon、AWS Graviton)这种弱内存
// 平台上,这段代码会**偶发失败**:consumer 看到 bReady=true,
// 但 PlayerX 还是 0 没更新。
//
// 这就是"x86 上跑得好好的,客户机器上偶尔崩"那种 bug 的来源。
// 用对 memory_order **不**是为 x86 写代码,是为**所有平台**写正确代码。
//
// =====================================================================
// 思考题:为什么 seq_cst 也对(只是贵)?
// =====================================================================
//
// seq_cst 比 acquire/release 多一层"全局唯一时间线"保证。
// 你这道题不需要这层(只有一个 atomic 在协调,没有跨变量的全局排序需求),
// 所以 seq_cst 多花的钱是浪费的。
//
// x86 上 seq_cst 比 release/acquire 多一条 mfence,差几个时钟周期。
// ARM 上 seq_cst 比 release/acquire 多一条 dmb 屏障,差距更大。
//
// 工程口诀:**默认 seq_cst,profile 出热点再降到 acq_rel / release / acquire / relaxed,
// 降的时候要写注释解释为什么够用。**
