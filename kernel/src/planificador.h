#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_


#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <semaphore.h>
#include "comunicacion.h"
#include "utils.h"

//Variables globales
extern uint32_t procesos_creados;
extern uint32_t multiprogramacion;
extern t_queue* estado_new;
extern t_list* estado_ready;
extern t_list* estado_bloqueado;
extern t_queue* estado_ejecutando;
extern t_queue* estado_exit;
extern t_list* recursos_disponibles;
extern t_queue* procesos_bloqueados_fs;
extern t_dictionary* tabla_archivos_abiertos;
extern int socket_servidor;
extern int operaciones_fs;
extern bool fs_bloqueado;

//Semáforos
extern sem_t procesosListos;
extern sem_t procesosEnReady;
extern sem_t procesoParaEjecutar;
extern sem_t noHayProcesoParaEjecutar;
extern sem_t procesosFinalizados;
extern sem_t procesosFileSystemBloqueado;
extern pthread_mutex_t procesos_new;
extern pthread_mutex_t procesos_ready;
extern pthread_mutex_t procesos_bloqueados;
extern pthread_mutex_t procesos_ejecutando;
extern pthread_mutex_t procesos_exit;
extern pthread_mutex_t compactacion_memoria;
extern pthread_mutex_t cant_conexiones_fs;
extern sem_t disponibilidadParaNuevoProceso;


//PCB
void crear_pcb(pcb* proceso, t_list* instrucciones);
void liberar_pcb(pcb* proceso);
const char* PIDsEnReady(void);


//Planificador a corto plazo
void *elegirPlanificador(void);
void planificadorFifo();
void planificadorHrrn();
int obtener_indice_proximoProceso();
double obtenerRR(pcb*);



//Planfificador a largo plazo
void inicializarEstructurasColas(void);
void liberarEstructurasColas(void);
void cargarRecursosDisponibles(void);
void recibirConsolas(void);
void liberarEstructurasColas(void);
void planificadorLargoPlazo(void);
void calcularProximaRafaga(pcb* pcb, t_temporal* tiempoEjecucion);
void ejecutarProcesos(void);
void mover_proceso_a_finalizado(pcb* proceso);
void manejarMultiprogramacion(void);
void liberarProcesosFinalizados(void);
void cambiarEstadoProceso(pcb* pcb, int nuevoEstado);
void bloquearProcesoIO(pcb* pcb);
void nueva_consola(int);
void informar_fin_consola(int, int);


//Recursos
bool hayRecursoDisponible(int indiceRecursoSolicitado);
int indiceRecurso(char* recursoSolicitado);
char* obtenerNombreRecurso(int id_recurso);
bool waitRecurso(int indice_recurso, pcb* proceso);
bool signalRecurso(int indice_recurso, pcb* proceso);

//FileSystem
pcb* sacarProcesoBloqueadoId(int pid);
void gestionar_fileSystem();
archivo_abierto_proceso* encontrar_archivo_proceso(pcb* proceso, char* nombre_archivo);
int encontrar_indice_archivo_proceso(pcb* proceso, char* nombre_archivo);
int obtenerIndiceProceso(uint32_t id);
void asignar_tablaSegmentos();


#endif /* PLANIFICADOR_H_ */
