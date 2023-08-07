#include "memoria.h"

int main(int argc, char** argv){

	logger = iniciar_logger("logs.txt");
//	argv[1] = "./cfg/memoriaWorst.config";
	config = iniciar_config(argv[1]);

	leerconfig();

	elegirAsignacion();

	crear_memoria();

	segmento0 = crearSegmento0(0,0,configuracion->TAM_SEGMENTO_0); //mover a utils.c?

	server_fd = socket_crear_servidor(configuracion->PUERTO_ESCUCHA,logger);

	pthread_t thread_conexion_kernel;
	pthread_t thread_conexion_cpu;
	pthread_t thread_conexion_fileSystem;

	socket_cpu = accept(server_fd, NULL, NULL);
	handshake_servidor(socket_cpu,logger, MEMORIA);
	pthread_create(&thread_conexion_cpu,NULL,(void*)conectar_con_cpu,NULL);

	socket_fileSystem = accept(server_fd,NULL,NULL);
	handshake_servidor(socket_fileSystem,logger, MEMORIA);
	pthread_create(&thread_conexion_fileSystem,NULL,(void*)conectar_con_fileSystem,NULL);

	socket_kernel = accept(server_fd, NULL, NULL);
	handshake_servidor(socket_kernel,logger, MEMORIA);
	pthread_create(&thread_conexion_kernel,NULL,(void*)conectar_con_kernel,NULL);

	pthread_join(thread_conexion_cpu,NULL);
	pthread_join(thread_conexion_fileSystem,NULL);
	pthread_join(thread_conexion_kernel,NULL);

	terminar_programa();

	return 0;
};
