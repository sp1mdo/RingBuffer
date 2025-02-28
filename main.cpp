#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include "RingBuffer.hpp"

static constexpr uint64_t kElements{1000};
uint64_t g_sum{0};
size_t g_NumberOfCompleted{0};
std::mutex g_sumMutex;

RingBuffer<int, 1000> *g_buffer{nullptr};
RingBuffer<int, 1000> global_g_buffer(MemoryType::Stack);

void worker(int id)
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < kElements; i++)
    {
        int number{rand() % 1000};
        g_buffer->push_back(number);
        sum = sum + number;

        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
    {
        std::lock_guard<std::mutex> mlock(g_sumMutex);
        g_sum = g_sum + sum;
        g_NumberOfCompleted++;
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    uint64_t receivedSum{0};
    size_t numberOfWorkers{static_cast<size_t>(std::stoi(argv[1]))};

    int stack[kElements]; // Storage for the FIFO
    // RingBuffer<int, 1000> local_g_buffer(stack);

    // g_buffer = &local_g_buffer; // use for stack allocated fifo
    g_buffer = &global_g_buffer; // use for heap allocated fifo

    for (size_t i = 0; i < numberOfWorkers; i++)
    {
        std::thread([i]()
                    { worker(i); })
            .detach();
    }

    while (1)
    {
        if (g_buffer->empty() == false) // TODO make pull based on Cond.var and notify
        {
            receivedSum = receivedSum + g_buffer->pull_front();
        }

        {
            std::lock_guard<std::mutex> mlock(g_sumMutex);
            if (g_NumberOfCompleted == numberOfWorkers)
            {
                g_sumMutex.unlock();
                break;
            }
        }
    }

    if (g_sum == receivedSum)
        std::cout << "Test passed!! \n" ;
    else
        std::cout << "Sent total: " << g_sum << " . Received : " << receivedSum << "\n";

    return 0;
}