![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)

# cpp-topics

C++ 概念库。记录写法、Type wrapper、算法演示等可回头参考的内容。每个文件自包含，附带可执行的演示。

## 结构

```
src/
├── Type/                       # 轻量 wrapper 类型
│   ├── TSharedState            # 多源共享状态控制，命名键值，幂等释放
│   └── TOptionalOverride       # 可选覆盖值，消除哨兵值约定
│
└── Topic/                      # 主题演示
    ├── Concurrency/            # 并发原语：mutex, condition_variable, atomic
    └── LinkedList/             # 链表算法合集
```

## Type

| 名称 | 用途 |
|------|------|
| `TSharedState<T>` | 多个来源共同控制一个状态，任一来源为真则激活。Set/Unset 以来源名称为键，Unset 幂等。 |
| `TOptionalOverride<T>` | 包装一个值和一个 set 标记。AssignOr 替换默认值，MultiplyOr 缩放累加器，未设置时透传。 |
