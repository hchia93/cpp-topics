![C++](https://img.shields.io/badge/C%2B%2B-20%2F23-blue?logo=cplusplus)

# cpp-topics

个人的现代 C++ (C++20/23) 学习仓库。当前聚焦在 wrapper 类型设计与独立主题演示,每个单元自包含,附带可运行示例。

## 结构

```
src/
├── Type/                       # 轻量 wrapper 类型
│   ├── TSharedState            # 多源共享状态控制,命名键值,幂等释放
│   └── TOptionalOverride       # 可选覆盖值,消除哨兵值约定
│
└── Topic/                      # 主题演示
    ├── LinkedList/             # 链表算法合集
    └── Multithreading/         # 多线程话题学习,按场景 sampling
        ├── Consumption/        # 消耗:HP 扣减、击杀归属、不变式保护
        │   └── Exercise/       # atomic / CAS / spin 练习题 + 参考样本
        └── Waiting/            # 等待:condition_variable,线程间协作通知
```

## Type

| 名称 | 用途 |
|------|------|
| `TSharedState<T>` | 多个来源共同控制一个状态,任一来源为真则激活。Set/Unset 以来源名称为键,Unset 幂等。 |
| `TOptionalOverride<T>` | 包装一个值和一个 set 标记。AssignOr 替换默认值,MultiplyOr 缩放累加器,未设置时透传。 |

## Build & Run

仓库根的 [build.ps1](build.ps1) 是单文件编译运行脚本,首次调用自动激活 MSVC,同会话二次调用跳过激活。

**约定:每个可运行单元都是一个含 `main` 的单 .cpp**,头文件相邻放置由 `cl` 自动找到。直接打中入口文件即可:

```powershell
.\build.ps1 src\Type\TSharedState.cpp
.\build.ps1 src\Type\TOptionalOverride.cpp
.\build.ps1 src\Topic\LinkedList\LinkedList.cpp
.\build.ps1 src\Topic\Multithreading\Waiting\BoundedQueue.cpp
.\build.ps1 src\Topic\Multithreading\Consumption\Race.cpp
```

只编译不运行用 `-NoRun`:

```powershell
.\build.ps1 src\Topic\Multithreading\Consumption\Race.cpp -NoRun
```

产物落在 `build\<filename>.exe`,已在 `.gitignore` 屏蔽。

### 入口文件命名约定

| 目录 | 入口文件命名 | 例子 |
|------|------------|------|
| `Type/` | `T<TypeName>.cpp`(同名 `.h` 是类型定义,`.cpp` 含演示 main) | `TSharedState.cpp` + `TSharedState.h` |
| `Topic/<Subject>/` | 与目录同名的 `.cpp` | `LinkedList/LinkedList.cpp` |
| `Topic/Multithreading/<Theme>/` | 平铺多个独立 `.cpp` | `Consumption/Race.cpp`、`Atomic.cpp` 等 |
| `Topic/Multithreading/<Theme>/Exercise/` | `Test_NN_<Name>.cpp` 自学练习 + `Example_<Name>.cpp` 参考样本 | `Test_01_Counter.cpp`、`Example_AtomicOps.cpp` |

### 前置要求

- Windows + Visual Studio 18 Community(或同代 Build Tools),含 C++ 工作负载
- PowerShell 5.1+
- 脚本路径 hardcode 在 `C:\Program Files\Microsoft Visual Studio\18\Community\`,如装在别处需改 [build.ps1](build.ps1) 里 `$vcvars` 一行
