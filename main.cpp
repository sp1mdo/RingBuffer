#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "RingBuffer.hpp"

static constexpr uint64_t kElements{1000};
uint64_t g_sum{0};
size_t g_NumberOfCompleted{0};
std::mutex g_sumMutex;

RingBuffer<int, 100> *g_buffer{nullptr};
RingBuffer<int, 100> global_g_buffer(MemoryType::Stack);
size_t numberOfWorkers{0};
uint64_t receivedSum{0};

void worker(int id)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < kElements; i++)
    {
        int number{static_cast<int>(i)};
        g_buffer->push_back(number);
        sum = sum + number;
    }

    {
        std::lock_guard<std::mutex> mlock(g_sumMutex);
        g_sum = g_sum + sum;
        g_NumberOfCompleted++;
    }
    // printf("Im done %zu\n", g_NumberOfCompleted);
    //g_buffer->m_popCV.notify_one();
}

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
    int stack[kElements]; // Storage for the FIFO
    // RingBuffer<int, 1000> local_g_buffer(stack);
    // g_buffer = &local_g_buffer; // use for stack allocated fifo
    g_buffer = &global_g_buffer; // use for heap allocated fifo

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
        //int num = g_buffer->pull_front();
        int num = g_buffer->move_and_pop();
        receivedSum = receivedSum + num;
    }

    return 0;
}