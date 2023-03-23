#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <numeric>

#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
<< '\n'; exit(EXIT_FAILURE); }

typedef void* (MapReduceFunc)(void *);

struct Param
{
    std::vector<int>::const_iterator it;
    int count_elements;
    long long res = 0;
};

long long res = 0;

pthread_spinlock_t lock;

void* Map(void* arg)
{
    Param* param = (Param *)arg;

    for(auto it = param->it; it < param->it + param->count_elements; ++it)
    {
        param->res += *it;
    }
}

void* Reduce(void* arg)
{
    int err;

    err = pthread_spin_lock(&lock);
    if(err != 0)
        err_exit(err, "Cannot lock mutex");

    res += *(long long *)arg;

    err = pthread_spin_unlock(&lock);
    if(err != 0)
        err_exit(err, "Cannot lock mutex");
}

void MapReduce(const std::vector<int>& input, MapReduceFunc map, MapReduceFunc reduce, int count_threads)
{
    int err;

    err = pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");

    int count_elements = input.size() / count_threads;

    Param params[count_threads];
    pthread_t threads[count_threads];

    for(int i = 0; i < count_threads; ++i)
    {
        params[i].count_elements = count_elements;
        params[i].it = input.begin() + (count_elements * i);

        if(i == count_threads - 1)
        {
            params[i].count_elements += input.size() % count_threads;
        }

        err = pthread_create(&threads[i], NULL, map, (void *)&params[i]);

        if(err != 0)
        {
            err_exit(err, "Cannot create thread");
        }
    }

    for(int i = 0; i < count_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    for(int i = 0; i < count_threads; ++i)
    {
        err = pthread_create(&threads[i], NULL, reduce, (void *)&(params[i].res));

        if(err != 0)
        {
            err_exit(err, "Cannot create thread");
        }
    }

    for(int i = 0; i < count_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }
}

int main()
{
    std::vector<int> input;
    int size;
    std::cout << "Enter input array size: ";
    std::cin >> size;

    for(int i = 0; i < size; ++i)
    {
        input.push_back(i + 1);
    }

    std::cout << '\n';

    int count_threads;
    std::cout << "Enter count threads: ";
    std::cin >> count_threads;

    auto begin = std::chrono::steady_clock::now();

    MapReduce(input, Map, Reduce, count_threads);

    auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin);

    std::cout << "MapReduce res: " << res << '\n';
    std::cout << "Time: " << elapsed_time.count() << "\n\n";

    begin = std::chrono::steady_clock::now();

    long long sum = std::accumulate(input.begin(), input.end(), (long long)0);

    elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin);

    std::cout << "Std res: " << sum << '\n';
    std::cout << "Time: " << elapsed_time.count() << '\n';
}