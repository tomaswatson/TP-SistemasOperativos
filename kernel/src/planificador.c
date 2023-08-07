#include "planificador.h"


//Variables globales
uint32_t procesos_creados = 0;
uint32_t multiprogramacion = 0; //Puedo tener un máximo de n procesos en ejecucion / bloqueo / ready al mismo tiempo.
t_queue* estado_new;
t_list* estado_ready;
t_list* estado_bloqueado;
t_queue* estado_ejecutando;
t_queue* estado_exit;
t_list* recursos_disponibles;
t_queue* procesos_bloqueados_fs;
t_dictionary* tabla_archivos_abiertos;
int socket_servidor;
bool fs_bloqueado;

//Semáforos
sem_t procesosListos;
sem_t procesosEnReady;
sem_t procesoParaEjecutar;
sem_t noHayProcesoEjecutando;
sem_t procesosFinalizados;
sem_t procesosFileSystemBloqueado;
pthread_mutex_t procesos_new;
pthread_mutex_t procesos_ready;
pthread_mutex_t procesos_bloqueados;
pthread_mutex_t procesos_ejecutando;
pthread_mutex_t procesos_exit;
pthread_mutex_t recursos;
pthread_mutex_t compactacion_memoria;
pthread_mutex_t operando_con_fs;
sem_t disponibilidadParaNuevoProceso;
//Algoritmo
char* algoritmoSeleccionado;

//PCB
void crear_pcb(pcb* proceso, t_list* instrucciones){

	proceso->contexto_ejecucion = malloc(sizeof(contexto_ejecucion));
	proceso->contexto_ejecucion->solicitud = malloc(sizeof(solicitud_instruccion));
	proceso->contexto_ejecucion->registros = malloc(sizeof(Registros));

	inicializar_registros(proceso->contexto_ejecucion->registros);

	proceso->id = procesos_creados;
	procesos_creados++;
	proceso->contexto_ejecucion->instrucciones = list_create();
	proceso->contexto_ejecucion->instrucciones = instrucciones;
	proceso->contexto_ejecucion->program_counter = 0;
	solicitar_nuevo_proceso(socket_memoria, proceso->id);

	proceso->contexto_ejecucion->tabla_segmentos = recibir_segmentos(socket_memoria); //Recibo la tabla de segmentos (segmento0)

	proceso->estimado_rafaga_proxima = configuracion->estimacion_inicial/1000;
	proceso->tiempo_llegada_ready = 0; 	// Esto esta de más
	proceso->contexto_ejecucion->estado_proceso = NUEVO;
	proceso->contexto_ejecucion->tiempo_bloqueo = 0;
	proceso->contexto_ejecucion->solicitud->instruccion = -1;
	proceso->contexto_ejecucion->solicitud->size = 0;
	proceso->contexto_ejecucion->solicitud->parametro = NULL;
	proceso->recursosAsignados = list_create();
	proceso->recursoSolicitado = string_new();
	proceso->tabla_archivos_abiertos = list_create();
	proceso->contexto_ejecucion->estado_error = SUCCESS;
}

void liberar_pcb(pcb* proceso) {
    if (proceso == NULL) {
        return;
    }

    informar_fin_consola(proceso->socket_cliente, proceso->contexto_ejecucion->estado_error);

    list_remove(lista_procesos, obtenerIndiceProceso(proceso->id));
    liberar_contexto(proceso->contexto_ejecucion);

    list_destroy(proceso->recursosAsignados);

    list_destroy(proceso->tabla_archivos_abiertos);

    free(proceso);
}


//Planificador a corto plazo
void *elegirPlanificador(){
	int utilizarFifo = strcmp(configuracion->algoritmo_planificacion, "FIFO");
	int utilizarHrrn = strcmp(configuracion->algoritmo_planificacion, "HRRN");

		if(utilizarFifo == 0){
				//Ejecutar FIFO.
			log_info(logger,"Planificador a corto plazo: FIFO.");
			strcpy(algoritmoSeleccionado,"FIFO");
			planificadorFifo();
		}

		if(utilizarHrrn == 0){
			//Ejecuta HRRN.
			log_info(logger,"Planificador a corto plazo: HRRN.");
			strcpy(algoritmoSeleccionado,"HRRN");
			planificadorHrrn();
		}

	log_info(logger,"Hubo un error al elegir el planificador");
	abort();
	return NULL; //Nunca llega hasta acá
}

void planificadorFifo(){

	while(1){
		sem_wait(&noHayProcesoEjecutando);
		sem_wait(&procesosEnReady);
			pthread_mutex_lock(&procesos_ready);
			pthread_mutex_lock(&procesos_ejecutando);


			pcb* procesoEjecutar;
			procesoEjecutar =  list_remove(estado_ready, 0);
			queue_push(estado_ejecutando, procesoEjecutar);
			cambiarEstadoProceso(procesoEjecutar, EJECUTANDO);
			//temporal_destroy(procesoEjecutar->tiempo_en_ready);

			pthread_mutex_unlock(&procesos_ready);
			pthread_mutex_unlock(&procesos_ejecutando);
		sem_post(&procesoParaEjecutar);
	}

}

