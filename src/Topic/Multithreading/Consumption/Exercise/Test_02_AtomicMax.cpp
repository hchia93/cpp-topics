// 练习题 02:原子 max(用 CAS 循环)
//
// 用法:
//   1. 把 // TODO 标记的部分实现完整
//   2. 不要修改测试代码部分
//   3. 跑一遍看 assertion 是否通过
//
// 参考:同目录下的 AtomicOps.cpp 里的 DemoAtomicTransform 段
//
// 难度:中等
// 预计:15-20 分钟

// =====================================================================
// 题目
// =====================================================================
//
// 实现 THighScoreBoard:
//   - ReportScore(int Score):并发提交一个分数
//   - GetHighScore():返回当前最高分
//
// 多个线程同时 ReportScore,内部要原子地更新成 max(当前, Score)。
// 硬件没有 fetch_max,必须自己写 CAS 循环。
//
// 提示(取-算-CAS-重试模板):
//   1. load 当前最高分 Current
//   2. 如果 Score <= Current,什么都不用做(没必要写),直接返回
//   3. 否则尝试 compare_exchange_weak(期望 = Current, 新值 = Score)
//      - 成功:更新成功,完成
//      - 失败:CAS 自动把 Current 刷新成最新值,回到第 2 步重试

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

class THighScoreBoard
{
public:
    void ReportScore(int Score)
    {
        // TODO:写 CAS 循环更新 HighScore 为 max(当前, Score)
        int Old = GetHighScore();

        while (Score > Old)
        {
            if(HighScore.compare_exchange_weak(Old, Score, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                return;
            }
        }
    }

    int GetHighScore() const
    {
        return HighScore.load(std::memory_order_relaxed);
    }

private:
    // TODO:改成 std::atomic<int>
    // int HighScore = 0;
    std::atomic<int> HighScore {0};
};

int main()
{
    std::cout << "--- Test 02: THighScoreBoard ---\n";

    THighScoreBoard Board;
    std::vector<int> Scores = {50, 200, 30, 150, 80, 175, 90, 220, 60, 110};

    std::vector<std::thread> Reporters;
    for (int Score : Scores)
    {
        Reporters.emplace_back([&, Score] { Board.ReportScore(Score); });
    }
    for (auto& T : Reporters) T.join();

    const int Expected = 220;
    const int Actual = Board.GetHighScore();
    std::cout << "Expected: " << Expected << ", Actual: " << Actual << "\n";
    assert(Actual == Expected);
    std::cout << "PASSED\n";
    return 0;
}
