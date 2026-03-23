#include "ListNode.h"
#include <vector>
#include <queue>
#include <unordered_set>
#include <optional>
#include <cassert>

using namespace ListNodeUtil;

// LeetCode 206 - Reverse a linked list
template<typename T>
ListNode<T>* Reverse(ListNode<T>* Head)
{
    ListNode<T> Dummy;
    while (Head != nullptr)
    {
        ListNode<T>* Tmp = Head->Next;
        Head->Next = Dummy.Next;
        Dummy.Next = Head;
        Head = Tmp;
    }
    return Dummy.Next;
}

// LeetCode 25 - Reverse nodes in k-group
template<typename T>
ListNode<T>* ReverseKGroup(ListNode<T>* Head, int K)
{
    if (!Head || K == 1)
    {
        return Head;
    }

    ListNode<T> Dummy;
    Dummy.Next = Head;
    ListNode<T>* PrevGroupTail = &Dummy;

    while (true)
    {
        ListNode<T>* Kth = PrevGroupTail;
        for (int i = 0; i < K && Kth; ++i)
        {
            Kth = Kth->Next;
        }
        if (!Kth)
        {
            break;
        }

        ListNode<T>* GroupHead = PrevGroupTail->Next;
        ListNode<T>* NextGroupHead = Kth->Next;

        ListNode<T>* Prev = NextGroupHead;
        ListNode<T>* Curr = GroupHead;
        while (Curr != NextGroupHead)
        {
            ListNode<T>* Tmp = Curr->Next;
            Curr->Next = Prev;
            Prev = Curr;
            Curr = Tmp;
        }

        PrevGroupTail->Next = Kth;
        PrevGroupTail = GroupHead;
    }
    return Dummy.Next;
}

// LeetCode 21 - Merge two sorted lists
template<typename T>
ListNode<T>* MergeSorted(ListNode<T>* A, ListNode<T>* B)
{
    ListNode<T> Dummy;
    ListNode<T>* Tail = &Dummy;

    while (A && B)
    {
        if (A->Value < B->Value)
        {
            Tail->Next = A;
            A = A->Next;
        }
        else
        {
            Tail->Next = B;
            B = B->Next;
        }
        Tail = Tail->Next;
    }
    Tail->Next = A ? A : B;
    return Dummy.Next;
}

// LeetCode 23 - Merge k sorted lists
template<typename T>
ListNode<T>* MergeKSorted(std::vector<ListNode<T>*>& Lists)
{
    auto Cmp = [](ListNode<T>* A, ListNode<T>* B)
    {
        return A->Value > B->Value;
    };

    std::priority_queue<ListNode<T>*, std::vector<ListNode<T>*>, decltype(Cmp)> MinHeap(Cmp);

    for (auto* Node : Lists)
    {
        if (Node)
        {
            MinHeap.push(Node);
        }
    }

    ListNode<T> Dummy;
    ListNode<T>* Tail = &Dummy;

    while (!MinHeap.empty())
    {
        ListNode<T>* Smallest = MinHeap.top();
        MinHeap.pop();
        Tail->Next = Smallest;
        Tail = Tail->Next;
        if (Smallest->Next)
        {
            MinHeap.push(Smallest->Next);
        }
    }
    return Dummy.Next;
}

// LeetCode 148 - Sort list (merge sort)
template<typename T>
ListNode<T>* SortList(ListNode<T>* Head)
{
    if (!Head || !Head->Next)
    {
        return Head;
    }

    ListNode<T>* Slow = Head;
    ListNode<T>* Fast = Head;
    ListNode<T>* Prev = nullptr;

    while (Fast && Fast->Next)
    {
        Prev = Slow;
        Slow = Slow->Next;
        Fast = Fast->Next->Next;
    }
    Prev->Next = nullptr;

    return MergeSorted(SortList(Head), SortList(Slow));
}

