#ifndef COMUNICACION_H_
#define COMUNICACION_H_
#include <stdint.h>
#include <stdlib.h>
//#include <semaphore.h> INDAGAR QUE ES
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <commons/log.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

//Estructuras
typedef enum{
	NUEVO,
	LISTO,
	EJECUTANDO,
	BLOQUEADO,
	FINALIZADO
} estado;

typedef enum{
	ARCHIVO_EXISTE,
	ARCHIVO_NO_EXISTE
} codigo_archivo;

// Registros
typedef struct{

	char AX[4];
	char BX[4];
	char CX[4];
	char DX[4];
	char EAX[8];
	char EBX[8];
	char ECX[8];
	char EDX[8];
	char RAX[16];
	char RBX[16];
	char RCX[16];
	char RDX[16];

}Registros;

typedef struct{
	uint32_t id;
	uint32_t direccion_base;
	uint32_t tamanio_segmento;
} segmento;

typedef enum{
	INICIALIZAR_PROCESO,
	FINALIZAR_PROCESO,
	ACCEDER_ESPACIO_LECTURA,
	ACCEDER_ESPACIO_ESCRITURA,
	CREAR_SEGMENTO,
	ELIMINAR_SEGMENTO,
	COMPACTAR_SEGMENTOS
}operaciones_memoria;

typedef enum{
	ABRIR_ARCHIVO,
	CREAR_ARCHIVO,
	TRUNCAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	CERRAR_ARCHIVO
}cod_op_fs;

typedef enum
{
	F_READ,
	F_WRITE,
	SET,
	MOV_IN,
	MOV_OUT,
	F_TRUNCATE,
	F_SEEK,
	CREATE_SEGMENT,
	IO,
	WAIT,
	SIGNAL,
	F_OPEN,
	F_CLOSE,
	DELETE_SEGMENT,
	EXIT,
	YIELD
}instruction_index;

typedef struct{
	int idProceso;
	int size;
	t_list* tablaDeSegmentos;
}Proceso;

typedef struct{
	instruction_index instruccion;
	uint32_t size;
	void* parametro;
} solicitud_instruccion;

typedef struct{
	char* nombre_archivo;
	int puntero;
} archivo_abierto_proceso;

//Errores

typedef enum {

	SUCCESS,
	SEG_FAULT,
	OUT_OF_MEMORY,
	MEMORY_COMPACTION,
	WAIT_INEXISTENTE,
	SIGNAL_INEXISTENTE

}error;

typedef struct{
	uint32_t instrucciones_size; //4
	t_list* instrucciones; // n
	uint32_t program_counter; // 4
	Registros* registros;  // 112
	uint32_t segmentos_size;  // 4
	t_list* tabla_segmentos; // n
	estado estado_proceso; // 16
	uint32_t tiempo_bloqueo; // 4
	solicitud_instruccion *solicitud;  // 16 + 4 + n
	error estado_error; // 16
}contexto_ejecucion;

typedef struct{
	uint32_t id;
	contexto_ejecucion* contexto_ejecucion;
	double estimado_rafaga_proxima;
	double estimado_rafaga_anterior;
	int tiempo_llegada_ready;
	t_temporal* tiempo_en_ready;
	t_list* recursosAsignados;
	t_list* tabla_archivos_abiertos;
	char* recursoSolicitado;
	int socket_cliente;
}pcb;

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct{
	instruction_index instruccion;
	uint32_t parametros_long;
	char* parametros;
} __attribute__((packed))
Instruccion;

//Interfaz

void deserializar_instrucciones(t_buffer*, t_list*);

void crear_buffer(t_paquete*);

t_paquete* crear_paquete(void);

void eliminar_paquete(t_paquete*);

void agregar_a_paquete(t_buffer*, contexto_ejecucion*, int);

void agregar_instruccion(t_buffer*, Instruccion* , int);

int serializar_instrucciones(t_list*, t_buffer*, int);

int serializar_registros(Registros*, t_buffer*, int);

int serializar_segmentos(t_list*, t_buffer*, int);

void agregar_segmento(segmento*, t_buffer*, int, int);

void enviar_paquete(t_paquete*, int);

void* serializar_paquete(t_paquete*, int);

t_paquete *paquete_contexto(contexto_ejecucion*);

int instruction_size(t_list*);

void deserializar_contexto(t_buffer*, contexto_ejecucion*);

void cargar_segmentos(t_buffer*, t_list*, uint32_t, int);

void cargar_registros(t_buffer*, Registros*, int);

void cargar_instrucciones(t_buffer*, t_list*, uint32_t, int);

char* nombreEstado(int);

void enviar_mensaje(char*, int);

void enviar_mensaje_con_tamanio(char* mensaje, int socket_cliente, int tamanio);

int recibir_operacion(int);

char *recibir_mensaje(int);

void inicializar_registros(Registros*);

void enviar_operacion(int, int);

void enviar_segmentos(int, t_list*);

t_list* recibir_segmentos(int);

void enviar_lista_procesos(int, t_list*);

int procesos_size(t_list*);

void serializar_procesos(t_list*, t_buffer* );

t_list* recibir_lista_procesos(int);

void deserializar_procesos(t_buffer*, t_list*);

int solicitar_creacion_segmento(int, char*, char*, int);

segmento* recibir_solicitud_segmento(int);

t_paquete* recibir_paquete(int);

void solicitar_nuevo_proceso(int socket, int id);

int informar_id(int, int);

int recibir_id(int);

//CONTEXTO
contexto_ejecucion* recibir_contexto(int, t_log*);

void liberar_instruccion(void* instruccion);
void liberar_archivo(void* instruccionVoid);
void liberar_contexto(contexto_ejecucion* contexto);
void liberar_registros(Registros*);
void liberar_proceso(Proceso*);


#endif /* COMUNICACION_H_ */
