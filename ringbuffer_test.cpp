#include <iostream>
#include <thread>
#include <vector>
#include "RingBuffer.hpp"

static constexpr uint64_t kElements{1000};
uint64_t g_sum{0};
size_t g_NumberOfCompleted{0};
std::mutex g_sumMutex;

RingBuffer<int, 100, StorageType::Static> my_Fifo;
size_t numberOfWorkers{0};
uint64_t receivedSum{0};

void worker(int id)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < kElements; i++)
    {
        int number{static_cast<int>(i)};
        my_Fifo.push_back(number);
        sum = sum + number;
    }

    {
        std::lock_guard<std::mutex> mlock(g_sumMutex);
        g_sum = g_sum + sum;
        g_NumberOfCompleted++;
    }
}

// Observer thread has to interrupt the test, in case it is completed, because 
// as per design pull() method is locked on condition variable, expecting
// that forever there will be some data to be read from the queue
void observer(void)
{
    std::cout << "Observer thread started\n";
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::lock_guard<std::mutex> mlock(g_sumMutex);
            if (g_NumberOfCompleted == numberOfWorkers)
            {
                if (g_sum == receivedSum)
                    std::cout << "Test passed!! \n";
                else
                    std::cout << "Sent total: " << g_sum << " . Received : " << receivedSum << "\n";

                std::abort();
            }
        }
    }
}

int main(int argc, char **argv)
{
    numberOfWorkers = static_cast<size_t>(std::stoi(argv[1]));

    std::thread([]()
                { observer(); })
        .detach();

    for (size_t i = 0; i < numberOfWorkers; i++)
    {
        std::thread([i]()
                    { worker(i); })
            .detach();
    }

    while (1)
    {
        int num = my_Fifo.pull_front();
        receivedSum = receivedSum + num;
    }

    return 0;
}
