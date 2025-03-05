#include <iostream>
#include <thread>
#include <vector>
#include "RingBuffer.hpp"

static constexpr uint64_t kElements{1000};
static constexpr uint64_t kQueueSize{100};

uint64_t g_sum{0};
size_t g_NumberOfCompleted{0};
std::mutex g_sumMutex;

RingBuffer<int, kQueueSize, StorageType::Static> my_Fifo;
size_t g_numberOfWorkers{0};
uint64_t g_receivedSum{0};

// Worker thread that is to push the numbers into the queue
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
            if (g_NumberOfCompleted == g_numberOfWorkers)
            {
                if (g_sum == g_receivedSum)
                    std::cout << "Test passed!! \n";
                else
                    std::cout << "Sent total: " << g_sum << " . Received : " << g_receivedSum << "\n";

                std::abort();
            }
        }
    }
}

int main(int argc, char **argv)
{
    // Get number of thread workers that will be pushing data into the queue
    g_numberOfWorkers = static_cast<size_t>(std::stoi(argv[1]));

    // Spawn observer thread that will interupt the process, once the test is finished
    std::thread([]()
                { observer(); })
        .detach();

    // Spawn this many worker threads
    for (size_t i = 0; i < g_numberOfWorkers; i++)
    {
        std::thread([i]()
                    { worker(i); })
            .detach();
    }

    // Perform the test
    while (1)
    {
        int num = my_Fifo.pull_front();
        g_receivedSum = g_receivedSum + num;
    }

    return 0;
}
