#ifndef UTILS_H_
#define UTILS_H_

#include <sockets.h>
#include <utilsShared.h>
#include <pthread.h>
#include <comunicacion.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <semaphore.h>
#include <math.h>
#include <dirent.h>

#include <fcntl.h>     // Para las constantes O_RDWR, O_CREAT, etc.
#include <sys/mman.h>  // Para mmap(), msync(), y las constantes PROT_READ, PROT_WRITE, MAP_SHARED, etc.
#include <unistd.h>    // Para close()
#include <sys/stat.h>  // Para S_IRUSR, S_IWUSR, etc. (permisos de archivo)


typedef struct {
	char* IP_MEMORIA;
	char* PUERTO_MEMORIA;
	char* PUERTO_ESCUCHA;
	char* PATH_SUPERBLOQUE;
	char* PATH_BITMAP;
	char* PATH_BLOQUES;
	char* PATH_FCB;
	int RETARDO_ACCESO_BLOQUE;
}Configuracion;

typedef struct {
	int BLOCK_COUNT;
	int BLOCK_SIZE;
}SuperBloque;

typedef struct {
	char* nombre_archivo;
	int tamanio_archivo; // (en bytes)
	uint32_t punteroDirecto; // Puntero que apunta al primer bloque de datos
	uint32_t punteroIndirecto; // Apunta a un bloque que contendrá los punteros a los siguientes bloques del archivo.
}FCB;

/*Esta estructura hay que estudiarla mas a fondo*/
typedef struct{
	int numero_instruccion;
	char* nombre_archivo;
	char* param2;
	char* param3;
}Instruccion_a_procesar;

extern SuperBloque* superBloque;
extern t_config* config_superBloque;
extern t_bitarray* bitMap;
extern void* bitMap_mapeado;
extern t_log* logger;
extern t_config* config;
extern t_config* templateFcb;
extern Configuracion* configuracion;
extern t_dictionary* tabla_archivos_abiertos;
extern void* archivo_bloques;
extern int socket_memoria;
extern int socket_servidor;
extern int socket_kernel;
extern int processID;

void leerConfig(void);
void conectarse_memoria();
void recibirKernel(int);
void esperar_kernel(void);
void atender_solicitudes_kernel(void);
void terminar_programa(void);


// Solicitudes del Kernel

void abrir_archivo(void);
void crear_archivo(void);
void truncar_archivo(char* nombre_archivo, int tamanio);
void leer_archivo(char*, uint32_t, uint32_t, uint32_t);
void escribir_archivo(char*, uint32_t, uint32_t, uint32_t);
void cerrar_archivo(void);
void recibir_lectura(void);
void recibir_escritura(void);
void recibir_truncado(void);
void persistir_fcb(FCB*);
bool bloque_libre(int index);
uint32_t divisionRedondeada(uint32_t numerador, uint32_t denominador);
t_config* iniciar_fcb(void);


//
void levantar_superBloque();
void levantar_bitMap();
void crear_estructuras_administrativas();
void procesar_archivo(char*, char*);
void imprimir_bloques_ocupados();
int actualizar_bitmap_disco();
uint32_t ocupar_bloque_vacio();
void levantar_archivo_bloques();
uint32_t buscar_numero_bloque(FCB* fcb, uint32_t puntero);
uint32_t limpiar_numero_bloque(FCB* fcb, uint32_t puntero);
void escribir_bloque(char* nombre_archivo, uint32_t puntero, void* informacion_escritura, uint32_t tamanio_escribir);
char* leer_bloque(FCB* fcb, uint32_t puntero, uint32_t tamanio_leer);

int actualizar_archivo_bloques_disco();
FCB* crear_fcb(char*, int, uint32_t, uint32_t);
void levantar_estructuras(void);
void terminar_programa(void);

#endif /* UTILS_H_ */
