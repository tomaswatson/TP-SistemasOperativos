#include "fileSystem.h"

int main(int argc, char** argv) {
//	argv[1] = "./cfg/fileSystem.config";
	config = iniciar_config(argv[1]);

	logger = iniciar_logger("./logs.txt");

	leerConfig();
	levantar_superBloque();

	conectarse_memoria();

	levantar_bitMap();
	levantar_estructuras();
	crear_estructuras_administrativas();
	levantar_archivo_bloques();

	socket_servidor = socket_crear_servidor(configuracion->PUERTO_ESCUCHA, logger);

	atender_solicitudes_kernel();

	close(socket_memoria);

	terminar_programa();

	return EXIT_SUCCESS;
}