void planificadorHrrn(){
	while(1){
		sem_wait(&noHayProcesoEjecutando);
		sem_wait(&procesosEnReady);
			pthread_mutex_lock(&procesos_ready);
			pthread_mutex_lock(&procesos_ejecutando);

			pcb* procesoEjecutar;

			int indice = obtener_indice_proximoProceso();
			procesoEjecutar = list_remove(estado_ready, indice);
			queue_push(estado_ejecutando, procesoEjecutar);
			cambiarEstadoProceso(procesoEjecutar, EJECUTANDO);
			temporal_destroy(procesoEjecutar->tiempo_en_ready);

			pthread_mutex_unlock(&procesos_ready);
			pthread_mutex_unlock(&procesos_ejecutando);
		sem_post(&procesoParaEjecutar);

	}
}

int obtener_indice_proximoProceso() {

t_list_iterator* iterador = list_iterator_create(estado_ready);
pcb* pcbAuxiliar;

int indiceProximoProceso = -1;
int posicionActual = 0;
double rrMaximo= -1;


	while(list_iterator_has_next(iterador)) {


		pcbAuxiliar = list_get(estado_ready, posicionActual);


		if(obtenerRR(pcbAuxiliar) > rrMaximo) {
			rrMaximo = obtenerRR(pcbAuxiliar);
			indiceProximoProceso = posicionActual;
		}

		posicionActual++;
		list_iterator_next(iterador);
	}

	list_iterator_destroy(iterador);
	if(indiceProximoProceso == -1)
	log_error(logger,"Error, indice del proceso no encontrado");

	return indiceProximoProceso;
}

double obtenerRR(pcb* unProceso) {
	double s;
	int64_t w;

	w = temporal_gettime(unProceso->tiempo_en_ready)/1000;
	s = unProceso->estimado_rafaga_proxima;

	return (w+s)/s;
}


//Planificador a largo plazo
void inicializarEstructurasColas(){
	estado_new = queue_create();
	estado_ready = list_create();
	lista_procesos = list_create();
	//
	estado_bloqueado = list_create();
	estado_ejecutando = queue_create();
	estado_exit = queue_create();
	//
	recursos_disponibles = list_create();
	//
	procesos_bloqueados_fs = queue_create();
	//
	tabla_archivos_abiertos = dictionary_create();
	//

	sem_init(&procesoParaEjecutar,0,0);
	sem_init(&noHayProcesoEjecutando,0,1);
	sem_init(&procesosEnReady,0,0);
	sem_init(&procesosFinalizados,0,0);
	sem_init(&procesosListos,0,0);
	sem_init(&procesosFileSystemBloqueado,0,0);
	pthread_mutex_init(&procesos_new,NULL);
	pthread_mutex_init(&procesos_ready,NULL);
	pthread_mutex_init(&procesos_bloqueados,NULL);
	pthread_mutex_init(&procesos_ejecutando,NULL);
	pthread_mutex_init(&procesos_exit,NULL);
	pthread_mutex_init(&recursos,NULL);
	pthread_mutex_init(&compactacion_memoria,NULL);
	pthread_mutex_init(&operando_con_fs,NULL);
	sem_init(&disponibilidadParaNuevoProceso,0,configuracion->grado_max_multiprogramacion);

	algoritmoSeleccionado = malloc(5);
	cargarRecursosDisponibles();
}

void liberarEstructurasColas(){
	queue_destroy(estado_new);
	list_destroy(estado_ready);
	list_destroy(estado_bloqueado);
	queue_destroy(estado_ejecutando);
	queue_destroy(estado_exit);

	queue_destroy(procesos_bloqueados_fs);

	queue_destroy(estado_exit);

	dictionary_destroy(tabla_archivos_abiertos);

	sem_destroy(&procesoParaEjecutar);
	sem_destroy(&procesosEnReady);
	sem_destroy(&noHayProcesoEjecutando);
	sem_destroy(&procesosFinalizados);
	sem_destroy(&procesosListos);
	sem_destroy(&procesosFileSystemBloqueado);
	pthread_mutex_destroy(&procesos_new);
	pthread_mutex_destroy(&procesos_ready);
	pthread_mutex_destroy(&procesos_bloqueados);
	pthread_mutex_destroy(&procesos_ejecutando);
	pthread_mutex_destroy(&procesos_exit);
	pthread_mutex_destroy(&recursos);
	pthread_mutex_destroy(&compactacion_memoria);
	pthread_mutex_destroy(&operando_con_fs);
}

