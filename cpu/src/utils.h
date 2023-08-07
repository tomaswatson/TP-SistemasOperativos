#ifndef UTILS_H_
#define UTILS_H_

#include <sockets.h>
#include <utilsShared.h>
#include <pthread.h>
#include <comunicacion.h>


extern Registros* registros;

typedef struct {

	int RETARDO_INSTRUCCION;
	char* PUERTO_ESCUCHA;
	int TAM_MAX_SEGMENTO;

}EstructuraConfiguracion;

typedef struct{

	char* puerto;
	char* ip;

}conexion;

extern t_log* logger;
extern t_config* config;
extern EstructuraConfiguracion* configuracion;
extern conexion conexion_memoria;
extern conexion servidor_cpu;
extern int socket_servidor;
extern int socket_kernel;
extern int socket_memoria;


void conectarse_a_memoria();
void esperar_kernel();
int obtener_numRegistro(char *registro);
void setear_registro(char *registro, char *valor);
char* leer_registro(char* registro);
void obtenerParametros(const char* cadena, char** cad1, char** cad2, char** cad3);
int obtener_tamanio_registros(int num_registro);
void leerConfig(void);
void terminar_programa(void);
int cantidadDigitos(int);
char* leer_memoria(uint32_t, int);
void escribir_memoria(uint32_t, char*);


#endif /* UTILS_H_ */
