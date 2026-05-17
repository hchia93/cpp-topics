# Multithreading 学习进度

按 sampling 风格补 multithreading 基础到工程化原语,记录走到哪、还差什么。

## 总览

基于"完整 multithreading 学习目标"的粗估,不是精确测量。

| 维度 | 估算 | 一句话定性 |
|------|------|-----------|
| 概念覆盖 | ~63% | 单变量同步 + CV + memory_order 配对(含 acq_rel R/M/W 拆解) + CAS 几种变种已通,异步契约层与高阶模式未碰 |
| 代码样本 | ~53% | Consumption 完整,Waiting 五件齐,LastHit.cpp 已读 + 重构,Exercise 进到 09 ✅,10 待做,Pipeline 起步 |
| 综合 | ~52% | acquire/release 推到 75%,acq_rel 推到 60%,CAS 失败侧 acquire 语义内化 |

## 仓库重整(2026-05-11)

| 旧路径 | 新路径 |
|--------|--------|
| `Topic/Concurrency/Concurrency.cpp` | `Topic/Multithreading/Waiting/BoundedQueue.cpp` |
| `Topic/Multithreading/Exercise/` | `Topic/Multithreading/Consumption/Exercise/` |
| `Exercise/AtomicOps.cpp` | `Consumption/Exercise/Example_AtomicOps.cpp`(改名) |

Concurrency 子目录是 Multithreading 的一部分,已合并。Exercise 归属到 Consumption(全是 atomic/CAS/spin 题)。`AtomicOps.cpp` 是练习参考样本,改前缀 `Example_` 与 `Test_` 区分。

## 概念覆盖

理解度估读:**能不能脱离提示自己解释清楚 + 在新 snippet 里识别出这个 pattern**。低于 60% 的需要"死嗑"。

| 领域 | 已覆盖 | 状态 | 理解度 |
|------|--------|------|--------|
| thread 基础 | `emplace_back`、`join`、`detach`、lambda 按引用捕获、`vector<thread>` 批量起 | ✅ | 85% |
| race condition / lost update | 现象 + 机制,跨核 cache coherence vs 单核中断的对照 | ✅ | 85% |
| 单核 / 多核 + 编译器优化对原子性的影响 | volatile 强制拆 RMW、affinity 实测、`lock` 前缀作用 | ✅ | 70% |
| atomic 家族 | `load`/`store`/`exchange`/`fetch_add`/`fetch_sub` 返回旧值的语义 | ✅ | 80% |
| CAS / `compare_exchange` | 通用模板(while + 条件 + CAS + 自动重判),Test_05/06/07/08 反复练 | ✅ | 80% |
| **CAS 通用模板识别**(while + cond + CAS + 重判) | 自己识别出"max/min/bounded/permit pool 是同一个模板"的能力 | ✅ | 75% |
| weak vs strong | 概念清楚,标准用法(loop 用 weak,单次用 strong) | ✅ | 70% |
| ABA 问题 | 提了一次,未演练 | ⏳ | 25% |
| memory_order(relaxed / seq_cst) | 两端语义,与分布式一致性等级的对应 | ✅ | 75% |
| memory_order(acquire / release) | Test_03/04/08 反复见配对 pattern。LastHit.cpp 全员 order 解码,识别 release 冗余 vs 必须、failure 一定 ≤ acquire | ✅ | 75% |
| memory_order(acq_rel) | LastHit CAS 的 acq_rel 拆成 R/M/W 模型,理解 R+W 同时承担两份职责。还没自己写过完整 ref count | 🟡 | 60% |
| mutex / lock_guard / unique_lock | 在 OverKill.cpp 见过,`lock_guard` vs `unique_lock` 区别未深入 | 🟡 | 60% |
| **spinlock 设计与陷阱**(自己实现过) | Test_04 从 0 写过,踩过 5+ 个 bug,知道哪些场景不适合 | ✅ | 75% |
| condition_variable | `wait(Lock, Pred)` + `notify_one`/`notify_all`/`wait_for` 全见过,5 个 Waiting/ demo 齐 | ✅ | 75% |
| spurious wakeup / lost wakeup | LostWakeup.cpp 强制重现 lost wakeup,无 Pred 裸 wait 死等 | ✅ | 70% |
| 关键字辨析:volatile | 四个合法用途 + 不是同步原语 | ✅ | 85% |
| 关键字辨析:mutable | 外部 const 内部可改,lambda mutable | ✅ | 85% |
| 关键字辨析:const_cast | 通常是签名设计错,Scott Meyers pattern 是正当用法 | ✅ | 75% |
| 异步契约层(future / promise / async) | 比喻精准,代码层 0 实战 | 🟡 | 55% |
| coroutine | 位置定位清楚,没碰 `co_await` | ⏳ | 35% |
| 多线程模式速查 | `Multithreading.md` 十种模式 + 选型经验,Pipeline 首样本已实装(Producer/Worker/Sink + 关闭协议 + join 拓扑序),其余子主题样本未动 | 🟡 | 35% |
| 工程基础设施 | `build.ps1` + Topic/Type 目录约定 | ✅ | 90% |
| shared_mutex / 读写锁 | 未碰 | ⏳ | 5% |
| semaphore / latch / barrier | 未碰 | ⏳ | 5% |
| lock-free 数据结构 | 未碰 | ⏳ | 5% |
| 性能议题(false sharing / NUMA / cache line) | 未碰 | ⏳ | 5% |
| 测试工具(TSan / Helgrind / model checking) | 未碰 | ⏳ | 0% |