void cargarRecursosDisponibles(void){
	int cantRecursos = string_array_size(configuracion->recursos);
	for(int i = 0; i < cantRecursos; i++){
		int* instanciaRecurso = malloc(sizeof(int));
		*instanciaRecurso = atoi(configuracion->instancias_recursos[i]);
		list_add(recursos_disponibles, instanciaRecurso);
	}
}

/*
 * Esta funcion se encarga de recibir consolas, parsea las instrucciones y las mete en estado_new
*/
void recibirConsolas(){
	while(1){
		int socket_cliente = accept(socket_servidor, NULL, NULL);
		handshake_servidor(socket_cliente, logger, KERNEL);
		if(socket_cliente>0){
			nueva_consola(socket_cliente);
		}
	}
}

void nueva_consola(int socket_cliente){

	pcb* nuevo_pcb = malloc(sizeof(pcb));

	int cod_op = recibir_operacion(socket_cliente);

	log_info(logger, "- %d -", cod_op);

	switch(cod_op){

		case MENSAJE:
			char* mensaje = recibir_mensaje(socket_cliente);
			log_info(logger, "Importante mensaje desde la consola: %s", mensaje);
			close(socket_cliente);
			break;

		case PAQUETE:
			crear_pcb(nuevo_pcb, recibir_instrucciones(socket_cliente));
			list_add(lista_procesos, nuevo_pcb);

			nuevo_pcb->socket_cliente = socket_cliente;

			pthread_mutex_lock(&procesos_new);

			queue_push(estado_new, nuevo_pcb);

			pthread_mutex_unlock(&procesos_new);
			log_info(logger, "Se crea el proceso %d en NEW",nuevo_pcb->id);
			sem_post(&procesosListos);
			break;

		default:
			log_error(logger, "Error recibiendo los datos de la consola");
			break;
	}
}

void informar_fin_consola(int socket_cliente, int estado){
	send(socket_cliente, &estado, sizeof(int), 0);
	close(socket_cliente);
}

void planificadorLargoPlazo(){

	pthread_t multiprogramacion;
	pthread_t procesosFinalizados;

	pthread_create(&multiprogramacion, NULL, (void*) manejarMultiprogramacion, NULL);
	pthread_create(&procesosFinalizados, NULL, (void*) liberarProcesosFinalizados, NULL);

	pthread_join(multiprogramacion, NULL);
	pthread_join(procesosFinalizados, NULL);

}

void calcularProximaRafaga(pcb* pcb, t_temporal* tiempoEjecucion){
	double tiempoEjecucionSegundos = (temporal_gettime(tiempoEjecucion) / 1000);
	//double alfa = configuracion->hrrn_alfa;
	double estimado_anterior = pcb->estimado_rafaga_proxima;

	pcb->estimado_rafaga_proxima = (tiempoEjecucionSegundos * configuracion->hrrn_alfa) + estimado_anterior * (1 - configuracion->hrrn_alfa);
}

