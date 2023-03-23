#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }

const int TASKS_COUNT = 10;

int task_list[TASKS_COUNT]; // Массив заданий

int current_task = 0; // Указатель на текущее задание

void do_task(int task_no)
{
    double sum = 0.0;
    int t = 0;

    for(int i = 0; i < (task_no + 1) * 1000000; ++i)
    {
        t = rand() % 10 + 1;

        if(t < 2)
        {
            sum += (rand() % 10 + 1) / 10.0;
        }
        else if(t < 5)
        {
            sum += (rand() % 50 + 1) / 10.0;
        }
        else if(t < 8)
        {
            sum += (rand() % 100 + 1) / 10.0;
        }
    }
}

void *thread_job(void *arg)
{
    pthread_t thread = pthread_self();
    cout << "Thread " << thread << " starting\n";

    int task_no;
    int err = 0;

    // Перебираем в цикле доступные задания
    while(true) 
    {
        // Запоминаем номер текущего задания, которое будем исполнять
        task_no = current_task;
            
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(task_no < TASKS_COUNT)
        {
            // Вывод информации о том, какую задачу взял поток на исполнение
            cout << "Thread " << thread << " get task №" << task_no << '\n';
            do_task(task_no);
        }
        else
        {
            cout << "Thread " << thread << " finishing\n";
            return NULL;
        }

        // Сдвигаем указатель текущего задания на следующее
        current_task++; 

        sleep(rand() % 5 + 1);
    }
}

int main()
{
    srand(time(NULL));

    pthread_t thread1, thread2; // Идентификаторы потоков
    int err; // Код ошибки

    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i)
        task_list[i] = rand() % TASKS_COUNT;

    // Создаём потоки
    err = pthread_create(&thread1, NULL, thread_job, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");

    err = pthread_create(&thread2, NULL, thread_job, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}