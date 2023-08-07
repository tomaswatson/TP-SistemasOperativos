#include "./sockets.h"

void test(void){
	printf("se promociona SO \n");
}

int socket_crear_servidor(char* puerto, t_log *logger)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol);

	// Asociamos el socket a un puerto

	if(socket_servidor == -1 || bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		freeaddrinfo(servinfo);
		log_error(logger, "Fallo al crear el servidor");
		abort();
		return -1;
	}
	// Escuchamos las conexiones entrantes

	/*SOMAXCONN es una constante con un valor de 4096. Su funcion es la de limitar la cantidad máxima de conexiones
	que recibe. Si se supera el número máximo de peticiones, el servidor empezará a rechazar las peticiones entrantes*/
	listen(socket_servidor, SOMAXCONN);

	log_info(logger, "Servidor creado exitosamente");
	freeaddrinfo(servinfo);

	return socket_servidor;
}

int socket_conectarse_servidor(char *ip, char* puerto, t_log *logger)
{
	struct addrinfo hints;
	struct addrinfo *server_info;
	struct addrinfo *p;
	int socket_cliente;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	for(p = server_info; p != NULL; p = p->ai_next){
		socket_cliente = socket(p->ai_family,
						 p->ai_socktype,
						 p->ai_protocol);

		if(socket_cliente == -1) continue;

		if (connect(socket_cliente, p->ai_addr, p->ai_addrlen) != -1) break;

		close(socket_cliente);

		socket_cliente = -1;
	}
	freeaddrinfo(server_info);
	handshake_cliente(socket_cliente, logger);

	return socket_cliente;
}

void handshake_cliente(int socket, t_log *logger) {

uint32_t handshake = 1;
uint32_t result = -1;

	send(socket, &handshake, sizeof(uint32_t), 0);
	recv(socket, &result, sizeof(uint32_t), MSG_WAITALL);

	if(result != -1) {
		detectar_servidor(result, logger);
	}
	else {
		log_error(logger, "Error al intentar conectar con el servidor");
		abort();
	}
}

void handshake_servidor(int socket, t_log *logger, int cod_servidor){
	uint32_t handshake = 0;
	uint32_t resultOk = cod_servidor;
	uint32_t resultError = 0;

	recv(socket, &handshake, sizeof(uint32_t), MSG_WAITALL);

	if(handshake){
		send(socket, &resultOk, sizeof(uint32_t), 0);
		log_info(logger, "Conexion establecida con el cliente \n");
	}
	else{
		send(socket, &resultError, sizeof(uint32_t), 0);
		log_error(logger, "Error al intentar conectar con el cliente \n");
		abort();
	}
}

void detectar_servidor(int cod_servidor, t_log *logger){
	char* servidor = malloc(20);
	switch(cod_servidor){
	case KERNEL: strcpy(servidor, "Kernel");
				 break;

	case CPU: strcpy(servidor, "CPU");
			  break;

	case FILESYSTEM: strcpy(servidor, "FileSystem");
				  break;

	case MEMORIA: strcpy(servidor, "Memoria");
				  break;

	default: break;
	}

	log_info(logger, "Conectado correctamente con %s \n", servidor);
	free(servidor);
}