## 死嗑清单(<60% 的领域,优先补)

| 领域 | 现值 | 需要做 |
|------|------|--------|
| async / future / promise | 55% | 写一个最小 promise+future 的 snippet,亲手摸一次 |
| memory_order acq_rel | 50% | 写一个 ref count 的小样本,亲手用 acq_rel |
| coroutine | 35% | 写一个 `co_await` 的最小 awaitable,理解暂停/恢复机制 |
| ABA 问题 | 25% | 看一个反例 snippet 后,知道何时该警惕 |

## 代码样本清单

### 已完成

**演示样本**(我写的,你跑过):

| 路径 | 主题 | 关键点 |
|------|------|--------|
| `Consumption/Race.cpp` | race / lost update | 裸 `int` 扣血,总伤害 < 攻击次数 |
| `Consumption/Atomic.cpp` | `fetch_sub` 修复 | 单变量计数,伤害守恒 |
| `Consumption/LastHit.cpp` | CAS 抢击杀者 | `compare_exchange_weak` 多攻击者抢归属 |
| `Consumption/OverKill.cpp` | mutex 不变式 | `bAlive == (HP > 0)`,`OnDeath` 仅一次 |
| `Consumption/LastHit.cpp` | CAS 抢击杀 + acq_rel 实战 | 已读 + 重构 load 外置。每个 order 都能解释为什么这么选 |
| `Waiting/Order.cpp` | CV 最小骨架 | `wait(Lock, Pred)` + `notify_one` 顾客等厨师 |
| `Waiting/BossDeath.cpp` | `notify_all` 广播 | N looter 等 boss 死,只 notify_one 会死锁 |
| `Waiting/LostWakeup.cpp` | wait 必须带 predicate | notifier 抢跑实演 lost wakeup,Pred 版自救 |
| `Waiting/Timeout.cpp` | `wait_for` 限时等 | 500ms 上限,超时放弃,网络请求标配 |
| `Waiting/BoundedQueue.cpp` | producer-consumer 综合 | 双 CV(NotEmpty/NotFull)+ 优雅 stop |
| `Pipeline/Pipeline.cpp` | 三段流水线 + 关闭协议 | Producer→Worker→Sink,join 拓扑序,首个 3+ 角色协调样本 |
| `Consumption/Exercise/Example_AtomicOps.cpp` | atomic 家族速查 | 6 个 demo 涵盖 load/store/exchange/fetch_X/CAS/release-acquire |

**练习题**(你自己写的实现):

