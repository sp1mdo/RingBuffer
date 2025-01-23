#include <iostream>
#include <thread>
#include <vector>
#include <barrier>
#include <unistd.h>
#include "RingBuffer.hpp"

static constexpr uint64_t elements = 100;
uint64_t g_sum = 0;
uint32_t number_of_completed = 0;
std::mutex sum_mutex;

RingBuffer<int, 1000> buforek(MemoryType::Stack);

void worker(int id)
{
    // std::cout << "Worker " << id << " spawned" << std::endl;
    uint64_t sum = 0;
    for (uint64_t i = 0; i < elements; i++)
    {
        int number = rand() % 100;
        if (buforek.full() == false)
        {
            buforek.push_back(number);
            sum = sum + number;
        }
        usleep(100);
    }

    sum_mutex.lock();
    g_sum = g_sum + sum;
    number_of_completed++;
    sum_mutex.unlock();
    // std::cout << "Worker " << id << " ended with " << sum << std::endl;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    std::vector<std::thread> workers;
    uint64_t received_sum = 0;
    uint32_t number_of_workers = atoi(argv[1]);
    workers.reserve(number_of_workers);
    for (size_t i = 0; i < number_of_workers; i++)
    {

        workers.push_back(std::thread(worker, i));
        workers.back().detach();
    }

    while (1)
    {
        if (buforek.empty() == false)
        {
            received_sum = received_sum + buforek.pull_front();
            // std::cout << "rcvsum: " << received_sum << " \n";
        }
        {
            sum_mutex.lock();
            if (number_of_completed == number_of_workers)
            {
                sum_mutex.unlock();
                break;
            }
            sum_mutex.unlock();
        }
    }

    usleep(100000);
    if (g_sum == received_sum)
        std::cout << "Test passed!! " << std::endl;
    else
        std::cout << "Sent total: " << g_sum << " . Received : " << received_sum << std::endl;

    return 0;
}