void ejecutarProcesos(){
	t_temporal* tiempoEjecucion;
	bool procesoEnEjecucion = false;
	while(1){
		sem_wait(&procesoParaEjecutar);
		tiempoEjecucion = temporal_create();
		procesoEnEjecucion = true;
		pthread_mutex_lock(&procesos_ejecutando);

			while(procesoEnEjecucion){
				pcb* proceso;
				proceso = queue_pop(estado_ejecutando);
				proceso->contexto_ejecucion->solicitud->size = 0;
				t_paquete* paquete = paquete_contexto(proceso->contexto_ejecucion);
				if(proceso->id != informar_id(socket_cpu, proceso->id)) log_error(logger, "Error al enviar pid a CPU");
				enviar_paquete(paquete, socket_cpu);
				eliminar_paquete(paquete);

				contexto_ejecucion* nuevoContexto = recibir_contexto(socket_cpu, logger);

				liberar_contexto(proceso->contexto_ejecucion);

				proceso->contexto_ejecucion = nuevoContexto;

				char* param1;
				char* param2;
				char* param3;
				solicitud_instruccion* miInstruccion = proceso->contexto_ejecucion->solicitud;
				char* parametros = malloc(strlen(miInstruccion->parametro)+1);

				strcpy(parametros, miInstruccion->parametro);
				param1 = strtok(parametros, " \0");
				param2 = strtok(NULL, " \0");
				param3 = strtok(NULL, " \0");

				int indice_recurso;
				switch(proceso->contexto_ejecucion->solicitud->instruccion){
				case F_READ:
					pthread_mutex_lock(&operando_con_fs);

					informar_id(socket_fs, proceso->id);

					archivo_abierto_proceso* archivo_proceso_read = encontrar_archivo_proceso(proceso,param1);

					uint32_t direccionFisica_r = atoi(param2);
					uint32_t cantBytes_r = atoi(param3);

					log_info(logger, "PID: %d - Leer Archivo: %s - Puntero: %d - Dirección de Memoria: %d - Tamaño: %d",
							proceso->id, param1, archivo_proceso_read->puntero, direccionFisica_r, cantBytes_r);

					enviar_operacion_fs(param1, direccionFisica_r, cantBytes_r, archivo_proceso_read->puntero, LEER_ARCHIVO);

					pthread_mutex_lock(&procesos_bloqueados);
						queue_push(procesos_bloqueados_fs,proceso->id);
						cambiarEstadoProceso(proceso, BLOQUEADO);
						list_add(estado_bloqueado, proceso);
					pthread_mutex_unlock(&procesos_bloqueados);

					sem_post(&procesosFileSystemBloqueado);
					procesoEnEjecucion = false;
					pthread_mutex_unlock(&operando_con_fs);
					break;

				case F_WRITE:
					pthread_mutex_lock(&operando_con_fs);

					informar_id(socket_fs, proceso->id);

					archivo_abierto_proceso* archivo_proceso_write = encontrar_archivo_proceso(proceso,param1);

					uint32_t direccionFisica_w = atoi(param2);
					uint32_t cantBytes_w = atoi(param3);

					uint32_t puntero = archivo_proceso_write->puntero;

					log_info(logger, "PID: %d - Escribir Archivo: %s - Puntero: %d - Dirección de Memoria: %d - Tamaño: %d",
							proceso->id, param1, puntero, direccionFisica_w,cantBytes_w);

					enviar_operacion_fs(param1, direccionFisica_w, cantBytes_w, archivo_proceso_write->puntero, ESCRIBIR_ARCHIVO);

					pthread_mutex_lock(&procesos_bloqueados);
						queue_push(procesos_bloqueados_fs,proceso->id);
						cambiarEstadoProceso(proceso, BLOQUEADO);
						list_add(estado_bloqueado, proceso);
					pthread_mutex_unlock(&procesos_bloqueados);

					sem_post(&procesosFileSystemBloqueado);

					procesoEnEjecucion = false;
					pthread_mutex_unlock(&operando_con_fs);
					break;

				case F_TRUNCATE:
					pthread_mutex_lock(&operando_con_fs);

					informar_id(socket_fs, proceso->id);

					log_info(logger, "PID: %d - Archivo: %s - Tamaño: %s", proceso->id, param1, param2);
					enviar_truncate_fs(param1, atoi(param2));

					pthread_mutex_lock(&procesos_bloqueados);
						queue_push(procesos_bloqueados_fs,proceso->id);
						cambiarEstadoProceso(proceso, BLOQUEADO);
						list_add(estado_bloqueado, proceso);
					pthread_mutex_unlock(&procesos_bloqueados);

					sem_post(&procesosFileSystemBloqueado);

					procesoEnEjecucion = false;
					pthread_mutex_unlock(&operando_con_fs);
					break;

				case F_SEEK:
					archivo_abierto_proceso* archivo_proceso = encontrar_archivo_proceso(proceso,param1);
					archivo_proceso->puntero = atoi(param2);
					log_info(logger, "PID: %d - Actualizar Puntero Archivo: %s - Puntero: %d", proceso->id, param1, archivo_proceso->puntero);

					break;
				case CREATE_SEGMENT:

					crear_segmento:
					int respuestaEntera = MEMORY_COMPACTION;

					respuestaEntera = solicitar_creacion_segmento(socket_memoria, param1, param2, proceso->id);

					switch(respuestaEntera) {

				case SUCCESS:

							list_destroy_and_destroy_elements(proceso->contexto_ejecucion->tabla_segmentos, free);
							proceso->contexto_ejecucion->tabla_segmentos = recibir_segmentos(socket_memoria);

							log_info(logger, "PID: %d - Crear Segmento - Id: %s - Tamaño %s", proceso->id, param1, param2);
							break;
				case OUT_OF_MEMORY:

							proceso->contexto_ejecucion->estado_error = OUT_OF_MEMORY;
							pthread_mutex_lock(&procesos_exit);
							queue_push(estado_exit, proceso);
							pthread_mutex_unlock(&procesos_exit);
							cambiarEstadoProceso(proceso,FINALIZADO);
							sem_post(&procesosFinalizados);
							sem_post(&disponibilidadParaNuevoProceso);
							procesoEnEjecucion = false;

							break;
				case MEMORY_COMPACTION:

						if(!fs_bloqueado){
							log_info(logger, "Compactacion: Se solicito compactacion");

						} else {
							log_info(logger, "Compactacion: Esperando fin de operacion de FS");
							pthread_mutex_lock(&operando_con_fs);
						}

						log_info(logger, "Compactacion: Comenzando compactacion");

							pthread_mutex_lock(&compactacion_memoria);
							if(proceso->id != informar_id(socket_memoria, proceso->id)) log_error(logger, "Error al enviar solicitud a Memoria");
							enviar_operacion(socket_memoria, COMPACTAR_SEGMENTOS);
							pthread_mutex_unlock(&compactacion_memoria);
							asignar_tablaSegmentos();

						log_info(logger, "Se finalizo el proceso de compactacion");
						pthread_mutex_unlock(&operando_con_fs);

							goto crear_segmento;
							break;
						default:
							log_error(logger, "ERROR EN RESPUESTA DE MEMORIA");

					}


					break;
				case WAIT:
					char* nombreRecursoWait = proceso->contexto_ejecucion->solicitud->parametro;
					indice_recurso = indiceRecurso(nombreRecursoWait);
					procesoEnEjecucion = waitRecurso(indice_recurso, proceso);
					break;
				case SIGNAL:
					char* recursoSolicitado = proceso->contexto_ejecucion->solicitud->parametro;
					indice_recurso = indiceRecurso(recursoSolicitado);
					procesoEnEjecucion = signalRecurso(indice_recurso, proceso);
					break;
				case F_OPEN:
					log_info(logger, "PID: %d - ABRIR ARCHIVO: %s", proceso->id, param1);

					if(dictionary_has_key(tabla_archivos_abiertos, param1)){
						log_info(logger, "El ARCHIVO: %s ya estaba abierto, bloqueando el proceso %d", param1, proceso->id);
						archivo_abierto* archivoAbierto = dictionary_get(tabla_archivos_abiertos,param1);
						list_add(archivoAbierto->pid_proceso_espera, &proceso->id);

						archivo_abierto_proceso* archivoAbiertoProceso = malloc(sizeof(archivo_abierto_proceso));
						archivoAbiertoProceso->nombre_archivo = malloc(sizeof(param1) + 1);
						strcpy(archivoAbiertoProceso->nombre_archivo, param1);
						archivoAbiertoProceso->puntero = 0;

						list_add(proceso->tabla_archivos_abiertos,archivoAbiertoProceso);

						pthread_mutex_lock(&procesos_bloqueados);
							cambiarEstadoProceso(proceso, BLOQUEADO);
							list_add(estado_bloqueado, proceso);
						pthread_mutex_unlock(&procesos_bloqueados);

						procesoEnEjecucion = false;
					}else{
						log_info(logger, "El ARCHIVO: %s no estaba abierto", param1);
						pthread_mutex_lock(&operando_con_fs);

						administrar_archivo(param1, ABRIR_ARCHIVO, proceso->id);

						archivo_abierto_proceso* archivoAbiertoProceso = malloc(sizeof(archivo_abierto_proceso));
						archivoAbiertoProceso->nombre_archivo = malloc(strlen(param1)+1);
						strcpy(archivoAbiertoProceso->nombre_archivo, param1);
						archivoAbiertoProceso->puntero = 0;

						list_add(proceso->tabla_archivos_abiertos, archivoAbiertoProceso);
						archivo_abierto* archivo_abierto_global = malloc(sizeof(archivo_abierto));
						archivo_abierto_global->pid_proceso_espera = list_create();
						archivo_abierto_global->pid_proceso = proceso->id;
						dictionary_put(tabla_archivos_abiertos, param1, archivo_abierto_global);
						pthread_mutex_unlock(&operando_con_fs);
					}

					break;

				case F_CLOSE:
					pthread_mutex_lock(&operando_con_fs);

					archivo_abierto* archivo_tabla_global = dictionary_get(tabla_archivos_abiertos,param1);
					log_info(logger, "PID: %d - CERRAR ARCHIVO: %s", proceso->id, param1);

					int pid_proceso_desbloqueado = -1;

					if(list_is_empty(archivo_tabla_global->pid_proceso_espera)){
						dictionary_remove_and_destroy(tabla_archivos_abiertos,param1, liberar_archivo_abierto);
					}else{
						pid_proceso_desbloqueado = *(int*)list_remove(archivo_tabla_global->pid_proceso_espera,0);
						archivo_tabla_global->pid_proceso = pid_proceso_desbloqueado;

						pcb* procesoDesbloqueado = sacarProcesoBloqueadoId(pid_proceso_desbloqueado);
						pthread_mutex_lock(&procesos_ready);
							list_add(estado_ready,procesoDesbloqueado);
							cambiarEstadoProceso(procesoDesbloqueado,LISTO);
							sem_post(&procesosEnReady);
						pthread_mutex_unlock(&procesos_ready);
					}

					int indice_archivo_proceso_activo = encontrar_indice_archivo_proceso(proceso, param1);
					list_remove(proceso->tabla_archivos_abiertos, indice_archivo_proceso_activo);
					pthread_mutex_unlock(&operando_con_fs);
					break;

				case DELETE_SEGMENT:
					int segmento = atoi(param1);
					eliminar_segmento(proceso->id, segmento);

					t_list* nuevosSegmentos = recibir_segmentos(socket_memoria);
					list_destroy_and_destroy_elements(proceso->contexto_ejecucion->tabla_segmentos,free);
					proceso->contexto_ejecucion->tabla_segmentos = nuevosSegmentos;
					log_info(logger, "PID: %d - Eliminar Segmento - Id Segmento: %s", proceso->id, param1);
					break;

				case IO:
					cambiarEstadoProceso(proceso,BLOQUEADO);
					pthread_t procesoBloqueado;
					if(strcmp(configuracion->algoritmo_planificacion, "HRRN") == 0) {
						calcularProximaRafaga(proceso,tiempoEjecucion);
					}
					pthread_create(&procesoBloqueado, NULL, (void*) bloquearProcesoIO, proceso);
					pthread_detach(procesoBloqueado);
					procesoEnEjecucion = false;
					break;

				case YIELD:
					if(strcmp(configuracion->algoritmo_planificacion, "HRRN") == 0) {
						calcularProximaRafaga(proceso,tiempoEjecucion);
						}
					pthread_mutex_lock(&procesos_ready);
					list_add(estado_ready, proceso);
					cambiarEstadoProceso(proceso,LISTO);
					if(strcmp(configuracion->algoritmo_planificacion, "HRRN") == 0){
						proceso->tiempo_en_ready = temporal_create();
					}
					pthread_mutex_unlock(&procesos_ready);
					sem_post(&procesosEnReady);
					procesoEnEjecucion = false;
					break;

				case EXIT:
					mover_proceso_a_finalizado(proceso);
					procesoEnEjecucion = false;
					break;

				default:
					break;
				}

				if(procesoEnEjecucion){

					if(proceso->contexto_ejecucion->estado_error == SEG_FAULT){
						mover_proceso_a_finalizado(proceso);
						procesoEnEjecucion = false;
					}else{
						queue_push(estado_ejecutando, proceso);
					}
				}
				free(parametros);
			}

			temporal_destroy(tiempoEjecucion);
			pthread_mutex_unlock(&procesos_ejecutando);
			sem_post(&noHayProcesoEjecutando);
	}

}

