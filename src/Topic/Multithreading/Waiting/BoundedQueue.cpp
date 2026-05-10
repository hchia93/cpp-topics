#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <string>
#include <cassert>

// Producer-consumer with a bounded queue.
// Demonstrates: mutex, lock_guard, unique_lock, condition_variable,
// atomic, thread, and graceful shutdown.

template<typename T>
class TBoundedQueue
{
public:
    explicit TBoundedQueue(int Capacity) : Capacity(Capacity) {}

    // Block until space is available or stopped.
    bool Push(T Item)
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        NotFull.wait(Lock, [this] { return Queue.size() < Capacity || bStopped; });

        if (bStopped)
        {
            return false;
        }

        Queue.push(std::move(Item));
        NotEmpty.notify_one();
        return true;
    }

    // Block until an item is available or stopped and empty.
    bool Pop(T& Out)
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        NotEmpty.wait(Lock, [this] { return !Queue.empty() || bStopped; });

        if (Queue.empty())
        {
            return false;
        }

        Out = std::move(Queue.front());
        Queue.pop();
        NotFull.notify_one();
        return true;
    }

    // Signal all waiting threads to wake up and exit.
    void Stop()
    {
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            bStopped = true;
        }
        NotEmpty.notify_all();
        NotFull.notify_all();
    }

    int Size()
    {
        std::lock_guard<std::mutex> Lock(Mutex);
        return static_cast<int>(Queue.size());
    }

private:
    std::queue<T> Queue;
    std::mutex Mutex;
    std::condition_variable NotEmpty;
    std::condition_variable NotFull;
    size_t Capacity;
    bool bStopped = false;
};

int main()
{
    TBoundedQueue<std::string> Queue(4);
    std::atomic<int> Produced{0};
    std::atomic<int> Consumed{0};

    // Producer: pushes items until stopped
    auto ProducerFn = [&](std::string Name, int Count)
    {
        for (int i = 0; i < Count; ++i)
        {
            std::string Item = Name + "-" + std::to_string(i);
            if (!Queue.Push(Item))
            {
                break;
            }
            ++Produced;
        }
    };

    // Consumer: pops items until stopped and empty
    auto ConsumerFn = [&](std::string Name)
    {
        std::string Item;
        while (Queue.Pop(Item))
        {
            std::cout << Name << " consumed: " << Item << "\n";
            ++Consumed;
        }
    };

    // Two producers, one consumer
    std::thread P1(ProducerFn, "P1", 5);
    std::thread P2(ProducerFn, "P2", 5);
    std::thread C1(ConsumerFn, "C1");

    P1.join();
    P2.join();

    // All producers done, signal consumer to drain and exit
    Queue.Stop();
    C1.join();

    assert(Produced == 10);
    assert(Consumed == 10);
    std::cout << "Produced: " << Produced << ", Consumed: " << Consumed << "\n";
    std::cout << "All tests passed.\n";

    return 0;
}
