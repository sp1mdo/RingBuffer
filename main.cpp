#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include "RingBuffer.hpp"

static constexpr uint64_t elements = 1000;
uint64_t g_sum = 0;
size_t number_of_completed = 0;
std::mutex sum_mutex;

RingBuffer<int, 1000> *g_buffer;

void worker(int id)
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < elements; i++)
    {
        int number = rand() % 1000;
        g_buffer->push_back(number);
        sum = sum + number;

        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

    sum_mutex.lock();
    g_sum = g_sum + sum;
    number_of_completed++;
    sum_mutex.unlock();
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    uint64_t received_sum = 0;
    uint32_t number_of_workers = atoi(argv[1]);

    int stack[elements]; // Storage for the FIFO

    RingBuffer<int, 1000> local_g_buffer(stack);
    g_buffer = &local_g_buffer;

    for (size_t i = 0; i < number_of_workers; i++)
    {
        std::thread([i]()
                    { worker(i); })
            .detach();
    }

    while (1)
    {
        if (g_buffer->empty() == false)
        {
            received_sum = received_sum + g_buffer->pull_front();
        }

        sum_mutex.lock();
        if (number_of_completed == number_of_workers)
        {
            sum_mutex.unlock();
            break;
        }
        sum_mutex.unlock();
    }

    if (g_sum == received_sum)
        std::cout << "Test passed!! " << std::endl;
    else
        std::cout << "Sent total: " << g_sum << " . Received : " << received_sum << std::endl;

    return 0;
}