void mover_proceso_a_finalizado(pcb* proceso){
	pthread_mutex_lock(&procesos_exit);
		queue_push(estado_exit, proceso);
	pthread_mutex_unlock(&procesos_exit);
	cambiarEstadoProceso(proceso,FINALIZADO);
	sem_post(&procesosFinalizados);
	sem_post(&disponibilidadParaNuevoProceso);
}

void manejarMultiprogramacion(){

	while(1){
		sem_wait(&procesosListos);
		sem_wait(&disponibilidadParaNuevoProceso);
		pthread_mutex_lock(&procesos_new);
				pcb* pcbAEncolar = queue_pop(estado_new);
				pthread_mutex_unlock(&procesos_new);

				pthread_mutex_lock(&procesos_ready);
					if(strcmp(configuracion->algoritmo_planificacion, "HRRN") == 0) {
						pcbAEncolar->tiempo_en_ready = temporal_create();
					}
					list_add(estado_ready, pcbAEncolar);
					cambiarEstadoProceso(pcbAEncolar, LISTO);
				pthread_mutex_unlock(&procesos_ready);
				sem_post(&procesosEnReady);
	}
}

void liberarProcesosFinalizados(){
	while(1){
		sem_wait(&procesosFinalizados);
		pthread_mutex_lock(&procesos_exit);
			pcb* procesoFinalizado = queue_pop(estado_exit);
			char* mensaje_error = codigo_error(procesoFinalizado->contexto_ejecucion->estado_error);
			log_info(logger, "Finaliza el proceso %d - MOTIVO: %s", procesoFinalizado->id, mensaje_error);
			free(mensaje_error);
			eliminar_proceso(procesoFinalizado->id);
			liberar_pcb(procesoFinalizado);
		pthread_mutex_unlock(&procesos_exit);
	}
}

