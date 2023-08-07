/*
 * utils.h
 *
 *  Created on: Apr 24, 2023
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <sockets.h>
#include <utilsShared.h>
#include <pthread.h>
#include <comunicacion.h>

typedef struct{
	char* PUERTO_ESCUCHA;
	int TAM_MEMORIA;
	int TAM_SEGMENTO_0;
	int CANT_SEGMENTOS;
	int RETARDO_MEMORIA;
	int RETARDO_COMPACTACION;
	char* ALGORITMO_ASIGNACION;
}Configuracion;

typedef struct{
	int comienzo;
	int fin;
	int tamanio;
}Hueco_Vacio;

void conectar_con_kernel();
void conectar_con_cpu();
void conectar_con_fileSystem();
void leerconfig();
void terminar_programa();
void crearHuecoVacio(int , int );
void crear_memoria();
Proceso* crear_proceso();
segmento* crearSegmento(int,int,int);
char* leer(uint32_t, int);
void escribir(uint32_t, int, char*);
int algoritmoBestFit(int);
int algoritmoWorstFit(int);
int algoritmoFirstFit(int);
void elegirAsignacion();
Hueco_Vacio* comparardorMinimo(int,Hueco_Vacio*,Hueco_Vacio*);
Hueco_Vacio* comparardorMaximo(int,Hueco_Vacio*,Hueco_Vacio*);
int obtenerIndiceProceso(uint32_t);
void combinarHuecos();
bool entraEnElHueco(int, Hueco_Vacio*);
bool buscarHueco(int);
int aniadirSegmento(Proceso*,segmento*);
Proceso* ingresar_nuevo_proceso(int);
void eliminar_segmento(int, int);
void recibir_eliminacion_segmento();
bool esElSegmento(segmento*,int);
segmento* procesoContieneDireccion(Proceso*,int);
segmento* buscarSegmento(int);
int espacioVacioEnMemoria();
void recibir_escritura();
void recibir_lectura();
segmento* crearSegmento0(int ,int ,int );
void escribir_archivo();
void leer_archivo();
int sumarTamanioHuecos(int ,Hueco_Vacio* );
bool ordenarLista(Hueco_Vacio* ,Hueco_Vacio* );
void ordernarListaHuecosVacios();
void loggersHuecos();

extern t_log* logger;
extern t_config* config;
extern Configuracion* configuracion;
extern int socket_cpu;
extern int socket_fileSystem;
extern int socket_kernel;
extern int server_fd;
extern void* memoria;
extern t_list* listaDeProcesos;
extern segmento* segmento0;
extern t_list* huecosVacios;
extern int (*puntero_algoritmo)(int); //puntero para la funcion de asignacion

#endif /* UTILS_H_ */
