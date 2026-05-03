// 练习题 05:原子 min(Test_02 的镜像)
//
// 目的:用最简单的对称性把 CAS 循环模板再练一遍,建立肌肉记忆
// 难度:基础(5 分钟)
//
// 这道题就是 Test_02 的反向版。如果之前过了 Test_02,
// 应该立刻能写出来。卡住就回去看你 Test_02 自己写的版本。

// =====================================================================
// 题目
// =====================================================================
//
// 实现 TLowestScoreBoard:
//   - ReportScore(int Score):并发提交一个分数
//   - GetLowestScore():返回当前最低分
//
// 高尔夫式排行榜,**最低分获胜**。多线程并发 ReportScore,
// 内部要原子地更新成 min(当前, Score)。
//
// 提示:
//   - 把 Test_02 的代码复制过来,把 > 改成 <,把 max 改成 min
//   - 注意初始值要用 INT_MAX(任何分数都会比它低)
//   - 模板:CAS loop 取-算-CAS-重试

#include <atomic>
#include <cassert>
#include <climits>
#include <iostream>
#include <thread>
#include <vector>

class TLowestScoreBoard
{
public:
    void ReportScore(int Score)
    {
        // TODO:CAS 循环更新 LowestScore 为 min(当前, Score)
    }

    int GetLowestScore() const
    {
        // TODO
        return 0;
    }

private:
    std::atomic<int> LowestScore{INT_MAX};   // 初始值最大,谁来都低
};

int main()
{
    std::cout << "--- Test 05: TLowestScoreBoard ---\n";

    TLowestScoreBoard Board;
    std::vector<int> Scores = {50, 200, 30, 150, 80, 175, 90, 220, 60, 110};

    std::vector<std::thread> Reporters;
    for (int Score : Scores)
    {
        Reporters.emplace_back([&, Score] { Board.ReportScore(Score); });
    }
    for (auto& T : Reporters) T.join();

    const int Expected = 30;
    const int Actual = Board.GetLowestScore();
    std::cout << "Expected: " << Expected << ", Actual: " << Actual << "\n";
    assert(Actual == Expected);
    std::cout << "PASSED\n";
    return 0;
}

// =====================================================================
// 自检
// =====================================================================
//
// void ReportScore(int Score)
// {
//     int Old = LowestScore.load(std::memory_order_relaxed);
//     while (Score < Old)
//     {
//         if (LowestScore.compare_exchange_weak(
//                 Old, Score,
//                 std::memory_order_relaxed,
//                 std::memory_order_relaxed))
//         {
//             return;
//         }
//         // CAS 失败,Old 已被刷新成当前最新值,loop 重判 Score < Old
//     }
// }
//
// int GetLowestScore() const
// {
//     return LowestScore.load(std::memory_order_relaxed);
// }
//
// 整套和 Test_02 几乎一模一样,只有 max 翻成 min。
// 如果你能秒写,说明 Test_02 那个 pattern 已经基本固化。
