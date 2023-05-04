#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include "mpi.h"


using std::cout;

const int MAINPROC = 0;

const int LOWERPROC = 100;
const int UPPERPROC = 101;

const int X_LOCALSTART = 102;
const int X_LOCALEND = 103;

const int LOWERBOUND = 104;
const int UPPERBOUND = 105;

const double a = 10.0e5;

const double accuracy = 1.0e-10;

// Начальное приближение
const double start_phi = 0.0;

// Границы области
const double xStart = -1.0;
const double xEnd = 1.0;

const double yStart = -1.0;
const double yEnd = 1.0;

const double zStart = -1.0;
const double zEnd = 1.0;

// Размеры области
const double Dx = xEnd - xStart;
const double Dy = yEnd - yStart;
const double Dz = zEnd - zStart;

// Количество узлов сетки
const int Nx = 15;
const int Ny = 15;
const int Nz = 15;

// Размеры шага на сетке
const double hx = Dx / (Nx - 1);
const double hy = Dy / (Ny - 1);
const double hz = Dz / (Nz - 1);

double phi(double x, double y, double z)
{
    return pow(x, 2) + pow(y, 2) + pow(z, 2);
}

double rho(double phiValue)
{
    return 6.0 - a * phiValue;
}

// Перевод из координаты сетки в настоящее значение
double coordTrans(double start, double step, int bias)
{
    return start + step * bias;
}

void createGrid(double***& grid, int xLength, double xLocalStart)
{
    // Создаём массив и заполняем начальными значениями
    grid = new double** [xLength];

    for (int i = 0; i < xLength; i++)
    {
        grid[i] = new double* [Ny];

        for (int j = 0; j < Ny; j++)
        {
            grid[i][j] = new double[Nz];

            for (int k = 1; k < Nz - 1; k++)
            {
                grid[i][j][k] = start_phi;
            }
        }
    }

    // Записываем краевые значения
    double xCurr;

    for (int i = 0; i < xLength; i++)
    {
        xCurr = coordTrans(xLocalStart, hx, i);

        for (int k = 0; k < Nz; k++)
        {
            // При j = 0
            grid[i][0][k] = phi(xCurr, yStart, coordTrans(zStart, hz, k));
        }

        for (int k = 0; k < Nz; k++)
        {
            // При j = Ny - 1
            grid[i][Ny - 1][k] = phi(xCurr, yEnd, coordTrans(zStart, hz, k));
        }

        double yCurr;

        for (int j = 1; j < Ny - 1; j++)
        {
            yCurr = coordTrans(yStart, hy, j);

            // При k = 0
            grid[i][j][0] = phi(xCurr, yCurr, zStart);

            // При k = Nz - 1
            grid[i][j][Nz - 1] = phi(xCurr, yCurr, zEnd);
        }
    }
}

void freeMemory(double*** arr, int xLocalLength)
{
    for (int i = 0; i < xLocalLength; i++)
    {
        for (int j = 0; j < Ny; j++)
        {
            delete[] arr[i][j];
        }

        delete[] arr[i];
    }

    delete[] arr;
}

// Считаем точность, как максимальное значение модуля отклонения
// от истинного значения функции
double getAccuracyMPI(double*** grid, int xLocalLength, double xLocalStart, int rootProcRank)
{
    // Значение ошибки на некотором узле
    double currErr;

    // Максимальное значение ошибки данного процесса
    double maxLocalErr = 0.0;

    for (int i = 1; i < xLocalLength - 1; i++)
    {
        for (int j = 1; j < Ny - 1; j++)
        {
            for (int k = 1; k < Nz - 1; k++)
            {
                currErr = abs(grid[i][j][k] - phi(coordTrans(xLocalStart, hx, i), coordTrans(yStart, hy, j), coordTrans(zStart, hz, k)));

                if (currErr > maxLocalErr)
                {
                    maxLocalErr = currErr;
                }
            }
        }
    }

    // Максимальное значение ошибки по всем процессам
    double absoluteMax = -1;
    MPI_Reduce((void*)&maxLocalErr, (void*)&absoluteMax, 1, MPI_DOUBLE, MPI_MAX, rootProcRank, MPI_COMM_WORLD);

    return absoluteMax;
}

