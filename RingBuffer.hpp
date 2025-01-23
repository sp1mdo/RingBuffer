#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>

enum class MemoryType
{
    Stack = 0,
    Heap,
};

template <typename T, std::size_t N>
class RingBuffer
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = value_type *;
    using const_iterator = const value_type *;

    RingBuffer(MemoryType type = MemoryType::Stack) : front_(0), back_(0), heap_(nullptr)
    {
        if (type == MemoryType::Stack)
        {
            data_ = stack_;
            memoryType_ = MemoryType::Stack;
        }
        else
        {
            std::cout << "Using heap for storage" << std::endl;
            memoryType_ = MemoryType::Heap;
            heap_ = new value_type[N];
            data_ = heap_;
        }
    };

    ~RingBuffer()
    {
        if (memoryType_ == MemoryType::Heap)
        {
            std::cout << "Releasing memory" << std::endl;
            delete[] heap_;
            heap_ = nullptr;
            data_ = nullptr;
        }
    }

    constexpr size_t size(T (&)[N])
    {
        return N;
    }

    void dump()
    {
        for (size_t i = 0; i < N; i++)
            std::cout << data_[i] << ", ";

        std::cout << std::endl;
    }

    value_type front() const
    {
        std::lock_guard<std::mutex> mlock(mutex_);
        return data_[front_ % N];
    }

    value_type back() const
    {
        std::lock_guard<std::mutex> mlock(mutex_);
        return data_[back_ % N];
    }

    value_type move_and_pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (empty())
        {
            cond_.wait(mlock);
        }

        // auto ret = std::move(data_.front());
        auto ret = front();
        pop_front();
        return ret;
    }

    value_type pull_front()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        auto ret = data_[front_ % N];
        front_++; // pop front
        return ret;
    }

    void push_back(const T &other)
    {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            data_[back_ % N] = other;
            back_++;
        }
        cond_.notify_one();
    }

    void push_back(T &&other)
    {
        {
            std::unique_lock<std::mutex> mlock(mutex_);
            data_[back_ % N] = std::move(other);
            back_++;
        }
        cond_.notify_one();
    }

    void pop_front(void)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        front_++;
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return front_ == back_;
    }

    bool full() const
    {
        bool rbool;
        std::unique_lock<std::mutex> mlock(mutex_);
        // return (front_ - 1 == back_);

        if (front_ > 0)
            rbool = (back_ == (front_ - 1)) ? true : false;
        else
            rbool = (back_ == (N - 1)) ? true : false;

        return rbool;
    }
    /*
    void lock()
    {
        mutex_.lock();
    }

    void unlock()
    {
        mutex_.unlock();
    }
*/
    void clear()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        front_ = 0;
        back_ = 0;
    }

    size_type size() const
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return back_ - front_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    MemoryType memoryType_;
    size_t front_;
    size_t back_;
    value_type stack_[N];
    value_type *heap_;
    value_type *data_;
};