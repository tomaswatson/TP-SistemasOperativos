#include "utils.h"

t_log *logger;
t_config *config;
char* IP;
char* PUERTO;
int SOCKET;
int estado_proceso = -1;
int cant_instrucciones = 0;

Instruccion_Leida tablaInstrucciones[] = {
    {"F_READ", 3},
    {"F_WRITE", 3},
    {"SET", 2},
    {"MOV_IN", 2},
    {"MOV_OUT", 2},
    {"F_TRUNCATE", 2},
    {"F_SEEK", 2},
    {"CREATE_SEGMENT", 2},
    {"I/O", 1},
    {"WAIT", 1},
    {"SIGNAL", 1},
    {"F_OPEN", 1},
    {"F_CLOSE", 1},
    {"DELETE_SEGMENT", 1},
    {"EXIT", 0},
    {"YIELD", 0},
};

void terminar_programa(t_config* config, t_log *logger){

	log_info(logger, "Finalizando programa");

	config_destroy(config);

	log_destroy(logger);

}


t_paquete* paquete_instrucciones(FILE* instrucciones){
	t_paquete* paquete = crear_paquete();

	char linea[60];
	Instruccion *instruccion_parseada = malloc(sizeof(Instruccion));
	instruccion_parseada->parametros = malloc(sizeof(char) * 60);
	int instruccion_size;
	while (fgets(linea, 60, instrucciones)){
		char *parametros = malloc(sizeof(char) * 60);
		char *instruccion, *param1, *param2, *param3;
		instruccion = strtok(linea, " \n");
		param1 = strtok(NULL, " \n");
		param2 = strtok(NULL, " \n");
		param3 = strtok(NULL, " \n");

		if(param1){
			strcpy(parametros, param1);
			if(param2){
				strcat(parametros, " ");
				strcat(parametros, param2);
				if(param3){
					strcat(parametros, " ");
					strcat(parametros, param3);
				}
			}
		}
		else strcpy(parametros, "");

		int cant_parametros = (param1 ? 1 : 0) + (param2 ? 1 : 0) + (param3 ? 1 : 0);

		int codigo_instruccion = instruccion_correcta(instruccion, cant_parametros);

		if(codigo_instruccion == -1){
			log_error(logger, "Instruccion incorrecta: %s - Cantidad de parametros: %d - cod_instruccion %d", instruccion, cant_parametros, codigo_instruccion);
			abort();
		}

		instruccion_parseada->instruccion = codigo_instruccion;
		strcpy(instruccion_parseada->parametros, parametros);
		instruccion_parseada->parametros_long = strlen(instruccion_parseada->parametros) + 1;

		instruccion_size = sizeof(instruction_index) + sizeof(uint32_t) + strlen(instruccion_parseada->parametros) + 1;
		agregar_instruccion(paquete->buffer, instruccion_parseada, instruccion_size);
		cant_instrucciones++;
		free(parametros);
		}

	free(instruccion_parseada);

	return paquete;
}

int instruccion_correcta(char *instruccion, int cant_parametros){
	int correcto = -1;
	for (int i = 0; i < 16; i++) {
		if (strcmp(tablaInstrucciones[i].instruccion, instruccion) == 0) {
			if (cant_parametros != tablaInstrucciones[i].cant_parametros) {
				log_error(logger, "La instrucción %s requiere %d parámetros.\n", instruccion, tablaInstrucciones[i].cant_parametros);
				break;
			}
			else{
				correcto = i;
			}
			break;
		}
	}

	return correcto;
}

FILE *archivo_instrucciones(char* ruta_instrucciones){

	FILE *instrucciones = fopen(ruta_instrucciones, "r");

	if(instrucciones == NULL){
			log_error(logger, "Fallo al cargar el archivo de instrucciones");
			abort();
	}

	return instrucciones;
}

void cargar_config(char* ruta_config){
	config = iniciar_config(ruta_config);

	IP = config_get_string_value(config, "IP_KERNEL");
	PUERTO = config_get_string_value(config, "PUERTO_KERNEL");

}

void esperar_respuesta(){

	log_info(logger, "Esperando finalizacion del proceso");

	read(SOCKET, &estado_proceso, sizeof(int));

	if(estado_proceso == SUCCESS){
		log_info(logger, "Proceso finalizado con exito");
	}
	else {
		char* error = codigo_error(estado_proceso);
		log_error(logger, "Proceso finalizado con codigo de error: %s", error);
		free(error);
	}
}
