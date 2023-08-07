#include "comunicacion.h"

void deserializar_instrucciones(t_buffer* buffer, t_list *lista_instrucciones){

	void* stream = buffer->stream;

	int desplazamiento = 0;

	while(desplazamiento < buffer->size){
	Instruccion *instruccion = malloc(sizeof(Instruccion));

	memcpy(&(instruccion->instruccion), stream + desplazamiento, sizeof(instruction_index));
	desplazamiento += sizeof(instruction_index);
	memcpy(&(instruccion->parametros_long), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	instruccion->parametros = malloc(instruccion->parametros_long);
	memcpy(instruccion->parametros, stream + desplazamiento, instruccion->parametros_long);
	desplazamiento += instruccion->parametros_long;

	list_add(lista_instrucciones, instruccion);
	}
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

t_paquete* crear_mensaje(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = MENSAJE;
	crear_buffer(paquete);
	return paquete;
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

t_paquete *paquete_contexto(contexto_ejecucion *contexto){
	t_paquete* paquete = crear_paquete();

	contexto->instrucciones_size = instruction_size(contexto->instrucciones);

	contexto->segmentos_size = list_size(contexto->tabla_segmentos) * sizeof(uint32_t) * 3;

	int contexto_size = sizeof(uint32_t) * 5 +
						sizeof(instruction_index) +
						sizeof(estado) +
						contexto->instrucciones_size +
						sizeof(char) * 112 +
						contexto->segmentos_size +
						contexto->solicitud->size +
						sizeof(int); //Tamaño del contexto


	agregar_a_paquete(paquete->buffer, contexto, contexto_size);

	return paquete;
}


void agregar_a_paquete(t_buffer* buffer, contexto_ejecucion *valor, int tamanio)
{
	buffer->stream = malloc(tamanio);

	int desplazamiento = 0;

	memcpy(buffer->stream + desplazamiento, &valor->instrucciones_size, sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	desplazamiento += serializar_instrucciones(valor->instrucciones, buffer, desplazamiento);
	memcpy(buffer->stream + desplazamiento, &valor->program_counter, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	desplazamiento += serializar_registros(valor->registros, buffer, desplazamiento);
	memcpy(buffer->stream + desplazamiento, &valor->segmentos_size, sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	desplazamiento += serializar_segmentos(valor->tabla_segmentos, buffer, desplazamiento);
	memcpy(buffer->stream + desplazamiento, &valor->estado_proceso, sizeof(estado));
	desplazamiento += sizeof(estado);
	memcpy(buffer->stream + desplazamiento, &valor->tiempo_bloqueo, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, &valor->solicitud->instruccion, sizeof(instruction_index));
	desplazamiento += sizeof(instruction_index);
	memcpy(buffer->stream + desplazamiento, &valor->solicitud->size, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, valor->solicitud->parametro, valor->solicitud->size);
	desplazamiento += valor->solicitud->size;
	memcpy(buffer->stream + desplazamiento, &valor->estado_error, sizeof(int));
	desplazamiento += valor->estado_error;

	buffer->size = tamanio;
}

int serializar_instrucciones(t_list *instrucciones, t_buffer *buffer, int desplazamiento){

	int size = 0;
	t_list_iterator *iterador = list_iterator_create(instrucciones);

	while(list_iterator_has_next(iterador)){
		Instruccion *instruccion = list_iterator_next(iterador);
		int tamanio = sizeof(instruction_index) + sizeof(uint32_t) + strlen(instruccion->parametros) + 1;
		size += tamanio;
		memcpy(buffer->stream + desplazamiento, &instruccion->instruccion, sizeof(instruction_index));
		desplazamiento+=sizeof(instruction_index);
		memcpy(buffer->stream + desplazamiento, &instruccion->parametros_long, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer->stream + desplazamiento, instruccion->parametros, instruccion->parametros_long);
		desplazamiento += instruccion->parametros_long;
	}
	list_iterator_destroy(iterador);
	return size;
}

void agregar_instruccion(t_buffer* buffer, Instruccion *valor, int tamanio)
{
	buffer->stream = realloc(buffer->stream, buffer->size + tamanio);

	int desplazamiento = buffer->size;
	memcpy(buffer->stream + desplazamiento, &valor->instruccion, sizeof(instruction_index));
	desplazamiento+=sizeof(instruction_index);
	memcpy(buffer->stream + desplazamiento, &valor->parametros_long, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, valor->parametros, valor->parametros_long);

	buffer->size += tamanio;
}

int instruction_size(t_list *instrucciones){
	int size = 0;

	t_list_iterator *iterador = list_iterator_create(instrucciones);

	while(list_iterator_has_next(iterador)){
		Instruccion *instruccion = list_iterator_next(iterador);
		int tamanio = sizeof(instruction_index) + sizeof(uint32_t) + strlen(instruccion->parametros) + 1;
		size += tamanio;
	}
	list_iterator_destroy(iterador);
	return size;
}

int serializar_registros(Registros* registros, t_buffer *buffer, int desplazamiento){

	int size = sizeof(char) * 112; //4*4 + 4*8 + 4*16

	memcpy(buffer->stream + desplazamiento, registros->AX , sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy(buffer->stream + desplazamiento, registros->BX , sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy(buffer->stream + desplazamiento, registros->CX , sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy(buffer->stream + desplazamiento, registros->DX , sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy(buffer->stream + desplazamiento, registros->EAX , sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy(buffer->stream + desplazamiento, registros->EBX , sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy(buffer->stream + desplazamiento, registros->ECX , sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy(buffer->stream + desplazamiento, registros->EDX , sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy(buffer->stream + desplazamiento, registros->RAX , sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy(buffer->stream + desplazamiento, registros->RBX , sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy(buffer->stream + desplazamiento, registros->RCX , sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy(buffer->stream + desplazamiento, registros->RDX , sizeof(char) * 16);

	return size;

}

int serializar_segmentos(t_list *segmentos, t_buffer *buffer, int desplazamiento){
	int size = 0;
	int tamanio = sizeof(uint32_t) * 3;

	t_list_iterator *iterador = list_iterator_create(segmentos);

	while(list_iterator_has_next(iterador)){
		segmento *segmento = list_iterator_next(iterador);
		agregar_segmento(segmento, buffer, tamanio, desplazamiento + size);
		size += tamanio;
	}
	list_iterator_destroy(iterador);

	return size;
}

void agregar_segmento(segmento *segmento, t_buffer *buffer, int tamanio, int desplazamiento){

	memcpy(buffer->stream + desplazamiento, &segmento->id, sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, &segmento->direccion_base, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, &segmento->tamanio_segmento, sizeof(uint32_t));

	buffer->size += tamanio;
}

void enviar_paquete(t_paquete* paquete, int socket)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * stream = malloc(bytes);
	int desplazamiento = 0;

	memcpy(stream + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(stream + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(stream + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return stream;
}

void deserializar_contexto(t_buffer *buffer, contexto_ejecucion* contexto){
	void* stream = buffer->stream;

	int desplazamiento = 0;

	memcpy(&(contexto->instrucciones_size), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	cargar_instrucciones(buffer, contexto->instrucciones, contexto->instrucciones_size, desplazamiento);
	desplazamiento += contexto->instrucciones_size;

	memcpy(&(contexto->program_counter), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//contexto->registros = malloc(sizeof(Registros));
	cargar_registros(buffer, contexto->registros, desplazamiento);
	desplazamiento += sizeof(char) * 112;

	memcpy(&(contexto->segmentos_size), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	cargar_segmentos(buffer, contexto->tabla_segmentos, contexto->segmentos_size, desplazamiento);
	desplazamiento += contexto->segmentos_size;

	memcpy(&(contexto->estado_proceso), stream + desplazamiento, sizeof(estado));
	desplazamiento += sizeof(estado);

	memcpy(&(contexto->tiempo_bloqueo), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(contexto->solicitud->instruccion), stream + desplazamiento, sizeof(instruction_index));
	desplazamiento += sizeof(instruction_index);

	memcpy(&(contexto->solicitud->size), stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	contexto->solicitud->parametro = malloc(contexto->solicitud->size);
	memcpy(contexto->solicitud->parametro, stream + desplazamiento, contexto->solicitud->size);
	desplazamiento += contexto->solicitud->size;

	memcpy(&(contexto->estado_error), stream + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
}

void cargar_instrucciones(t_buffer *instrucciones, t_list *lista_instrucciones, uint32_t instrucciones_size, int desplazamiento){

	instrucciones_size += desplazamiento;

	while(desplazamiento < instrucciones_size){
		Instruccion *instruccion = malloc(sizeof(Instruccion));
		memcpy(&(instruccion->instruccion), instrucciones->stream + desplazamiento, sizeof(instruction_index));
		desplazamiento += sizeof(instruction_index);
		memcpy(&(instruccion->parametros_long), instrucciones->stream + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		instruccion->parametros = malloc(instruccion->parametros_long);
		memcpy(instruccion->parametros, instrucciones->stream + desplazamiento, instruccion->parametros_long);
		desplazamiento += instruccion->parametros_long;

		list_add(lista_instrucciones, instruccion);
	}
}

void cargar_registros(t_buffer *buffer, Registros *registros, int desplazamiento){

	memcpy(registros->AX , buffer->stream + desplazamiento, sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy(registros->BX , buffer->stream + desplazamiento, sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy((registros->CX) , buffer->stream + desplazamiento, sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy((registros->DX), buffer->stream + desplazamiento, sizeof(char) * 4);
	desplazamiento+=sizeof(char) * 4;
	memcpy((registros->EAX) , buffer->stream + desplazamiento, sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy((registros->EBX) , buffer->stream + desplazamiento, sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy((registros->ECX) , buffer->stream + desplazamiento, sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy((registros->EDX) , buffer->stream + desplazamiento, sizeof(char) * 8);
	desplazamiento+=sizeof(char) * 8;
	memcpy((registros->RAX) , buffer->stream + desplazamiento, sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy((registros->RBX) , buffer->stream + desplazamiento, sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy((registros->RCX) , buffer->stream + desplazamiento, sizeof(char) * 16);
	desplazamiento+=sizeof(char) * 16;
	memcpy((registros->RDX) , buffer->stream + desplazamiento, sizeof(char) * 16);
}

void cargar_segmentos(t_buffer *buffer, t_list *lista_segmentos, uint32_t segmentos_size, int desplazamiento){

	segmentos_size += desplazamiento;

	while(desplazamiento < segmentos_size){
		segmento *nuevoSegmento = malloc(sizeof(segmento));

		memcpy(&(nuevoSegmento->id), buffer->stream + desplazamiento, sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);
		memcpy(&nuevoSegmento->direccion_base, buffer->stream + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(&nuevoSegmento->tamanio_segmento, buffer->stream + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		list_add(lista_segmentos, nuevoSegmento);
	}
}

contexto_ejecucion* recibir_contexto(int socket, t_log *logger){
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	recv(socket, &(paquete->codigo_operacion), sizeof(op_code), 0);

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	log_info(logger, "Ingresando nuevo contexto");

	paquete->buffer->stream = malloc(paquete->buffer->size);

	contexto_ejecucion *contexto = malloc(sizeof(contexto_ejecucion));
	contexto->registros = malloc(sizeof(Registros));
	contexto->solicitud = malloc(sizeof(solicitud_instruccion));
	contexto->instrucciones = list_create();
	contexto->tabla_segmentos = list_create();

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

	log_info(logger, "Recibido contexto | BUFFER_SIZE: - %d -", paquete->buffer->size);

	switch(paquete->codigo_operacion){
		case MENSAJE:
			break;
		case PAQUETE:
			deserializar_contexto(paquete->buffer, contexto);
			break;
		default: abort();
		}
	log_info(logger, "Generado nuevo contexto");
	eliminar_paquete(paquete);
	return contexto;
}

void liberar_instruccion(void* instruccionVoid){

	Instruccion* instruccion = (Instruccion*)instruccionVoid;

	free(instruccion->parametros);

	free(instruccion);
}

void liberar_archivo(void* instruccionVoid){

	archivo_abierto_proceso* instruccion = (archivo_abierto_proceso*)instruccionVoid;

	free(instruccion->nombre_archivo);

	free(instruccion);
}

void liberar_contexto(contexto_ejecucion* contexto) {

    if (contexto == NULL) {
        return;
    }

    liberar_registros(contexto->registros);
    if(contexto->solicitud->size > 0 ) free(contexto->solicitud->parametro);

    free(contexto->solicitud);
    list_destroy_and_destroy_elements(contexto->instrucciones, liberar_instruccion);
    list_destroy_and_destroy_elements(contexto->tabla_segmentos, free);

    free(contexto);

}

char* nombreEstado(int estado){
	switch (estado){
	case NUEVO:
		return "NUEVO"; break;
	case LISTO:
		return "LISTO"; break;
	case EJECUTANDO:
		return "EJECUTANDO"; break;
	case BLOQUEADO:
		return "BLOQUEADO"; break;
	case FINALIZADO:
		return "FINALIZADO"; break;
	default:
		abort();
	}
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = crear_mensaje();

	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void enviar_mensaje_con_tamanio(char* mensaje, int socket_cliente, int tamanio){
	t_paquete* paquete = crear_mensaje();

	paquete->buffer->size = tamanio;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

char *recibir_mensaje(int socket){
	int size;
	recv(socket, &size, sizeof(int), MSG_WAITALL);
	char *cadena = malloc(size);
	recv(socket, cadena, size, MSG_WAITALL);
	return cadena;
}

int recibir_operacion(int socket)
{
	int cod_op;
	if(recv(socket, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		return -1;
	}
}

void inicializar_registros(Registros* registros) {

	memset(registros->AX, '0', 4 * sizeof(char));
	memset(registros->BX, '0', 4 * sizeof(char));
	memset(registros->CX, '0', 4 * sizeof(char));
	memset(registros->DX, '0', 4 * sizeof(char));
	memset(registros->EAX, '0', 8 * sizeof(char));
	memset(registros->EBX, '0', 8 * sizeof(char));
	memset(registros->ECX, '0', 8 * sizeof(char));
	memset(registros->EDX, '0', 8 * sizeof(char));
	memset(registros->RAX, '0', 16 * sizeof(char));
	memset(registros->RBX, '0', 16 * sizeof(char));
	memset(registros->RCX, '0', 16 * sizeof(char));
	memset(registros->RDX, '0', 16 * sizeof(char));

}

void enviar_operacion(int socket, int operacion){
	send(socket, &operacion, sizeof(int), 0);
}

void enviar_segmentos(int socket, t_list *segmentos){
	t_paquete* paquete = crear_paquete();
	paquete->buffer->size = list_size(segmentos) * sizeof(uint32_t) * 3;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	serializar_segmentos(segmentos, paquete->buffer, 0);
	paquete->buffer->size = list_size(segmentos) * sizeof(uint32_t) * 3;
	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

t_list* recibir_segmentos(int socket){
	t_list *segmentos = list_create();
	t_paquete* paquete = crear_paquete();

	recv(socket, &(paquete->codigo_operacion), sizeof(op_code), 0);

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

	cargar_segmentos(paquete->buffer, segmentos, paquete->buffer->size, 0);

	eliminar_paquete(paquete);

	return segmentos;
}

void enviar_lista_procesos(int socket, t_list* lista_procesos){
	t_paquete* paquete = crear_paquete();

	paquete->buffer->size = procesos_size(lista_procesos);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	serializar_procesos(lista_procesos, paquete->buffer);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

int procesos_size(t_list* lista_procesos){
	t_list_iterator* iterador = list_iterator_create(lista_procesos);
	int size = 0;

	while(list_iterator_has_next(iterador)){
		Proceso* proceso = list_iterator_next(iterador);
		proceso->size = list_size(proceso->tablaDeSegmentos) * sizeof(uint32_t) * 3;
		size += sizeof(int) * 2 + proceso->size;
	}
	list_iterator_destroy(iterador);
	return size;
}

void serializar_procesos(t_list* lista_procesos, t_buffer* buffer){
	t_list_iterator* iterador = list_iterator_create(lista_procesos);
	int desplazamiento = 0;

	while(list_iterator_has_next(iterador)){
		Proceso* proceso = list_iterator_next(iterador);
		memcpy(buffer->stream + desplazamiento, &proceso->idProceso, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(buffer->stream + desplazamiento, &proceso->size, sizeof(int));
		desplazamiento+=sizeof(int);

		t_list_iterator* iterador_segmentos = list_iterator_create(proceso->tablaDeSegmentos);

		while(list_iterator_has_next(iterador_segmentos)){
			segmento* segmento = list_iterator_next(iterador_segmentos);
			memcpy(buffer->stream + desplazamiento, &segmento->id, sizeof(uint32_t));
			desplazamiento+=sizeof(uint32_t);
			memcpy(buffer->stream + desplazamiento, &segmento->direccion_base, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(buffer->stream + desplazamiento, &segmento->tamanio_segmento, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
		}
	}
}

t_list* recibir_lista_procesos(int socket){
	t_list *procesos = list_create();
	t_paquete* paquete = crear_paquete();
	recv(socket, &(paquete->codigo_operacion), sizeof(op_code), 0);

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

	deserializar_procesos(paquete->buffer, procesos);

	eliminar_paquete(paquete);

	return procesos;
}

void deserializar_procesos(t_buffer* buffer, t_list* procesos){

	int desplazamiento = 0;
	while(desplazamiento < buffer->size){
		Proceso* nuevoProceso = malloc(sizeof(Proceso));
		nuevoProceso->tablaDeSegmentos = list_create();
		memcpy(&(nuevoProceso->idProceso), buffer->stream + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		memcpy(&(nuevoProceso->size), buffer->stream + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		int desplazamiento_segmentos = 0;
		while(desplazamiento_segmentos < nuevoProceso->size){
			segmento* nuevoSegmento = malloc(sizeof(segmento));
			memcpy(&(nuevoSegmento->id), buffer->stream + desplazamiento_segmentos + desplazamiento, sizeof(uint32_t));
			desplazamiento_segmentos += sizeof(uint32_t);
			memcpy(&(nuevoSegmento->direccion_base), buffer->stream + desplazamiento_segmentos + desplazamiento, sizeof(uint32_t));
			desplazamiento_segmentos += sizeof(uint32_t);
			memcpy(&(nuevoSegmento->tamanio_segmento), buffer->stream + desplazamiento_segmentos + desplazamiento, sizeof(uint32_t));
			desplazamiento_segmentos += sizeof(uint32_t);
			list_add(nuevoProceso->tablaDeSegmentos, nuevoSegmento);
		}
		desplazamiento+=desplazamiento_segmentos;
		list_add(procesos, nuevoProceso);
	}
}

int solicitar_creacion_segmento(int socket, char* char_id, char* char_tamanio, int idProceso){

	if(idProceso != informar_id(socket, idProceso)); //TODO: log de error

	t_paquete* paquete = crear_paquete();
	int resultado = -1;

	int id = atoi(char_id);
	int tamanio = atoi(char_tamanio);

	paquete->codigo_operacion = CREAR_SEGMENTO;

	paquete->buffer->size = 2 * sizeof(int);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &id, sizeof(int));
	memcpy(paquete->buffer->stream + sizeof(int), &tamanio, sizeof(int));

	enviar_paquete(paquete, socket);

	eliminar_paquete(paquete);

	resultado = recibir_operacion(socket);

	return resultado;
}

segmento* recibir_solicitud_segmento(int socket){
	segmento* nuevoSegmento = malloc(sizeof(segmento));
	t_paquete* paquete = crear_paquete();

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&nuevoSegmento->id, paquete->buffer->stream, sizeof(int));
	memcpy(&nuevoSegmento->tamanio_segmento, paquete->buffer->stream + sizeof(int), sizeof(int));
	nuevoSegmento->direccion_base = 0;

	eliminar_paquete(paquete);

	return nuevoSegmento;
}

t_paquete* recibir_paquete(int socket){
	t_paquete* paquete = crear_paquete();
	recv(socket, &(paquete->codigo_operacion), sizeof(op_code), 0);

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);
	return paquete;
}


void solicitar_nuevo_proceso(int socket, int id){

	if(id != informar_id(socket, id)); //TODO: log de error

	enviar_operacion(socket, INICIALIZAR_PROCESO);

}

int informar_id(int socket, int id){

	send(socket, &id, sizeof(int), 0);
	if(recv(socket, &id, sizeof(int), MSG_WAITALL) == -1) return -1;

	return id;
}

int recibir_id(int socket){
	int id = -1;

	recv(socket, &id, sizeof(int), MSG_WAITALL);
	send(socket, &id, sizeof(int), 0);

	return id;
}

void liberar_registros(Registros* registros) {

/*
	free(registros->AX);
	free(registros->BX);
	free(registros->CX);
	free(registros->DX);
	free(registros->EAX);
	free(registros->EBX);
	free(registros->ECX);
	free(registros->EDX);
	free(registros->RAX);
	free(registros->RBX);
	free(registros->RCX);
	free(registros->RDX);
*/
	free(registros);
}


void liberar_proceso(Proceso* proceso) {

	list_destroy_and_destroy_elements(proceso->tablaDeSegmentos, free);
	free(proceso);

}

