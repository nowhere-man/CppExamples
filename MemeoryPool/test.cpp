#include <chrono>
#include <iostream>
#include <vector>
#include <random>

#include "MemoryPool.h"

struct PktBuffer {
    char buf[1500];
    size_t bufLen;
};

const int TOTAL_OPERATIONS = 1000000;
const int PRE_ALLOC_COUNT = 1000;

void testWithMemoryPool()
{
    MemoryPool<PktBuffer, sizeof(PktBuffer) * 256, false> pool;
    std::vector<PktBuffer*> pkts(PRE_ALLOC_COUNT);

    for (int i = 0; i < PRE_ALLOC_COUNT; ++i) {
        pkts[i] = pool.NewElement();
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, PRE_ALLOC_COUNT - 1);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < TOTAL_OPERATIONS; ++i) {
        int idx = distrib(gen);
        pool.DeleteElement(pkts[idx]);
        pkts[idx] = pool.NewElement();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "MemoryPool took " << diff.count() << " seconds." << std::endl;

    for (int i = 0; i < PRE_ALLOC_COUNT; ++i) {
        pool.DeleteElement(pkts[i]);
    }
}

void testWithNewDelete()
{
    std::vector<PktBuffer*> pkts(PRE_ALLOC_COUNT);

    for (int i = 0; i < PRE_ALLOC_COUNT; ++i) {
        pkts[i] = new PktBuffer();
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, PRE_ALLOC_COUNT - 1);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < TOTAL_OPERATIONS; ++i) {
        int idx = distrib(gen);
        delete pkts[idx];
        pkts[idx] = new PktBuffer();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "new/delete took " << diff.count() << " seconds." << std::endl;

    for (int i = 0; i < PRE_ALLOC_COUNT; ++i) {
        delete pkts[i];
    }
}

int main()
{
    std::cout << "Performing " << TOTAL_OPERATIONS << " interleaved allocations/deallocations..." << std::endl;
    testWithMemoryPool();
    testWithNewDelete();
    return 0;
}
