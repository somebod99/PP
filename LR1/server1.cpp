#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <pthread.h>
#include <cstring>
#include <string>
#include <iostream>

using namespace std;

char c_response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

struct Client
{
    int client_fd;
    int request_number;
};

void* request_handle(void* arg)
{
    Client* client = (Client*)arg;

    string response = c_response;
    response += "Request number: " + std::to_string(client->request_number) + '\n';

    write(client->client_fd, response.c_str(), response.size() * sizeof(char));
    
    shutdown(client->client_fd, SHUT_WR);

    delete client;

    pthread_exit(NULL);
}

int main()
{
    int one = 1, client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cerr << "can't open socket\n";
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    int port = 8080;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1)
    {
        close(sock);
        cerr << "Can't bind\n";
    }

    pthread_attr_t attr;
    int error;

    error = pthread_attr_init(&attr);
    // Если при инициализации переменной атрибутов произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(error != 0)
    {
        cerr << "Cannot create thread attribute: " << strerror(error) << endl;
        exit(-1);
    }

    // Устанавливаем минимальный размер стека для потока (в байтах)
    size_t stack_size = 50 * 1024 * 1024;
    error = pthread_attr_setstacksize(&attr, stack_size);
    // Если при установке размера стека произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(error != 0)
    {
        cerr << "Setting stack size attribute failed: " << strerror(error) << endl;
        exit(-1);
    }

    listen(sock, 5);
    Client* client = NULL;

    int request_number = 1;
    
    pthread_t thread;

    while (1)
    {
        client_fd = accept(sock, NULL, NULL);
        cout << "got connection : " << request_number << '\n';

        if (client_fd == -1)
        {
            cerr << "Can't accept\n";
            continue;
        }

        client = new Client;
        client->request_number = request_number;
        client->client_fd = client_fd;

        ++request_number;
        
        error = pthread_create(&thread, &attr, request_handle, (void*)client);
        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        if(error != 0)
        {
            cerr << "Cannot create a thread: " << strerror(error) << '\n';
            exit(-1);
        }

        pthread_detach(thread);
    }
}