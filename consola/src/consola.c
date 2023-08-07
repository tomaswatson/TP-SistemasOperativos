#include "consola.h"

int main(int argc, char** argv) {

	// INICIALIZACION DE CONSOLA

	logger = iniciar_logger("./logs.txt");

	cargar_config(argv[1]);

	// CARGA DE INSTRUCCIONES

	FILE *instrucciones = archivo_instrucciones(argv[2]);

	paquete = paquete_instrucciones(instrucciones);

	log_info(logger, "Intentando conectar al Kernel");

	SOCKET = socket_conectarse_servidor(IP, PUERTO, logger);

	log_info(logger, "Enviando %d instrucciones", cant_instrucciones);

	enviar_paquete(paquete, SOCKET);

	fclose(instrucciones);

	esperar_respuesta();

	terminar_programa(config, logger);

	return estado_proceso;
}
