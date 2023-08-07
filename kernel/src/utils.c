#include "utils.h"

//Definiciones de variables globales utilizadas dentro de todo el kernel
t_config *config;
Configuracion* configuracion;
t_log *logger;
config_conex* config_valores_memoria;
config_conex* config_valores_filesystem;
config_conex* config_valores_cpu;
t_list* lista_procesos;
int socket_memoria;
int socket_cpu;
int socket_fs;

//DUDA: INSTANCIO UN NUEVO PCB?
pcb* pcb_kernel;

void leer_config(){

	configuracion =  malloc(sizeof(Configuracion));

	config_valores_memoria = malloc(sizeof(config_conex));
	config_valores_filesystem = malloc(sizeof(config_conex));
	config_valores_cpu = malloc(sizeof(config_conex));

	//Puertos
	config_valores_memoria->ip = config_get_string_value(config, "IP_MEMORIA");
	config_valores_memoria->puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	config_valores_filesystem->ip = config_get_string_value(config, "IP_FILESYSTEM");
	config_valores_filesystem->puerto= config_get_string_value(config, "PUERTO_FILESYSTEM");
	config_valores_cpu->ip = config_get_string_value(config, "IP_CPU");
	config_valores_cpu->puerto = config_get_string_value(config, "PUERTO_CPU");
	configuracion->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	//PCB
	configuracion->algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	configuracion->estimacion_inicial = config_get_int_value(config, "ESTIMACION_INICIAL");
	configuracion->hrrn_alfa = config_get_double_value(config, "HRRN_ALFA");
	configuracion->grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	configuracion->recursos = config_get_array_value(config, "RECURSOS");
	configuracion->instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
}

void conectarse_memoria() {

	log_info(logger,"Intentando conectar con Memoria");
	socket_memoria = socket_conectarse_servidor(config_valores_memoria->ip, config_valores_memoria->puerto, logger);
	if(socket_memoria == -1) log_error(logger, "Error al establecer conexion con Memoria");
}

void conectarse_cpu() {
	log_info(logger,"Intentando conectar con CPU");
	socket_cpu = socket_conectarse_servidor(config_valores_cpu->ip, config_valores_cpu->puerto, logger);
	if(socket_cpu == -1) log_error(logger, "Error al establecer conexion con CPU");
}

void conectarse_fileSystem() {

	log_info(logger,"Intentando conectar con FileSystem");
	socket_fs = socket_conectarse_servidor(config_valores_filesystem->ip, config_valores_filesystem->puerto, logger);
	if(socket_fs == -1) log_info(logger, "Error al establecer conexion con FileSystem");
}

void liberar_archivo_abierto(void* archivo){

	archivo_abierto* archivo_casteado = (archivo_abierto*) archivo;

	list_destroy(archivo_casteado->pid_proceso_espera);

	free(archivo_casteado);
}

void terminar_programa(){

	log_info(logger, "Finalizando programa");

	config_destroy(config);

	log_destroy(logger);

	free(config_valores_memoria);
	free(config_valores_filesystem);
	free(config_valores_cpu);

}

t_list *recibir_instrucciones(int socket_cliente){

	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	t_list *lista_instrucciones = list_create();

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	deserializar_instrucciones(paquete->buffer, lista_instrucciones);

	log_info(logger, "Recibidas %d instrucciones | BUFFER_SIZE: - %d -", list_size(lista_instrucciones) ,paquete->buffer->size);

	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
	return lista_instrucciones;
}

void eliminar_segmento(int id_proceso, int id_segmento){

	if(id_proceso != informar_id(socket_memoria, id_proceso)) log_error(logger, "Error al enviar solicitud a Memoria"); //TODO: Mejorar log
	enviar_operacion(socket_memoria, ELIMINAR_SEGMENTO);
	send(socket_memoria, &id_segmento, sizeof(int), 0);
}

void eliminar_proceso(int id){
	if(id != informar_id(socket_memoria, id)) log_error(logger, "Error al enviar solicitud a Memoria (id)"); //TODO: Mejorar log
	enviar_operacion(socket_memoria, FINALIZAR_PROCESO);
}

void enviar_operacion_fs(char* nombreArchivo, uint32_t direccionFisica, uint32_t cant_bytes, uint32_t puntero, int cod_op){

	t_paquete* paquete = crear_paquete();
	int desplazamiento = 0;
	int tamanioNombre = strlen(nombreArchivo) + 1;

	paquete->codigo_operacion = cod_op;

	paquete->buffer->size = sizeof(uint32_t) * 3 + tamanioNombre + sizeof(int);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &tamanioNombre, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, nombreArchivo, tamanioNombre);
	desplazamiento += tamanioNombre;
	memcpy(paquete->buffer->stream + desplazamiento, &cant_bytes, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &direccionFisica, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &puntero, sizeof(uint32_t));

	enviar_paquete(paquete, socket_fs);

	eliminar_paquete(paquete);
}

void enviar_truncate_fs(char* nombreArchivo, uint32_t nuevoTamanio){

	t_paquete* paquete = crear_paquete();

	int desplazamiento = 0;
	int tamanioNombre = strlen(nombreArchivo) + 1;

	paquete->codigo_operacion = TRUNCAR_ARCHIVO;

	paquete->buffer->size = sizeof(uint32_t) + tamanioNombre + sizeof(int);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &tamanioNombre, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, nombreArchivo, tamanioNombre);
	desplazamiento += tamanioNombre;
	memcpy(paquete->buffer->stream + desplazamiento, &nuevoTamanio, sizeof(uint32_t));

	enviar_paquete(paquete, socket_fs);

	eliminar_paquete(paquete);
}

void administrar_archivo(char* nombreArchivo, int cod_op, int proccesID){

	informar_id(socket_fs, proccesID);

	t_paquete* paquete = crear_paquete();

	int tamanioNombre = strlen(nombreArchivo) + 1;

	paquete->codigo_operacion = cod_op;

	paquete->buffer->size = tamanioNombre;

	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, nombreArchivo, tamanioNombre);

	enviar_paquete(paquete, socket_fs);

	eliminar_paquete(paquete);

	if(recibir_operacion(socket_fs) == ARCHIVO_NO_EXISTE){
		administrar_archivo(nombreArchivo, CREAR_ARCHIVO,proccesID);
	}
}
