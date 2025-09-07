
#include "ThreadPool.h"

#include <chrono>
#include <iostream>

void task(int value)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Task executed with value: " << value << std::endl;
}


int main()
{
    ThreadPool pool(4);

    pool.CommitTask(task, 1);

    std::cout << "Tasks committed." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
}