# Waiting

游戏栈场景:线程之间的"协作通知"。一方等条件成立才能继续,另一方负责把条件改成立后通知它。围绕 `std::condition_variable` 的核心 pattern 与陷阱。

建议阅读顺序:

| # | 文件 | 概念 | 一句话 |
|---|------|------|--------|
| 1 | [Order.cpp](Order.cpp) | `wait` + `notify_one` 最小骨架 | 一个顾客等一个厨师做菜 |
| 2 | BossDeath.cpp(待写) | `notify_all` 广播 | 多 attacker 等 boss 死后一起冲进来分赃 |
| 3 | LostWakeup.cpp(待写) | 为何 `wait` 必须带 predicate | spurious wakeup + lost wakeup 实演 |
| 4 | Timeout.cpp(待写) | `wait_for` / `wait_until` | 等不到就放弃,不能死等 |

主线:从最小骨架(单等待者)→ 广播(多等待者)→ 经典坑(假醒/丢通知)→ 限时等待。

## CV 三件套(永远成对出现)

```cpp
std::mutex Mutex;
std::condition_variable CV;
T SharedState;          // 真相所在,例:bool bReady; std::queue<X> Q;
```

- `wait` 一侧:`std::unique_lock<std::mutex>` + `CV.wait(Lock, Predicate)`
- `notify` 一侧:**先持锁改 SharedState,再(可选释放锁后)调 notify_one / notify_all**
- 不带 predicate 的裸 `wait(Lock)` 永远不要写,会被 spurious wakeup 坑
