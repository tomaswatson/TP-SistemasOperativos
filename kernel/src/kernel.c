#include "kernel.h"

int main(int argc, char** argv){

//	argv[1] = "./cfg/memoria.config";
	config = iniciar_config(argv[1]);

	leer_config(config, configuracion);

	logger = iniciar_logger("./logs.txt");

	inicializarEstructurasColas();

	socket_servidor = socket_crear_servidor(configuracion->puerto_escucha, logger);

	//Conexion con otros módulos

	conectarse_memoria();
	conectarse_cpu();
	conectarse_fileSystem();

	//Hilos

	pthread_t gestionarConsolas;
	pthread_create(&gestionarConsolas, NULL, (void*) recibirConsolas, NULL);

	pthread_t conexion_cpu;
	pthread_create(&conexion_cpu, NULL, (void*) ejecutarProcesos, NULL); //Ejecuta los procesos

	pthread_t manejo_filesystem;
	pthread_create(&manejo_filesystem, NULL, (void*) gestionar_fileSystem, NULL);

	pthread_t planificador_largo_plazo;
	pthread_create(&planificador_largo_plazo, NULL, (void*) planificadorLargoPlazo, NULL);

	pthread_t planificador_corto_plazo;
	pthread_create(&planificador_corto_plazo, NULL, (void*) elegirPlanificador, NULL);

	pthread_join(manejo_filesystem, NULL);
	pthread_join(conexion_cpu, NULL);
	pthread_join(gestionarConsolas, NULL);
	pthread_join(planificador_corto_plazo, NULL);
	pthread_join(planificador_largo_plazo, NULL);

	//Bajar el sistema

	liberarEstructurasColas();
	terminar_programa();

	return EXIT_SUCCESS;
}
