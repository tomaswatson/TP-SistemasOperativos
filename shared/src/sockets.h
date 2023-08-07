#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

typedef enum{
	KERNEL,
	CPU,
	FILESYSTEM,
	MEMORIA
} servidor;

void test(void);

int socket_crear_servidor(char*, t_log*);

int socket_conectarse_servidor(char*, char*, t_log*);

void handshake_cliente(int, t_log*);

void handshake_servidor(int, t_log*, int);

void detectar_servidor(int, t_log *);

void administrar_archivo(char*, int, int);

#endif /* SOCKETS_H_ */
