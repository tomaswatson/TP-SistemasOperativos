#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sockets.h>
#include <comunicacion.h>
#include <utilsShared.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdint.h>

extern t_log *logger;
extern t_config *config;
extern char* IP;
extern char* PUERTO;
extern int SOCKET;
extern int estado_proceso;
extern int cant_instrucciones;

typedef struct{
	char* instruccion;
	int cant_parametros;
} Instruccion_Leida;

void terminar_programa(t_config* config, t_log *logger);

void* serializar_paquete(t_paquete*, int);

void verificar_instrucciones(FILE*, t_log*);

t_paquete *paquete_instrucciones(FILE*);

int instruccion_correcta(char*, int);

FILE *archivo_instrucciones(char*);

void cargar_config(char*);

void esperar_respuesta();

#endif /* UTILS_H_ */
