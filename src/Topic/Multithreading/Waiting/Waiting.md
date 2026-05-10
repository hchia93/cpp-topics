# Waiting

游戏栈场景:线程之间的"协作通知"。一方等条件成立才能继续,另一方负责把条件改成立后通知它。围绕 `std::condition_variable` 的核心 pattern 与陷阱。

建议阅读顺序:

| # | 文件 | 概念 | 一句话 |
|---|------|------|--------|
| 1 | [Order.cpp](Order.cpp) | `wait` + `notify_one` 最小骨架 | 一个顾客等一个厨师做菜 |
| 2 | [BossDeath.cpp](BossDeath.cpp) | `notify_all` 广播 | 多 looter 等 boss 死后一起冲进来分赃 |
| 3 | [LostWakeup.cpp](LostWakeup.cpp) | 为何 `wait` 必须带 predicate | notifier 抢跑导致裸 wait 死等,带 Pred 版立刻返回 |
| 4 | [Timeout.cpp](Timeout.cpp) | `wait_for` / `wait_until` | 慢服务器 + 500ms 上限,超时显示"加载失败" |
| 5 | [BoundedQueue.cpp](BoundedQueue.cpp) | producer-consumer 综合应用 | 双 CV(NotEmpty/NotFull) + 优雅 stop,前 4 题打牢基础后的整合样本 |

主线:从最小骨架(单等待者)→ 广播(多等待者)→ 经典坑(假醒/丢通知)→ 限时等待。最后用 `BoundedQueue.cpp` 把双 CV 整合成产消队列。

## CV 三件套(永远成对出现)

```cpp
std::mutex Mutex;
std::condition_variable CV;
T SharedState;          // 真相所在,例:bool bReady; std::queue<X> Q;
```

- `wait` 一侧:`std::unique_lock<std::mutex>` + `CV.wait(Lock, Predicate)`
- `notify` 一侧:**先持锁改 SharedState,再(可选释放锁后)调 notify_one / notify_all**
- 不带 predicate 的裸 `wait(Lock)` 永远不要写,会被 spurious wakeup 坑