void liberarProcesoFinalizadoPorError(pcb* proceso, char* motivo){
	log_info(logger, "Finaliza el proceso %d - MOTIVO: %s", proceso->id,motivo);
	liberar_pcb(proceso);
}

void cambiarEstadoProceso(pcb* pcb, int nuevoEstado){
	estado unEstado = nuevoEstado;
	log_info(logger,"PID: %d - Estado Anterior: %s - Estado Actual: %s",pcb->id,nombreEstado(pcb->contexto_ejecucion->estado_proceso),nombreEstado(unEstado));
	pcb->contexto_ejecucion->estado_proceso = nuevoEstado;
	if(nuevoEstado == LISTO){
		log_info(logger,"Cola Ready %s: %s",algoritmoSeleccionado, PIDsEnReady());
	}
}

void bloquearProcesoIO(pcb* pcb){
	//Supongo son segundos
	log_info(logger,"PID: %d - Bloqueado por: I/O",pcb->id);
	log_info(logger,"PID: %d - Ejecuta IO: %d",pcb->id,pcb->contexto_ejecucion->tiempo_bloqueo);
	sleep(pcb->contexto_ejecucion->tiempo_bloqueo);
	pthread_mutex_lock(&procesos_ready);
		list_add(estado_ready, pcb);
		pcb->tiempo_en_ready = temporal_create();
		cambiarEstadoProceso(pcb, LISTO);
	pthread_mutex_unlock(&procesos_ready);
	sem_post(&procesosEnReady);
}