// LeetCode 82 - Remove all nodes that have duplicate values
template<typename T>
ListNode<T>* RemoveAllDuplicates(ListNode<T>* Head)
{
    ListNode<T> Dummy;
    Dummy.Next = Head;
    ListNode<T>* Prev = &Dummy;

    while (Prev->Next)
    {
        ListNode<T>* Curr = Prev->Next;
        while (Curr->Next && Curr->Next->Value == Curr->Value)
        {
            ListNode<T>* Dup = Curr->Next;
            Curr->Next = Dup->Next;
            delete Dup;
        }
        if (Curr != Prev->Next)
        {
            // Curr itself is also a duplicate
            Prev->Next = Curr->Next;
            delete Curr;
        }
        else
        {
            Prev = Prev->Next;
        }
    }
    return Dummy.Next;
}

// LeetCode 83 - Remove duplicates (keep one)
template<typename T>
ListNode<T>* RemoveDuplicates(ListNode<T>* Head)
{
    ListNode<T>* Curr = Head;
    while (Curr && Curr->Next)
    {
        if (Curr->Value == Curr->Next->Value)
        {
            ListNode<T>* Dup = Curr->Next;
            Curr->Next = Dup->Next;
            delete Dup;
        }
        else
        {
            Curr = Curr->Next;
        }
    }
    return Head;
}

// LeetCode 203 - Remove all nodes with target value
template<typename T>
ListNode<T>* RemoveElements(ListNode<T>* Head, T Target)
{
    ListNode<T> Dummy;
    Dummy.Next = Head;
    ListNode<T>* Prev = &Dummy;

    while (Prev->Next)
    {
        if (Prev->Next->Value == Target)
        {
            ListNode<T>* ToDelete = Prev->Next;
            Prev->Next = ToDelete->Next;
            delete ToDelete;
        }
        else
        {
            Prev = Prev->Next;
        }
    }
    return Dummy.Next;
}

// LeetCode 19 - Remove nth node from end
template<typename T>
ListNode<T>* RemoveNthFromEnd(ListNode<T>* Head, int N)
{
    ListNode<T> Dummy;
    Dummy.Next = Head;
    ListNode<T>* Fast = &Dummy;
    ListNode<T>* Slow = &Dummy;

    for (int i = 0; i <= N; ++i)
    {
        if (!Fast)
        {
            return Head;
        }
        Fast = Fast->Next;
    }

    while (Fast)
    {
        Slow = Slow->Next;
        Fast = Fast->Next;
    }

    ListNode<T>* ToDelete = Slow->Next;
    Slow->Next = ToDelete->Next;
    delete ToDelete;
    return Dummy.Next;
}

// LeetCode 876 - Middle of the linked list
template<typename T>
ListNode<T>* GetMiddle(ListNode<T>* Head)
{
    ListNode<T>* Slow = Head;
    ListNode<T>* Fast = Head;
    while (Fast && Fast->Next)
    {
        Slow = Slow->Next;
        Fast = Fast->Next->Next;
    }
    return Slow;
}

// LeetCode 141 - Linked list cycle
template<typename T>
bool HasCycle(ListNode<T>* Head)
{
    ListNode<T>* Slow = Head;
    ListNode<T>* Fast = Head;
    while (Fast && Fast->Next)
    {
        Slow = Slow->Next;
        Fast = Fast->Next->Next;
        if (Slow == Fast)
        {
            return true;
        }
    }
    return false;
}

// LeetCode 142 - Linked list cycle entry index
template<typename T>
std::optional<int> GetCycleEntryIndex(ListNode<T>* Head)
{
    ListNode<T>* Slow = Head;
    ListNode<T>* Fast = Head;
    ListNode<T>* Meeting = nullptr;

    while (Fast && Fast->Next)
    {
        Slow = Slow->Next;
        Fast = Fast->Next->Next;
        if (Slow == Fast)
        {
            Meeting = Slow;
            break;
        }
    }

    if (!Meeting)
    {
        return std::nullopt;
    }

    ListNode<T>* Entry = Head;
    while (Entry != Meeting)
    {
        Entry = Entry->Next;
        Meeting = Meeting->Next;
    }

    int Index = 0;
    for (auto* Curr = Head; Curr != Entry; Curr = Curr->Next)
    {
        ++Index;
    }
    return Index;
}

