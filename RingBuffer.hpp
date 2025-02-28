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
            std::cout << "Using heap for storage\n" ;
            memoryType_ = MemoryType::Heap;
            data_ptr_ = std::make_unique<value_type[]>(N);
            heap_ = data_ptr_.get();
            data_ = heap_;
        }
    };

    RingBuffer(T *ptr) : front_(0), back_(0), heap_(nullptr)
    {
        data_ = ptr;
    };

    ~RingBuffer()
    {
    }

    void dump()
    {
        for (size_t i = 0; i < N; i++)
            std::cout << data_[i] << ", ";

        std::cout << "\n";
    }

    value_type front() const
    {
        std::unique_lock<std::mutex> mlock(m_popMutex);
        return data_[front_ % N];
    }

    value_type back() const
    {
        std::unique_lock<std::mutex> mlock(m_popMutex);
        return data_[back_ % N];
    }

    value_type move_and_pop()
    {
        value_type ret;
        {
            std::unique_lock<std::mutex> mlock(m_popMutex);
            while (empty())
            {
                m_popCV.wait(mlock);
            }

            ret = std::move(front());
            pop_front();
        }
        m_pushCV.notify_one();

        return ret;
    }

    value_type pull_front()
    {
        value_type ret;
        {
            std::unique_lock<std::mutex> mlock(m_popMutex);
            ret = data_[front_ % N];
            front_ = (front_ + 1) % N; // pop front
        }
        m_pushCV.notify_one();
        return ret;
    }

    void push_back(const T &other)
    {
        std::unique_lock<std::mutex> mlock(m_pushMutex);
        while (full())
        {
            m_pushCV.wait(mlock);
        }

        data_[back_ % N] = other;
        back_ = (back_ + 1) % N;
        
        m_popCV.notify_one();
    }

    void push_back(T &&other)
    {
        {
            std::unique_lock<std::mutex> mlock(m_pushMutex);
            while (full())
            {
                m_pushCV.wait(mlock);
            }
            data_[back_ % N] = std::move(other);
            back_ = (back_ + 1) % N;
        }
        m_popCV.notify_one();
    }

    void pop_front(void)
    {
        {
            std::unique_lock<std::mutex> mlock(m_popMutex);
            front_ = (front_ + 1) % N;
        }
        m_pushCV.notify_one();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> mlock(m_popMutex);
        return front_ == back_;
    }

    bool full() const
    {
        bool rbool;
        std::unique_lock<std::mutex> mlock(m_popMutex);

        if (front_ > 0)
            rbool = (back_ == (front_ - 1)) ? true : false;
        else
            rbool = (back_ == (N - 1)) ? true : false;

        return rbool;
    }

    void clear()
    {
        {
        std::unique_lock<std::mutex> mlock(m_popMutex);
        front_ = 0;
        back_ = 0;
        }
        m_pushCV.notify_one();
    }

    size_type size() const
    {
        std::unique_lock<std::mutex> mlock(m_popMutex);
        return back_ - front_;
    }

private:
    mutable std::mutex m_popMutex;
    mutable std::mutex m_pushMutex;
    std::condition_variable m_popCV;
    std::condition_variable m_pushCV;
    MemoryType memoryType_;
    size_t front_;
    size_t back_;
    value_type stack_[N];
    std::unique_ptr<value_type[]> data_ptr_;
    value_type *heap_;
    value_type *data_;
};