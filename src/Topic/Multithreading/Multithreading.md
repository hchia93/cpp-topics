# Multithreading 模式速查

游戏栈常见的多线程组织方式。每种模式 = 把"哪些线程做什么、怎么协调"固化成一个可复用的结构。

## 两个根本目的

多线程的存在不只是为了"快"。项目里多线程的动机通常落在两类之一,常常两者并存:

1. **并行加速**:同一类工作切片送到不同核(数据并行 / 任务并行)
2. **职责分线**:不同性质的工作互不打扰(渲染线程、网络线程、主逻辑、GC)

模式的差别就是这两个目的下的不同结构。

## 模式表

| 模式 | 一句话 | 游戏场景 | 关键原语 |
|------|--------|---------|---------|
| **Fork-Join** | 主线切分 → N 个 worker 并行 → join → 合并 | 并行 update 实体、并行 culling、parallel-for | `thread` / `barrier` / `latch` |
| **Producer-Consumer** | 有界队列,生产者塞、消费者取 | 资源加载队列、音频混音、日志刷盘 | `mutex` + `condition_variable` |
| **Pipeline** | 每阶段一条线程,数据按顺序流过阶段 | 帧管线:逻辑 → 物理 → 渲染提交 | 每段一个 SPSC 队列 |
| **Worker Pool** | 固定线程数 + 任务队列,按需派发 | 通用线程池 | `mutex` + `cv` + queue |
| **Job System** | 细粒度任务 + 依赖图 + work-stealing 调度 | Unreal Tasks、Unity Job System | `atomic` + work-stealing deque |
| **Actor / Mailbox** | 每对象一个邮箱,外部只发消息进来 | 网络模拟、AI 黑板、玩家会话 | 单消费者队列 + 调度器 |
| **Reactor / Event Loop** | 单线程 + 事件队列,事件驱动 | 主线程派发、网络 io_uring | `epoll` / `kqueue` / IOCP |
| **Double Buffer** | 一份在写、一份在读,帧末交换 | render thread 读上一帧 game state | `atomic<T*>` swap |
| **Fan-Out / Fan-In** | 一份请求散到多 worker,聚合结果 | 并行 raycast、并行 IK 求解 | `future` / `latch` |
| **RCU / Read-Mostly** | 读无锁,写者复制后原子替换指针 | 导航网格、配置热更新 | `atomic<shared_ptr<T>>` |

## 选型经验

| 工作特点 | 优先考虑 |
|---------|---------|
| 大量同质数据、无依赖 | Fork-Join / parallel-for |
| 大量小任务、依赖复杂 | Job System |
| 任务异步流入、处理速率不均 | Producer-Consumer / Worker Pool |
| 阶段固定、顺序固定 | Pipeline |
| 长寿对象 + 大量请求来访 | Actor |
| 一个状态,99% 读 1% 写 | RCU / Double Buffer |
| 大量 IO 等待 | Reactor + coroutine |

## 与基础原语的对应

模式不是替代原语,是把原语**组合**成有名字的结构。

| 原语 | 解决的最小问题 | 出现在哪些模式 |
|------|--------------|---------------|
| `mutex` | 临界区独占 | 几乎所有 |
| `atomic` | 单变量读改写 | Fork-Join 计数、Job System、CAS 类 |
| `condition_variable` | 等条件变化时不忙等 | Producer-Consumer、Worker Pool |
| `barrier` / `latch` | 等所有人到齐 | Fork-Join、Fan-In |
| `future` / `promise` | 拿一次性结果 | Fan-Out / Fan-In |
| `coroutine` | 把异步链写成线性 | Reactor、IO 重的任何场景 |

## 在本仓库的子主题地图

按 sampling 风格,每个模式占一个子目录,目录内同名 `.md` 索引样本:

```
Topic/Multithreading/
├── Multithreading.md       # 本文件
├── Consumption/            # ✅ 抢资源:atomic、CAS、mutex
├── Waiting/                # condition_variable 主场:等通知、广播
├── Pipeline/               # 帧管线:多段队列串接
├── WorkerPool/             # 线程池:任务派发、生命周期
├── JobSystem/              # 依赖图 + work-stealing
├── DoubleBuffer/           # 渲染/逻辑双缓冲
└── Coroutine/              # co_await 把异步写成同步
```

> 子目录里的 `.cpp` 不带数字前缀,顺序信息只放在子目录的 `.md` 里。