// LeetCode 160 - Intersection of two linked lists
template<typename T>
ListNode<T>* GetIntersection(ListNode<T>* A, ListNode<T>* B)
{
    if (!A || !B)
    {
        return nullptr;
    }

    ListNode<T>* PA = A;
    ListNode<T>* PB = B;
    while (PA != PB)
    {
        PA = PA ? PA->Next : B;
        PB = PB ? PB->Next : A;
    }
    return PA;
}

// LeetCode 328 - Odd even linked list
template<typename T>
ListNode<T>* OddEvenList(ListNode<T>* Head)
{
    ListNode<T> DummyOdd;
    ListNode<T> DummyEven;
    ListNode<T>* TailOdd = &DummyOdd;
    ListNode<T>* TailEven = &DummyEven;

    bool bOdd = true;
    while (Head)
    {
        if (bOdd)
        {
            TailOdd->Next = Head;
            TailOdd = TailOdd->Next;
        }
        else
        {
            TailEven->Next = Head;
            TailEven = TailEven->Next;
        }
        Head = Head->Next;
        bOdd = !bOdd;
    }

    TailEven->Next = nullptr;
    TailOdd->Next = DummyEven.Next;
    return DummyOdd.Next;
}

// LeetCode 725 - Split linked list in parts
template<typename T>
std::vector<ListNode<T>*> SplitListToParts(ListNode<T>* Head, int K)
{
    int N = Size(Head);
    int BaseLen = N / K;
    int Extra = N % K;

    std::vector<ListNode<T>*> Result(K, nullptr);
    ListNode<T>* Curr = Head;

    for (int i = 0; i < K && Curr; ++i)
    {
        int PartSize = BaseLen + (i < Extra ? 1 : 0);
        Result[i] = Curr;
        for (int j = 1; j < PartSize; ++j)
        {
            Curr = Curr->Next;
        }
        ListNode<T>* NextPart = Curr->Next;
        Curr->Next = nullptr;
        Curr = NextPart;
    }
    return Result;
}

// LeetCode 61 - Rotate list
template<typename T>
ListNode<T>* RotateRight(ListNode<T>* Head, int K)
{
    if (!Head || !Head->Next || K == 0)
    {
        return Head;
    }

    int N = 1;
    ListNode<T>* Tail = Head;
    while (Tail->Next)
    {
        Tail = Tail->Next;
        ++N;
    }

    K %= N;
    if (K == 0)
    {
        return Head;
    }

    Tail->Next = Head;

    ListNode<T>* NewTail = Head;
    for (int i = 0; i < N - K - 1; ++i)
    {
        NewTail = NewTail->Next;
    }

    ListNode<T>* NewHead = NewTail->Next;
    NewTail->Next = nullptr;
    return NewHead;
}

// --- Demo functions ---

void Demo_Reverse()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    List = Reverse(List);
    Print("Reversed", List);
    FreeList(List);
}

void Demo_ReverseKGroup()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    List = ReverseKGroup(List, 2);
    Print("K=2", List);
    FreeList(List);
}

void Demo_MergeSorted()
{
    auto* A = MakeList({1, 2, 4});
    auto* B = MakeList({1, 3, 4});
    Print("A", A);
    Print("B", B);
    auto* Merged = MergeSorted(A, B);
    Print("Merged", Merged);
    FreeList(Merged);
}

