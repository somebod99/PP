#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <chrono>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }

const int TASKS_COUNT = 10;

int task_list[TASKS_COUNT]; // Массив заданий

int current_task = 0; // Указатель на текущее задание

pthread_mutex_t mutex; // Мьютекс

pthread_spinlock_t lock; // Спинлок

void do_task(int task_no)
{
    int s;
    for(int i; i < task_no * 1000000; i++)
    {
        s += i;
        if(s%2 == 0)
        {
            s = i;
        }
    }
}

void *thread_job_mutex(void *arg)
{
    int task_no;
    int err;

    // Перебираем в цикле доступные задания
    while(true) 
    {
        // Захватываем мьютекс для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        
        auto begin = chrono::steady_clock::now();

        err = pthread_mutex_lock(&mutex);
        if(err != 0)
            err_exit(err, "Cannot lock mutex");

        // Запоминаем номер текущего задания, которое будем исполнять
        task_no = current_task;
        // Сдвигаем указатель текущего задания на следующее
        current_task++;
        // Освобождаем мьютекс
        err = pthread_mutex_unlock(&mutex);

        auto end = chrono::steady_clock::now();
        auto elapsed_time = chrono::duration_cast<std::chrono::nanoseconds>(end - begin);

        if(err != 0)
            err_exit(err, "Cannot unlock mutex");
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(task_no < TASKS_COUNT)
            {
                cout << "Время работы мьютекса: " << elapsed_time.count() << '\n';
                cout << "Поток с идентификатором " << pthread_self() <<" начинает выполнение задания №" << task_no+1 << "\n";
                do_task(task_no);
            }
        else
            return NULL;
    }
}

void *thread_job_spin(void *arg)
{
    int task_no;
    int err;

    // Перебираем в цикле доступные задания
    while(true) 
    {
        // Захватываем мьютекс для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        
        auto begin = chrono::steady_clock::now();

        err = pthread_spin_lock(&lock);
        if(err != 0)
            err_exit(err, "Cannot lock mutex");

        // Запоминаем номер текущего задания, которое будем исполнять
        task_no = current_task;
        // Сдвигаем указатель текущего задания на следующее
        current_task++;
        // Освобождаем мьютекс
        err = pthread_spin_unlock(&lock);

        auto end = chrono::steady_clock::now();
        auto elapsed_time = chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        if(err != 0)
            err_exit(err, "Cannot unlock mutex");
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(task_no < TASKS_COUNT)
        {
            cout << "Время работы спинлока: " << elapsed_time.count() << '\n';
            cout << "Поток с идентификатором " << pthread_self() <<" начинает выполнение задания №" << task_no+1 << "\n";
            do_task(task_no);
        }
        else
            return NULL;
    }
}

int main()
{
    int count = 1000;
    pthread_t threads[count]; // Идентификаторы потоков
    int err;                  // Код ошибки

    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i)
        task_list[i] = rand() % TASKS_COUNT;

    // Инициализируем мьютекс
    err = pthread_mutex_init(&mutex, NULL);
    err = pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

    if(err != 0)
        err_exit(err, "Cannot initialize mutex");

    // Создаём потоки
    for(int i = 0; i < count; ++i)
    {
        err = pthread_create(&threads[i], NULL, thread_job_spin, NULL);

        if(err != 0)
            err_exit(err, "Cannot create thread 1");
    }

    for(int i = 0; i < count; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    // Освобождаем ресурсы, связанные с мьютексом
    pthread_spin_destroy(&lock);
    pthread_mutex_destroy(&mutex);
} 