void jacobiMPI(double***& grid1, int xLocalLength, int xLocalStartIndx, int lowerProcRank, int upperProcRank)
{
    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Значение сходимости для некоторого узла сетки
    double currConverg;
    // Максимальное значение сходимости по всем узлам на некоторой итерации
    double maxLocalConverg;

    // Флаг, показывающий, является ли эпсилон меньше любого значения сходимости для данного процесса
    bool isEpsilonLower = true;

    const double hx2 = pow(hx, 2);
    const double hy2 = pow(hy, 2);
    const double hz2 = pow(hz, 2);

    // Константа, вынесенная за скобки
    double c = 1 / ((2 / hx2) + (2 / hy2) + (2 / hz2) + a);

    // Второй массив для того, чтобы использовать значения предыдущей итерации
    double*** grid2;

    // Просто копируем входной массив
    grid2 = new double** [xLocalLength];

    for (int i = 0; i < xLocalLength; i++)
    {
        grid2[i] = new double* [Ny];

        for (int j = 0; j < Ny; j++)
        {
            grid2[i][j] = new double[Nz];

            for (int k = 0; k < Nz; k++)
            {
                grid2[i][j][k] = grid1[i][j][k];
            }
        }
    }

    // Указатель на массив, из которого на некоторой итерации
    // берутся значения для расчёта
    double*** currentSourcePtr = grid1;

    // Указатель на массив, в который на некоторой итерации
    // Записываются новые значения
    double*** currentDestPtr = grid2;

    // Вспомогательный указатель для перемены указателей на массивы
    double*** tmpPtr;

    int messageLength = (Ny - 2) * (Nz - 2);

    double* messageBufSend_1 = new double[messageLength];
    double* messageBufSend_2 = new double[messageLength];

    double* messageBufReqv_1 = new double[messageLength];
    double* messageBufReqv_2 = new double[messageLength];

    // Флаг, который показывает, что нужно продолжать вычисления 
    bool loopFlag = true;

    MPI_Request requests[4];

    while(loopFlag)
    {
        maxLocalConverg = 0.0;

        // Сначала вычисляем граничные значения
        // При i = 1
        for (int j = 1; j < Ny - 1; j++)
        {
            for (int k = 1; k < Nz - 1; k++)
            {
                // Первая дробь в скобках
                currentDestPtr[1][j][k] = (currentSourcePtr[2][j][k] + currentSourcePtr[0][j][k]) / hx2;

                // Вторая дробь в скобках
                currentDestPtr[1][j][k] += (currentSourcePtr[1][j + 1][k] + currentSourcePtr[1][j - 1][k]) / hy2;

                // Третья дробь в скобках
                currentDestPtr[1][j][k] += (currentSourcePtr[1][j][k + 1] + currentSourcePtr[1][j][k - 1]) / hz2;

                // Остальная часть вычисления нового значения для данного узла
                currentDestPtr[1][j][k] -= rho(currentSourcePtr[1][j][k]);
                currentDestPtr[1][j][k] *= c;

                // Сходимость для данного узла
                currConverg = abs(currentDestPtr[1][j][k] - currentSourcePtr[1][j][k]);

                if (currConverg > maxLocalConverg)
                {
                    maxLocalConverg = currConverg;
                }
            }
        }

        // Если процесс должен отправить свой крайний слой с младшим значением x (не содержит слоя с x = 0)
        if (lowerProcRank != -1)
        {
            for (int j = 0; j < Ny - 2; j++)
            {
                for (int k = 0; k < Nz - 2; k++)
                {
                    messageBufSend_1[(Ny - 2) * j + k] = currentDestPtr[1][j + 1][k + 1];
                }
            }

            // Отправляем слой младшему процессу
            MPI_Isend((void*)messageBufSend_1, messageLength, MPI_DOUBLE, lowerProcRank, UPPERBOUND, MPI_COMM_WORLD, &requests[0]);
            MPI_Irecv((void*)messageBufReqv_1, messageLength, MPI_DOUBLE, lowerProcRank, LOWERBOUND, MPI_COMM_WORLD, &requests[2]);
        }

        // Если процесс должен отправить свой крайний слой со старшим значением x (не содержит слоя с x = Nx - 1)
        if (upperProcRank != -1)
        {
            for (int j = 0; j < Ny - 2; j++)
            {
                for (int k = 0; k < Nz - 2; k++)
                {
                    messageBufSend_2[(Ny - 2) * j + k] = currentDestPtr[xLocalLength - 2][j + 1][k + 1];
                }
            }

            // Отправляем слой старшему процессу
            MPI_Isend((void*)messageBufSend_2, messageLength, MPI_DOUBLE, upperProcRank, LOWERBOUND, MPI_COMM_WORLD, &requests[1]);
            MPI_Irecv((void*)messageBufReqv_2, messageLength, MPI_DOUBLE, upperProcRank, UPPERBOUND, MPI_COMM_WORLD, &requests[3]);
        }

        for (int i = 2; i < xLocalLength - 1; i++)
        {
            for (int j = 1; j < Ny - 1; j++)
            {
                for (int k = 1; k < Nz - 1; k++)
                {

                    // Первая дробь в скобках
                    currentDestPtr[i][j][k] = (currentSourcePtr[i + 1][j][k] + currentSourcePtr[i - 1][j][k]) / hx2;

                    // Вторая дробь в скобках
                    currentDestPtr[i][j][k] += (currentSourcePtr[i][j + 1][k] + currentSourcePtr[i][j - 1][k]) / hy2;

                    // Третья дробь в скобках
                    currentDestPtr[i][j][k] += (currentSourcePtr[i][j][k + 1] + currentSourcePtr[i][j][k - 1]) / hz2;

                    // Остальная часть вычисления нового значения для данного узла
                    currentDestPtr[i][j][k] -= rho(currentSourcePtr[i][j][k]);
                    currentDestPtr[i][j][k] *= c;

                    // Сходимость для данного узла
                    currConverg = abs(currentDestPtr[i][j][k] - currentSourcePtr[i][j][k]);
                    if (currConverg > maxLocalConverg)
                    {
                        maxLocalConverg = currConverg;
                    }

                }
            }
        }

        if (maxLocalConverg < accuracy)
        {
            isEpsilonLower = false;
        }

        // Применяем логичекую операцию ИЛИ над флагом сходимости между всеми процессами и помещаем результат во флаг цикла.
        // Таким образом, цикл завершится, когда у всех процессов сходимость будет меньше, чем эпсилон
        MPI_Reduce((void*)&isEpsilonLower, (void*)&loopFlag, 1, MPI_C_BOOL, MPI_LOR, MAINPROC, MPI_COMM_WORLD);
        MPI_Bcast((void*)&loopFlag, 1, MPI_C_BOOL, MAINPROC, MPI_COMM_WORLD);

        // Меняем местами указатели на массив-источник и массив-приёмник
        tmpPtr = currentSourcePtr;
        currentSourcePtr = currentDestPtr;
        currentDestPtr = tmpPtr;

        MPI_Status status;

        if (lowerProcRank != -1)
        {
            MPI_Wait(&requests[0], &status);
        }

        if (upperProcRank != -1)
        {
            MPI_Wait(&requests[1], &status);
        }

        if (lowerProcRank != -1)
        {
            MPI_Wait(&requests[2], &status);

            for (int j = 0; j < Ny - 2; j++)
            {
                for (int k = 0; k < Nz - 2; k++)
                {
                    currentDestPtr[0][j + 1][k + 1] = messageBufReqv_1[(Ny - 2) * j + k];
                }
            }
        }

        if (upperProcRank != -1)
        {
            MPI_Wait(&requests[3], &status);

            for (int j = 0; j < Ny - 2; j++)
            {
                for (int k = 0; k < Nz - 2; k++)
                {
                    currentDestPtr[xLocalLength - 1][j + 1][k + 1] = messageBufReqv_2[(Ny - 2) * j + k];
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }


    // В итоге массив должен содержать значения последней итерации
    if (currentSourcePtr == grid1) 
    {
        freeMemory(grid2, xLocalLength);
    }
    else
    {
        tmpPtr = grid1;
        grid1 = currentSourcePtr;
        freeMemory(tmpPtr, xLocalLength);
    }

    delete[] messageBufSend_1;
    delete[] messageBufSend_2;
    delete[] messageBufReqv_1;
    delete[] messageBufReqv_2;
}

int main(int argc, char** argv)
{
    // Количество процессов
    int count_proc;

    // Ранг текущего процесса
    int cur_rank;

    // Ранг младшего процесса для текущего
    // (Содержит слой, который необходим для просчёта локального слоя с младшим по x значением)
    // Если -1, то такого процесса нет
    int lower_proc;

    // Ранг старшего процесса для текущего
    // (Содержит слой, который необходим для просчёта локального слоя со старшим по x значением)
    // Если -1, то такого процесса нет
    int upper_proc;

    // Глобальный индекс по x, с которого начинается область текущего процесса (включительно)
    int start_ind;

    // Глобальный индекс по x, на котором заканчивается область текущего процесса (включительно)
    int end_ind;

    // Статус возврата
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &cur_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &count_proc);

    // Если процессов больше, чем внутренних слоёв
    if (count_proc > (Nx - 2))
    {
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if (cur_rank == MAINPROC)
    {
        int integer = (Nx - 2) / count_proc;
        int remainder = (Nx - 2) % count_proc;

        // Размер партии (сразу вычисляем для главного процесса)
        int batchsize = integer + ((cur_rank < remainder) ? 1 : 0);


        lower_proc = -1;
        upper_proc = count_proc == 1 ? -1 : 1;

        start_ind = 1;
        end_ind = batchsize;

        // Младший процесс, для процесса-получателя
        int lowerProcRank;

        // Старший процесс, для процесса-получателя
        int upperProcRank;

        // Начало области для процесса-получателя
        int xLocalStartIndx = 1;

        // Конец области для процесса-получателя
        int xLocalEndIndx;

        // Рассылаем всем процессам необходимую информацию
        for (int proc = 1; proc < count_proc; proc++)
        {
            lowerProcRank = proc - 1;
            MPI_Send((void*)&lowerProcRank, 1, MPI_INT, proc, LOWERPROC, MPI_COMM_WORLD);

            upperProcRank = proc == count_proc - 1 ? -1 : proc + 1;
            MPI_Send((void*)&upperProcRank, 1, MPI_INT, proc, UPPERPROC, MPI_COMM_WORLD);

            xLocalStartIndx += batchsize;

            batchsize = integer + ((proc < remainder) ? 1 : 0);

            MPI_Send((void*)&xLocalStartIndx, 1, MPI_INT, proc, X_LOCALSTART, MPI_COMM_WORLD);

            xLocalEndIndx = xLocalStartIndx + batchsize - 1;
            MPI_Send((void*)&xLocalEndIndx, 1, MPI_INT, proc, X_LOCALEND, MPI_COMM_WORLD);
        }
    }
    else
    {
        // Остальные процессы получают от главного необходимую информацию
        MPI_Recv((void*)&lower_proc, 1, MPI_INT, MAINPROC, LOWERPROC, MPI_COMM_WORLD, &status);
        MPI_Recv((void*)&upper_proc, 1, MPI_INT, MAINPROC, UPPERPROC, MPI_COMM_WORLD, &status);

        MPI_Recv((void*)&start_ind, 1, MPI_INT, MAINPROC, X_LOCALSTART, MPI_COMM_WORLD, &status);
        MPI_Recv((void*)&end_ind, 1, MPI_INT, MAINPROC, X_LOCALEND, MPI_COMM_WORLD, &status);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Длина области текущего процесса
    int xMyLocalLength = end_ind - start_ind + 1;

    // Добавляем два слоя, которые вычисляют соседние процессы, чтобы производить сво вычисления на крайних слоях
    // (либо это константные значения внешних слоёв, если таковых процессов нет)
    xMyLocalLength += 2;

    // Инициализируем решётку
    double*** myGrid;
    createGrid(myGrid, xMyLocalLength, coordTrans(xStart, hx, start_ind - 1));

    // Если процесс содержит первый внешний слой, где x - const (x = xStart)
    if (cur_rank == MAINPROC)
    {
        // Записываем краевые значения
        // При i = xStart
        for (int j = 1; j < Ny - 1; j++)
        {
            for (int k = 1; k < Nz - 1; k++)
            {
                myGrid[0][j][k] = phi(xStart, coordTrans(yStart, hy, j), coordTrans(zStart, hz, k));
            }
        }
    }

    // Если процесс содержит второй внешний слой, где x - const (x = xEnd)
    if (cur_rank == count_proc - 1)
    {
        // Записываем краевые значения
        // При i = xEnd 
        for (int j = 1; j < Ny - 1; j++)
        {
            for (int k = 1; k < Nz - 1; k++)
            {
                myGrid[xMyLocalLength - 1][j][k] = phi(xEnd, coordTrans(yStart, hy, j), coordTrans(zStart, hz, k));
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double startTime = MPI_Wtime();

    // Производим вычисления по методу Якоби
    jacobiMPI(myGrid, xMyLocalLength, start_ind, lower_proc, upper_proc);

    double elapsedTime  = MPI_Wtime() - startTime;

    // Считаем общую точность
    double total_accuracy = getAccuracyMPI(myGrid, xMyLocalLength, coordTrans(xStart, hx, start_ind - 1), MAINPROC);

    if (cur_rank == MAINPROC)
    {
        cout << "\nAccuracy: " << total_accuracy << '\n';
        cout << "\nTime: "     << elapsedTime << "s\n";
    }

    MPI_Finalize();

    freeMemory(myGrid, xMyLocalLength);
}