void bloquearProcesoPorRecurso(pcb* pcb){
	log_info(logger,"PID: %d - Bloqueado por: Recurso",pcb->id);
	list_add(estado_bloqueado, pcb);
	cambiarEstadoProceso(pcb, BLOQUEADO);
}

bool hayRecursoDisponible(int indiceRecursoSolicitado){
	return *(int*)list_get(recursos_disponibles,indiceRecursoSolicitado) > 0;
}

int indiceRecurso(char* recursoSolicitado){
	int cantRecursos = string_array_size(configuracion->recursos);
	for(int i = 0; i < cantRecursos; i++){
		if(string_equals_ignore_case(configuracion->recursos[i],recursoSolicitado)){
			return i;
		}
	}
	//log_error(logger,"Error, el recurso");
	return -1;
}

char* obtenerNombreRecurso(int id_recurso){
	return configuracion->recursos[id_recurso];
}

bool waitRecurso(int indice_recurso, pcb* proceso){
	if(indice_recurso == -1){
		proceso->contexto_ejecucion->estado_error = WAIT_INEXISTENTE;
		liberarProcesoFinalizadoPorError(proceso, "WAIT a recurso no existente");
		return false;
	}else{
		int* cantRecursosDisponibles = (int*)list_get(recursos_disponibles, indice_recurso);
		*cantRecursosDisponibles = *cantRecursosDisponibles - 1;
		list_replace(recursos_disponibles, indice_recurso, cantRecursosDisponibles);
		log_info(logger,"PID: %d - Wait: %s - Instancias: %d",proceso->id,obtenerNombreRecurso(indice_recurso),*(int*)list_get(recursos_disponibles,indice_recurso));
		if(*cantRecursosDisponibles >= 0){
			list_add(proceso->recursosAsignados,&indice_recurso);
			return true;
		}else{
			strcpy(proceso->recursoSolicitado, obtenerNombreRecurso(indice_recurso));
			bloquearProcesoPorRecurso(proceso);
			return false;
		}
	}

}

void liberarRecursoBloqueado(int indice_recurso_signal){
	int procesosBloqueados = list_size(estado_bloqueado);
	int index = 0;
	while(index < procesosBloqueados){
		if(indiceRecurso(((pcb*)list_get(estado_bloqueado,index))->recursoSolicitado) == indice_recurso_signal){
			pthread_mutex_lock(&procesos_bloqueados);
				pcb* proceso = list_remove(estado_bloqueado,index);
			pthread_mutex_unlock(&procesos_bloqueados);
			pthread_mutex_lock(&procesos_ready);
				list_add(estado_ready,proceso);
				cambiarEstadoProceso(proceso,LISTO);
			pthread_mutex_unlock(&procesos_ready);
			sem_post(&procesosEnReady);
			index = procesosBloqueados;
			proceso = NULL;
		}
		index++;
	}
}

bool signalRecurso(int indice_recurso_signal, pcb* proceso){
	if(indice_recurso_signal == -1){
		proceso->contexto_ejecucion->estado_error = SIGNAL_INEXISTENTE;
		liberarProcesoFinalizadoPorError(proceso, "SIGNAL a recurso no existente");
		return false;
	}else{
		int* cantRecursosDisponibles;
		cantRecursosDisponibles = (int*)list_get(recursos_disponibles, indice_recurso_signal);
		*cantRecursosDisponibles = *cantRecursosDisponibles + 1;
		log_info(logger,"PID: %d - Signal: %s - Instancias: %d",proceso->id,obtenerNombreRecurso(indice_recurso_signal),*(int*)list_get(recursos_disponibles,indice_recurso_signal));
		if(*cantRecursosDisponibles <= 0){
			liberarRecursoBloqueado(indice_recurso_signal);
		}
		return true;
	}
}

const char* PIDsEnReady(){
	int cantProcesos = list_size(estado_ready);
	char* PIDenReady = string_new();
	string_append(&PIDenReady,"[ ");
	for(int i = 0; i < cantProcesos; i++){
		string_append(&PIDenReady, string_itoa(((pcb*)list_get(estado_ready,i))->id));
		string_append(&PIDenReady, " ");
	}
	string_append(&PIDenReady,"]");
	return PIDenReady;
}

