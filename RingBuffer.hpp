#include <mutex>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <memory>

enum class StorageType
{
    Static,
    Dynamic
};

template <typename T, std::size_t N, StorageType S>
class RingBuffer
{
public:
    using value_type = T;
    using size_type = std::size_t;
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

    // Return element value at front of the queue
    value_type front() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return data_[front_ % N];
    }

    // Return element value at back of the queue
    value_type back() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return data_[back_ % N];
    }

    // Returns the element at front of the queue and pops the element usinng moving constructor 
    // If it's applicable.
    // This is a blocking call, so if queue is empty
    // it will wait until there is something in the queue
    value_type move_and_pop()
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

    // Returns the element at front of the queue and pops the element
    // This is a blocking call, so if queue is empty
    // it will wait until there is something in the queue
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

    // Put the element at the back of the queue
    // This is a blocking call, so if queue is full
    // it will wait until there is a free space to push the element
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

    // Emplace the element at the back of the queue
    // This is a blocking call, so if queue is full
    // it will wait until there is a free space to emplace the element
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

    // Pops the element from the front of the queue
    void pop_front(void)
    {
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            front_ = (front_ + 1) % N;
        }
        pushCV_.notify_one();
    }

    // Returns 'true' if the queue is empty
    bool empty() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return front_ == back_;
    }

    // Returns 'true' if the queue is full
    bool full() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return full_no_mutex();
    }


    // Clear all elements in the queue
    void clear()
    {
        {
            std::unique_lock<std::mutex> mlock(popMutex_);
            front_ = 0;
            back_ = 0;
        }
        pushCV_.notify_one();
    }

    // Returns the number of currently present elements in the queue
    size_type size() const
    {
        std::unique_lock<std::mutex> mlock(popMutex_);
        return back_ - front_;
    }

private:
    bool full_no_mutex() const
    {
        bool rbool;
        if (front_ > 0)
            rbool = (back_ == (front_ - 1)) ? true : false;
        else
            rbool = (back_ == (N - 1)) ? true : false;

        return rbool;
    }

    mutable std::mutex popMutex_;
    mutable std::mutex pushMutex_;
    std::condition_variable popCV_;
    std::condition_variable pushCV_;
    size_t front_;
    size_t back_;
    StorageContainer storage_;
    value_type *data_;
};