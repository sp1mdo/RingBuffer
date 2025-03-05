#include <mutex>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <memory>

enum class StorageType { Static, Dynamic };

template <typename T, std::size_t N, StorageType S>
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
    using StorageContainer = std::conditional_t<S == StorageType::Static, std::array<T, N>, std::unique_ptr<T[]>>;

    RingBuffer() : front_(0), back_(0)
    {
        if constexpr (S == StorageType::Dynamic) // use heap allocated array as a container
        {
            storage_ = std::make_unique<T[]>(N);
            data_ = storage_.get();
        }
        else // use std::array as a container
        {
            data_ = &storage_[0];
        }
    };

    ~RingBuffer()
    {
    }

    void dump() const
    {
        for (size_t i = 0; i < N; i++)
            std::cout << data_[i] << ", ";

        std::cout << "\n";
    }

    value_type front() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return data_[front_ % N];
    }

    value_type back() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return data_[back_ % N];
    }

    value_type move_and_pop() // use if  Typename has reasonable movable members.
    {
        value_type ret;
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            while (front_ == back_)
            {
                popCV_.wait(mlock);
            }

            ret = std::move(data_[front_ % N]);
            front_ = (front_ + 1) % N; // pop front
        }
        pushCV_.notify_one();

        return ret;
    }

    value_type pull_front()
    {
        value_type ret;
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            while (front_ == back_)
            {
                popCV_.wait(mlock);
            }
            ret = data_[front_ % N];
            front_ = (front_ + 1) % N; // pop front
        }
        pushCV_.notify_one();
        return ret;
    }

    void push_back(const value_type &other)
    {
        std::unique_lock<std::mutex> mlock(pushMutex_);
        {
            while (full_no_mutex())
            {
                pushCV_.wait(mlock);
            }

            data_[back_ % N] = other;
            back_ = (back_ + 1) % N;
        }
        popCV_.notify_one();
    }

    void push_back(T &&other)
    {
        {
            std::unique_lock<std::mutex> mlock(pushMutex_);
            while (full())
            {
                pushCV_.wait(mlock);
            }
            data_[back_ % N] = std::move(other);
            back_ = (back_ + 1) % N;
        }
        popCV_.notify_one();
    }

    void pop_front(void)
    {
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            front_ = (front_ + 1) % N;
        }
        pushCV_.notify_one();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return front_ == back_;
    }

    bool full_no_mutex() const
    {
        bool rbool;
        if (front_ > 0)
            rbool = (back_ == (front_ - 1)) ? true : false;
        else
            rbool = (back_ == (N - 1)) ? true : false;

        return rbool;
    }

    bool full() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return full_no_mutex();
    }

    void clear()
    {
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            front_ = 0;
            back_ = 0;
        }
        pushCV_.notify_one();
    }

    size_type size() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return back_ - front_;
    }

private:
    mutable std::mutex popMutex_;
    mutable std::mutex pushMutex_;
    std::condition_variable popCV_;
    std::condition_variable pushCV_;
    size_t front_;
    size_t back_;
    StorageContainer storage_;
    value_type *data_;
};