#ifndef PORT_SCANNER_H
#define PORT_SCANNER_H

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
#define THREADS_NUM 4

void scan_ports(const char* ip_addr);
void* run_thread(void* arguments);

#endif
