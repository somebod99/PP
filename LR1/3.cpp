#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <pthread.h>
#include <chrono>

using namespace std;

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
    int* p = (int *)arg;
    double sum = 0.0;

    auto begin = chrono::steady_clock::now();

    for(int i = 0; i < *p; ++i)
    {
        sum += 0.1;
    }

    auto end = chrono::steady_clock::now();
    auto elapsed_time = chrono::duration_cast<std::chrono::microseconds>(end - begin);
    *p = elapsed_time.count();
}

int main(int argc, char* argv[])
{
    // Определяем переменные: идентификатор потока и код ошибки
    pthread_t thread;
    int err;

    int n;

    for(int i = 100; i <= 100000000; i *= 10)
    {
        n = i;
        auto begin = chrono::steady_clock::now();
        // Создаём поток
        err = pthread_create(&thread, NULL, thread_job, (void*)&n);
        auto end = chrono::steady_clock::now();

        auto elapsed_time = chrono::duration_cast<std::chrono::microseconds>(end - begin);

        err = pthread_join(thread, NULL);
        if(err)
        {
            cout << "Cannot join a thread: " << strerror(err) << '\n';
            exit(-1);
        }

        cout << i << ": " << elapsed_time.count() << '\t' << n << '\n';

        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        if(err)
        {
            cout << "Cannot create a thread: " << strerror(err) << endl;
            exit(-1);
        }
    }
    
    // Ожидаем завершения созданного потока перед завершением
    // работы программы
    pthread_exit(NULL);
}