#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PORTS_NUM 65535
#define THREADS_NUM 8

int CURRENT_PORT = 0;
int PERCENT_DONE = 1;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void scan_ports(const char* ip_addr);
void* run_thread(void* arguments);

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: port_scanner <ip>\n");
        fprintf(stderr, "Usage: port_scanner 127.0.0.1\n");
        exit(1);
    }
    scan_ports(argv[1]);
    return 0;
}

void scan_ports(const char* ip_addr) {
    int status;
    pthread_t threads[THREADS_NUM];
    pthread_mutex_init(&mtx, NULL);
    for (int i = 0; i < THREADS_NUM; i++) {
        printf("! Creating thread %d\n", i + 1);
        status = pthread_create(&threads[i], NULL, run_thread, (void*)ip_addr);
        if (status != 0) {
            fprintf(stderr, "pthread_create: thread creation failed\n");
            exit(-1);
        }
    }
    for (int i = 0; i < THREADS_NUM; i++) {
        status = pthread_join(threads[i], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join: thread termination failed\n");
            exit(-1);
        }
        printf("! Thread %d has finished\n", i + 1);
    }
    pthread_mutex_destroy(&mtx);
}

void* run_thread(void* arguments) {
    int status;
    int sock;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct timeval timeout;
    char* ip_addr = (char*)arguments;
    while (1) {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        pthread_mutex_lock(&mtx);
        if (CURRENT_PORT == MAX_PORTS_NUM) {
            pthread_mutex_unlock(&mtx);
            break;
        }

        CURRENT_PORT++;
        if (CURRENT_PORT % 6535 == 0) {
            printf("[*] %d%% of ports are scanned\n", PERCENT_DONE++ * 10);
        }

        char port_num[6] = {0};
        sprintf(port_num, "%d", CURRENT_PORT);
        pthread_mutex_unlock(&mtx);

        if ((status = getaddrinfo(ip_addr, port_num, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            exit(-1);
        }

        sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (sock == -1) {
            fprintf(stderr, "socket: socket creation failed %d\n", errno);
            freeaddrinfo(servinfo);
            exit(-1);
        }

        status = fcntl(sock, F_SETFL, O_NONBLOCK);
        if (status == -1) {
            fprintf(stderr, "fcntl: file descriptor flag set failed %d\n", errno);
            freeaddrinfo(servinfo);
            close(sock);
            exit(-1);
        }

        status = connect(sock, servinfo->ai_addr, servinfo->ai_addrlen);

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        if (status != -1) {
            fprintf(stdout, "[+] Succeded to connect to %s:%s\n", ip_addr, port_num);
        } else if (errno == EINPROGRESS) {
            status = select(sock + 1, NULL, &write_fds, NULL, &timeout);
            if (status == -1) {
                fprintf(stderr, "select: fd monitor failed %d\n", errno);
                freeaddrinfo(servinfo);
                close(sock);
                exit(-1);
            } else if (status == 1) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    fprintf(stdout, "[+] Succeded to connect to %s:%s\n", ip_addr, port_num);
                }
            }
        }
        close(sock);
        freeaddrinfo(servinfo);
    }
    return NULL;
}
