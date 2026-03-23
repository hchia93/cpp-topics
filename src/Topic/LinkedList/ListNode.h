#pragma once

#include <iostream>
#include <initializer_list>
#include <string_view>
#include <cstdlib>
#include <ctime>

template<typename T>
struct ListNode
{
    T Value{};
    ListNode* Next = nullptr;

    ListNode() = default;
    ListNode(T InValue) : Value(InValue) {}
    ListNode(T InValue, ListNode* Next) : Value(InValue), Next(Next) {}
};

namespace ListNodeUtil
{
    template<typename T>
    void Print(ListNode<T>* Head)
    {
        while (Head != nullptr)
        {
            std::cout << Head->Value << " -> ";
            Head = Head->Next;
        }
        std::cout << "null\n";
    }

    template<typename T>
    void Print(std::string_view Label, ListNode<T>* Head)
    {
        std::cout << Label << ": ";
        Print(Head);
    }

    template<typename T>
    int Size(ListNode<T>* Head)
    {
        int Count = 0;
        while (Head != nullptr)
        {
            ++Count;
            Head = Head->Next;
        }
        return Count;
    }

    // Break a cycle if one exists, then delete all nodes.
    template<typename T>
    void FreeList(ListNode<T>* Head)
    {
        if (!Head)
        {
            return;
        }

        // Floyd's cycle detection to break loop before freeing
        ListNode<T>* Slow = Head;
        ListNode<T>* Fast = Head;
        while (Fast && Fast->Next)
        {
            Slow = Slow->Next;
            Fast = Fast->Next->Next;
            if (Slow == Fast)
            {
                // Find entry
                ListNode<T>* Entry = Head;
                while (Entry != Slow)
                {
                    Entry = Entry->Next;
                    Slow = Slow->Next;
                }
                // Find tail of loop
                ListNode<T>* Tail = Entry;
                while (Tail->Next != Entry)
                {
                    Tail = Tail->Next;
                }
                Tail->Next = nullptr;
                break;
            }
        }

        while (Head != nullptr)
        {
            ListNode<T>* Next = Head->Next;
            delete Head;
            Head = Next;
        }
    }

    template<typename T>
    ListNode<T>* MakeList(std::initializer_list<T> Values)
    {
        if (Values.size() == 0)
        {
            return nullptr;
        }

        auto It = Values.begin();
        ListNode<T>* Head = new ListNode<T>(*It);
        ListNode<T>* Current = Head;
        ++It;

        for (; It != Values.end(); ++It)
        {
            Current->Next = new ListNode<T>(*It);
            Current = Current->Next;
        }
        return Head;
    }

    // Create a list [1..N] with a cycle pointing back to a random node.
    inline ListNode<int>* MakeLoopedList(int N)
    {
        ListNode<int>* Head = new ListNode<int>(1);
        ListNode<int>* Current = Head;
        ListNode<int>* LoopTarget = Head;

        srand(static_cast<unsigned>(time(nullptr)));
        int LoopIndex = rand() % N;

        for (int i = 1; i < N; ++i)
        {
            Current->Next = new ListNode<int>(i + 1);
            Current = Current->Next;
            if (i == LoopIndex)
            {
                LoopTarget = Current;
            }
        }
        Current->Next = LoopTarget;
        return Head;
    }
}