pcb* sacarProcesoBloqueadoId(int pid){
	int procesosBloqueados = list_size(estado_bloqueado);
	int index = 0;
	while(index < procesosBloqueados){

		pthread_mutex_lock(&procesos_bloqueados);
		if(((pcb*)list_get(estado_bloqueado,index))->id == pid){
			pcb* proceso = list_remove(estado_bloqueado,index);
			pthread_mutex_unlock(&procesos_bloqueados);
			return proceso;
		}
		pthread_mutex_unlock(&procesos_bloqueados);
		index++;
	}

	log_error(logger,"FileSystem terminó de ejecutar un proceso no bloqueado");
	abort();
}

void gestionar_fileSystem(){

	fs_bloqueado = false;

	while(1){

		sem_wait(&procesosFileSystemBloqueado);
		log_info(logger,"Se activó la espera a la respuesta de fs");

		int operaciones_fs;
		sem_getvalue(&procesosFileSystemBloqueado, &operaciones_fs);
		operaciones_fs++;

		if(operaciones_fs == 1 && !fs_bloqueado){
			pthread_mutex_lock(&compactacion_memoria);
			pthread_mutex_lock(&operando_con_fs);
			fs_bloqueado = true;
		}

		if(recibir_operacion(socket_fs) != SUCCESS){
			log_error(logger, "No recibi SUCCESS de FS");
		}

		pcb* proceso;

		int pid = queue_pop(procesos_bloqueados_fs);
		proceso = sacarProcesoBloqueadoId(pid);

		pthread_mutex_lock(&procesos_ready);
			list_add(estado_ready, proceso);
			cambiarEstadoProceso(proceso,LISTO);
		pthread_mutex_unlock(&procesos_ready);

		sem_post(&procesosEnReady);

		operaciones_fs--;
		if(operaciones_fs == 0){
			log_info(logger, "Ya se recibió una respuesta de fs");
			pthread_mutex_unlock(&operando_con_fs);
			pthread_mutex_unlock(&compactacion_memoria);
			fs_bloqueado = false;
		}
	}
}

archivo_abierto_proceso* encontrar_archivo_proceso(pcb* proceso, char* nombre_archivo) {
	int tamanio = list_size(proceso->tabla_archivos_abiertos);
	for(int i = 0; i < tamanio; tamanio++){
		archivo_abierto_proceso* archivo_abierto = list_get(proceso->tabla_archivos_abiertos,i);
		if(strcmp(archivo_abierto->nombre_archivo,nombre_archivo) == 0){
			return archivo_abierto;
		}
	}

	log_error(logger,"El proceso no tiene abierto ese archivo");
	abort();
}

int encontrar_indice_archivo_proceso(pcb* proceso, char* nombre_archivo){
	t_list_iterator* iterador = list_iterator_create(proceso->tabla_archivos_abiertos);
		int posicion = 0;
		archivo_abierto_proceso* archivo_proceso;

		while(list_iterator_has_next(iterador)) {

			archivo_proceso = list_get(proceso->tabla_archivos_abiertos, posicion);

			if(strcmp(archivo_proceso->nombre_archivo, nombre_archivo) == 0) {
				list_iterator_destroy(iterador);
				return posicion;
			}

			posicion ++;
		}
		list_iterator_destroy(iterador);


		return -1;
}

int obtenerIndiceProceso(uint32_t id) {

	t_list_iterator* iterador = list_iterator_create(lista_procesos);
	pcb* nuevoPcb;

	while(list_iterator_has_next(iterador)) {

		nuevoPcb = list_iterator_next(iterador);

		if(nuevoPcb->id == id) {


			int indice = list_iterator_index(iterador);
			list_iterator_destroy(iterador);

			return indice;
		}
	}
	list_iterator_destroy(iterador);
	return -1;
}

void asignar_tablaSegmentos() {
	t_list* lista_segmentos = recibir_lista_procesos(socket_memoria);
	t_list_iterator* iterador = list_iterator_create(lista_procesos);
	pcb* pcbAuxiliar;
	Proceso* procesoAuxiliar;

	while(list_iterator_has_next(iterador)){

		pcbAuxiliar = list_iterator_next(iterador);
		list_destroy_and_destroy_elements(pcbAuxiliar->contexto_ejecucion->tabla_segmentos, free);
		procesoAuxiliar = extraer_proceso_por_id(lista_segmentos, pcbAuxiliar->id);
		pcbAuxiliar->contexto_ejecucion->tabla_segmentos = procesoAuxiliar->tablaDeSegmentos;
	}

	list_destroy(lista_segmentos);
	list_iterator_destroy(iterador);
}