void Demo_MergeKSorted()
{
    auto* A = MakeList({1, 4, 5});
    auto* B = MakeList({1, 3, 4});
    auto* C = MakeList({2, 6});
    std::vector<ListNode<int>*> Lists = {A, B, C};
    auto* Merged = MergeKSorted(Lists);
    Print("K-Merged", Merged);
    FreeList(Merged);
}

void Demo_SortList()
{
    auto* List = MakeList({4, 2, 1, 3});
    Print("Input", List);
    List = SortList(List);
    Print("Sorted", List);
    FreeList(List);
}

void Demo_RemoveAllDuplicates()
{
    auto* List = MakeList({1, 2, 3, 3, 4, 4, 5});
    Print("Input", List);
    List = RemoveAllDuplicates(List);
    Print("No82", List);
    FreeList(List);
}

void Demo_RemoveDuplicates()
{
    auto* List = MakeList({1, 1, 2, 3, 3});
    Print("Input", List);
    List = RemoveDuplicates(List);
    Print("No83", List);
    FreeList(List);
}

void Demo_RemoveElements()
{
    auto* List = MakeList({1, 2, 6, 3, 4, 5, 6});
    Print("Input", List);
    List = RemoveElements(List, 6);
    Print("No203", List);
    FreeList(List);
}

void Demo_RemoveNthFromEnd()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    List = RemoveNthFromEnd(List, 2);
    Print("No19", List);
    FreeList(List);
}

void Demo_GetMiddle()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    auto* Mid = GetMiddle(List);
    std::cout << "Middle: " << Mid->Value << "\n";
    FreeList(List);
}

void Demo_Cycle()
{
    auto* List = MakeLoopedList(8);
    std::cout << "HasCycle: " << (HasCycle(List) ? "true" : "false") << "\n";
    auto Entry = GetCycleEntryIndex(List);
    std::cout << "CycleEntry: " << (Entry ? std::to_string(*Entry) : "none") << "\n";
    FreeList(List);
}

void Demo_OddEvenList()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    List = OddEvenList(List);
    Print("No328", List);
    FreeList(List);
}

void Demo_SplitListToParts()
{
    auto* List = MakeList({1, 2, 3, 4, 5, 6, 7});
    Print("Input", List);
    auto Parts = SplitListToParts(List, 3);
    for (int i = 0; i < static_cast<int>(Parts.size()); ++i)
    {
        std::cout << "  Part " << i << ": ";
        Print(Parts[i]);
        FreeList(Parts[i]);
    }
}

void Demo_RotateRight()
{
    auto* List = MakeList({1, 2, 3, 4, 5});
    Print("Input", List);
    List = RotateRight(List, 2);
    Print("No61", List);
    FreeList(List);
}

int main()
{
    struct DemoEntry
    {
        const char* Name;
        void (*Fn)();
    };

    DemoEntry Demos[] =
    {
        {"Reverse (206)",           Demo_Reverse},
        {"ReverseKGroup (25)",      Demo_ReverseKGroup},
        {"MergeSorted (21)",        Demo_MergeSorted},
        {"MergeKSorted (23)",       Demo_MergeKSorted},
        {"SortList (148)",          Demo_SortList},
        {"RemoveAllDuplicates (82)",Demo_RemoveAllDuplicates},
        {"RemoveDuplicates (83)",   Demo_RemoveDuplicates},
        {"RemoveElements (203)",    Demo_RemoveElements},
        {"RemoveNthFromEnd (19)",   Demo_RemoveNthFromEnd},
        {"GetMiddle (876)",         Demo_GetMiddle},
        {"Cycle (141/142)",         Demo_Cycle},
        {"OddEvenList (328)",       Demo_OddEvenList},
        {"SplitListToParts (725)",  Demo_SplitListToParts},
        {"RotateRight (61)",        Demo_RotateRight},
    };

    for (auto& [Name, Fn] : Demos)
    {
        std::cout << "=== " << Name << " ===\n";
        Fn();
        std::cout << "\n";
    }

    return 0;
}
