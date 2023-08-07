//Utils del kernel
#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <sockets.h>
#include <utilsShared.h>
#include <comunicacion.h>

typedef struct {
    char* puerto_escucha;
    char* algoritmo_planificacion;
    int estimacion_inicial;
    double hrrn_alfa;
    int grado_max_multiprogramacion;
    char** recursos;
    char** instancias_recursos;
} Configuracion;

typedef struct {
	char* ip;
	char* puerto;
} config_conex;

typedef struct {
	int pid_proceso;
	t_list* pid_proceso_espera;
} archivo_abierto;

void liberar_archivo_abierto(void* archivo);

void leer_config();
void conectarse_cpu();
void conectarse_memoria();
void conectarse_fileSystem();
void terminar_programa();
t_list *recibir_instrucciones(int);
void eliminar_segmento(int, int);
void eliminar_proceso(int);
void enviar_operacion_fs(char*, uint32_t, uint32_t, uint32_t, int);
void enviar_truncate_fs(char*, uint32_t);


//Variables globales utilizadas dentro de todo el kernel
extern t_config* config;
extern Configuracion* configuracion;
extern t_log* logger;
extern config_conex* config_valores_memoria;
extern config_conex* config_valores_filesystem;
extern config_conex* config_valores_cpu;
extern t_list* lista_procesos;
extern int socket_memoria;
extern int socket_cpu;
extern int socket_fs;

#endif