| 路径 | 状态 | 主题 |
|------|------|------|
| `Consumption/Exercise/Test_01_Counter.cpp` | ✅ | 基础 fetch_X 计数器 |
| `Consumption/Exercise/Test_02_AtomicMax.cpp` | ✅ | CAS 循环,原子 max |
| `Consumption/Exercise/Test_03_PublishSubscribe.cpp` | ✅ | memory_order release/acquire 配对 |
| `Consumption/Exercise/Test_04_SpinLock.cpp` | ✅ | 从 0 实现完整 spinlock(踩过 bug 后修对) |
| `Consumption/Exercise/Test_05_AtomicMin.cpp` | ✅ | Test_02 镜像,改对后形式美观 |
| `Consumption/Exercise/Test_06_BoundedCounter.cpp` | ✅ | 条件 CAS(满了不写),识别出与 05 同模板 |
| `Consumption/Exercise/Test_07_SlotClaimer.cpp` | ✅ | CAS 占槽 + 普通写,实现正确 |
| `Consumption/Exercise/Test_08_PermitPool.cpp` | ✅ | TryDo/Do + 使用方完整。修过 Capacity vs OldCapacity 笔误。sleep 保留 200ms 当压力测试 |
| `Consumption/Exercise/Test_09_CountdownLatch.cpp` | ✅ | spin-Wait + acq/rel 配对,N release ↔ 1 acquire。Wait 在 join 前,看出"同步点"≠"线程生命周期" |
| `Consumption/Exercise/Test_10_OneShotFlag.cpp` | ⏳ | publish-subscribe 广播版,1 release ↔ N acquire |

**附加实测**(没落 .cpp,但跑过):

- 单核 vs 多核 affinity 对照,锁定 CPU0 跑 `Race.exe` 看 race 表现差异
- `volatile` 强制 3 指令 RMW,让单核也能露出 race

### 重点提醒:还没看的现成样本

- (LastHit 已通,这条本来在这里,已挪到上面表格)

### 待写

| 路径 | 主题 | 优先级 |
|------|------|--------|
| `Pipeline/*` | 多段 SPSC 队列串接,帧管线(首样本已落 Pipeline.cpp) | 中 |
| `WorkerPool/*` | 固定线程数 + 任务队列 | 中 |
| `JobSystem/*` | 依赖图 + work-stealing | 低 |
| `DoubleBuffer/*` | `atomic<T*>` swap,渲染读上一帧 | 低 |
| `Coroutine/*` | `co_await`、`std::generator` | 低 |
| memory_order publish-subscribe | release/acquire 落地样本 | 中 |

## 未覆盖内容

明确列出知道有这些但还没碰的:

- `shared_mutex` / 读写锁
- `semaphore` / `latch` / `barrier`
- `future` / `promise` / `async` 的代码样本(只剩概念)
- coroutine 的代码样本(`co_await`、`std::generator`)
- lock-free 数据结构(SPSC / MPSC queue)
- thread pool / job system 的实际实现
- 性能议题:false sharing、NUMA、cache line bouncing
- 测试工具:TSan、Helgrind、model checking
- ABA 问题的实战防御(只在 LastHit 里碰过 CAS,没专门演示 ABA)
- memory_order acquire/release 的深入样本

## 下一步建议

按推荐顺序:

1. **做 Test_09 + Test_10**(复习 spin-Wait + release/acquire 配对,无 CAS 干扰)
2. **memory_order acq_rel 实战**:写一个 mini ref count(类似 shared_ptr 内部的 ControlBlock),把 acq_rel 从 60% 推到 80%
3. **挑一个高阶模式起子主题**:Pipeline 或 WorkerPool 二选一
4. **起 `WorkerPool/` 或写 `std::jthread`+`stop_token` 协作取消样本**(把手排 join 序消掉)

## 基础设施摘要

| 项 | 内容 |
|----|------|
| 构建脚本 | `build.ps1`,单文件编译运行,自动激活 MSVC 环境,首次激活后同会话不重复 |
| 入口约定 | 含 `main` 的 `.cpp` 是入口,头文件相邻放 |
| 子主题组织 | Multithreading 子主题用平铺多 `.cpp` 模式,目录内同名 `.md` 做样本索引 |
| 命名 | Unreal 风:PascalCase,类前缀 `T`,布尔变量前缀 `b` |
| 注释语言 | 本仓库允许中文 |
