#include "utils.h"

t_log* logger;
t_config* config;
Configuracion* configuracion;
int socket_cpu;
int socket_fileSystem;
int socket_kernel;
int server_fd;
void* memoria;
t_list* listaDeProcesos;
segmento* segmento0;
t_list* huecosVacios;
int (*puntero_algoritmo)(int);

int pidFs;
int pidCpu;
pthread_mutex_t operando_cpu_kernel;

void leerconfig(){
	configuracion = malloc(sizeof(Configuracion));

	configuracion->PUERTO_ESCUCHA = config_get_string_value(config,"PUERTO_ESCUCHA");
	configuracion->TAM_MEMORIA = config_get_int_value(config,"TAM_MEMORIA");
	configuracion->TAM_SEGMENTO_0 = config_get_int_value(config,"TAM_SEGMENTO_0");
	configuracion->CANT_SEGMENTOS = config_get_int_value(config,"CANT_SEGMENTOS");
	configuracion->RETARDO_MEMORIA = config_get_int_value(config,"RETARDO_MEMORIA");
	configuracion->RETARDO_COMPACTACION = config_get_int_value(config,"RETARDO_COMPACTACION");
	configuracion->ALGORITMO_ASIGNACION = config_get_string_value(config,"ALGORITMO_ASIGNACION");
}

void crearHuecoVacio(int comienzo, int tamanio){
	Hueco_Vacio* hueco = malloc(sizeof(Hueco_Vacio));
	hueco->comienzo = comienzo;
	hueco->fin = comienzo + tamanio;
	hueco->tamanio = tamanio;
	list_add(huecosVacios,hueco);
}

void crear_memoria(){
	memoria = malloc(configuracion->TAM_MEMORIA);
	listaDeProcesos = list_create();
	huecosVacios = list_create();
	crearHuecoVacio(0,configuracion->TAM_MEMORIA);

	pthread_mutex_init(&operando_cpu_kernel,NULL);
}

Proceso* crear_proceso(){
	Proceso* nuevoProceso = malloc(sizeof(Proceso));
	nuevoProceso->tablaDeSegmentos = list_create();
	return nuevoProceso;
}

segmento* crearSegmento(int id,int base,int limite){
	segmento* nuevoSegmento = malloc(sizeof(segmento));
	nuevoSegmento->id = id;
	nuevoSegmento->direccion_base = base;
	nuevoSegmento->tamanio_segmento = limite;
	return nuevoSegmento;

}

segmento* crearSegmento0(int id,int base,int limite){
	segmento* nuevoSegmento = crearSegmento(id,base,limite);
	puntero_algoritmo(limite);
	return nuevoSegmento;
}

Hueco_Vacio* comparadorMinimo(int tamanioNecesario,Hueco_Vacio* hueco1,Hueco_Vacio* hueco2){
	if((hueco1->tamanio < hueco2->tamanio) && hueco1->tamanio >= tamanioNecesario){
		return hueco1;
	}else{
		if(hueco2->tamanio >= tamanioNecesario){
			return hueco2;
		}
		else{
			return hueco1;
		}
	}
}

Hueco_Vacio* comparadorMaximo(int tamanioNecesario,Hueco_Vacio* hueco1,Hueco_Vacio* hueco2){
	if((hueco1->tamanio > hueco2->tamanio) && hueco1->tamanio >= tamanioNecesario){
		return hueco1;
	}else{
		if(hueco2->tamanio >= tamanioNecesario){
			return hueco2;
		}
		else{
			return hueco1;
		}
	}
}

int obtenerIndiceProceso(uint32_t id) {

	t_list_iterator* iterador = list_iterator_create(listaDeProcesos);
	int posicion = 0;
	Proceso* nuevoProceso;

	while(list_iterator_has_next(iterador)) {

		nuevoProceso = list_get(listaDeProcesos, posicion);

		if(nuevoProceso->idProceso == id) {

			list_iterator_destroy(iterador);
			return posicion;
		}

		posicion ++;
	}
	list_iterator_destroy(iterador);


	return -1;
}

int algoritmoBestFit(int tamanio){
	void* _comparadorMinimo (void* elemento1,void* elemento2){
		return comparadorMinimo(tamanio,elemento1,elemento2);
	}
	Hueco_Vacio* hueco = list_get_minimum(huecosVacios,_comparadorMinimo);
	int base = hueco->comienzo;
	hueco->comienzo += tamanio;
	hueco->tamanio = hueco->fin - hueco->comienzo;
	if(hueco->tamanio == 0){
		list_remove_element(huecosVacios,hueco);
	}
	return base;
}
int algoritmoWorstFit(int tamanio){
	void* _comparadorMaximo(void* elemento1,void* elemento2){
		return comparadorMaximo(tamanio,elemento1,elemento2);
	}
	Hueco_Vacio* hueco = list_get_maximum(huecosVacios,_comparadorMaximo);
	int base = hueco->comienzo;
	hueco->comienzo += tamanio;
	hueco->tamanio = hueco->fin - hueco->comienzo;
	if(hueco->tamanio == 0){
		list_remove_element(huecosVacios,hueco);
	}
	return base;
}
int algoritmoFirstFit(int tamanio){
	Hueco_Vacio* hueco = list_get(huecosVacios,0);
	int base = hueco->comienzo;
	hueco->comienzo += tamanio;
	hueco->tamanio = hueco->fin - hueco->comienzo;
	if(hueco->tamanio == 0){
		list_remove_element(huecosVacios,hueco);
	}
	return base;
}

void elegirAsignacion(){
	int utilizarBestFit = strcmp(configuracion->ALGORITMO_ASIGNACION,"BEST");
	int utilizarWorstFit = strcmp(configuracion->ALGORITMO_ASIGNACION,"WORST");
	int utilizarFirstFit = strcmp(configuracion->ALGORITMO_ASIGNACION,"FIRST");

	if(utilizarBestFit == 0){
		puntero_algoritmo = algoritmoBestFit;
	}

	if(utilizarWorstFit == 0){
		puntero_algoritmo = algoritmoWorstFit;
	}

	if(utilizarFirstFit == 0){
		puntero_algoritmo = algoritmoFirstFit;
	}
}

void combinarHuecos(){
	ordernarListaHuecosVacios();
	int tamanioLista = list_size(huecosVacios);
	for(int i = 0;i < tamanioLista;i++){
		Hueco_Vacio* huecoActual = list_get(huecosVacios,i);

		for(int j = i+1;j < tamanioLista;j++){
			Hueco_Vacio* huecoSiguiente = list_get(huecosVacios,j);

			if(huecoActual->fin == huecoSiguiente->comienzo){
				Hueco_Vacio* huecoCombinado = malloc(sizeof(Hueco_Vacio));
				huecoCombinado->comienzo = huecoActual->comienzo;
				huecoCombinado->fin = huecoSiguiente->fin;
				huecoCombinado->tamanio = huecoCombinado->fin - huecoCombinado->comienzo;

				list_remove_and_destroy_element(huecosVacios,i,free);
				list_remove_and_destroy_element(huecosVacios,j-1,free);
				list_add(huecosVacios,huecoCombinado);

				tamanioLista--;
				i--;
				break;
			}
		}
	}
}

void liberarListaHuecos(Proceso* proceso){
	t_list_iterator* iterador = list_iterator_create(proceso->tablaDeSegmentos);
	segmento* nuevoSegmento = list_iterator_next(iterador);

	while(list_iterator_has_next(iterador)){
		nuevoSegmento = list_iterator_next(iterador);

		Hueco_Vacio* nuevoHuecoVacio = malloc(sizeof(Hueco_Vacio));
		nuevoHuecoVacio->comienzo = nuevoSegmento->direccion_base;
		nuevoHuecoVacio->fin = nuevoSegmento->direccion_base + nuevoSegmento->tamanio_segmento;
		nuevoHuecoVacio->tamanio = nuevoSegmento->tamanio_segmento;

		list_add(huecosVacios,nuevoHuecoVacio);
	}
	list_iterator_destroy(iterador);

	combinarHuecos();
}

bool entraEnElHueco(int tamanio, Hueco_Vacio* hueco){
	return hueco->tamanio >= tamanio;
}

bool buscarHueco(int tamanio){
	bool _entraEnElHueco(void* hueco){
		return entraEnElHueco(tamanio,hueco);
	}
	return list_any_satisfy(huecosVacios, _entraEnElHueco);
}

int aniadirSegmento(Proceso* proceso,segmento* segmento){
	int base = puntero_algoritmo(segmento->tamanio_segmento);
	segmento->direccion_base = base;
	list_add(proceso->tablaDeSegmentos,segmento);
	return base;
}

bool esElSegmento(segmento* segmento,int direccionFisica){
	return (segmento->direccion_base <= direccionFisica) && (direccionFisica <= segmento->direccion_base + segmento->tamanio_segmento);
}

//buscar un mejor nombre para la funcion
segmento* procesoContieneDireccion(Proceso* proceso,int direccionFisica){
	bool _esElSegmento(void* elemento){
		return esElSegmento(elemento,direccionFisica);
	}

	return list_find(proceso->tablaDeSegmentos,_esElSegmento);
}

segmento* buscarSegmento(int direccionFisica){
	bool _procesoContieneDireccion(void* elemento){
		return procesoContieneDireccion(elemento,direccionFisica) != NULL;
	}


	Proceso* proceso = list_find(listaDeProcesos,_procesoContieneDireccion);
	if(proceso != NULL){
		return procesoContieneDireccion(proceso,direccionFisica);
	}
	else
	{
		return NULL;
	}
}

int sumarTamanioHuecos(int acumulador,Hueco_Vacio* hueco){
	return acumulador + hueco->tamanio;
}

int espacioVacioEnMemoria(){
	void* _sumarTamanioHuecos(void* elemento1,void* elemento2){
		return sumarTamanioHuecos(elemento1,elemento2);
	}
	return list_fold(huecosVacios,0,_sumarTamanioHuecos);
}

void loggersCompactacion(){
//	“PID: <PID> - Segmento: <ID SEGMENTO> - Base: <BASE> - Tamaño <TAMAÑO>”
	t_list_iterator* iterador = list_iterator_create(listaDeProcesos);
	Proceso* proceso;

	while(list_iterator_has_next(iterador)){
		proceso = list_iterator_next(iterador);

		t_list_iterator* iteradorSegmento = list_iterator_create(proceso->tablaDeSegmentos);
		segmento* segmento;

		while(list_iterator_has_next(iteradorSegmento)){
			segmento = list_iterator_next(iteradorSegmento);

			log_info(logger,"PID: %d - Segmento: %d - Base: %d - Tamaño %d", proceso->idProceso, segmento->id, segmento->direccion_base, segmento->tamanio_segmento);
		}
		list_iterator_destroy(iteradorSegmento);
	}
	list_iterator_destroy(iterador);
}

void loggersHuecos(){
//	“PID: <PID> - Segmento: <ID SEGMENTO> - Base: <BASE> - Tamaño <TAMAÑO>”
	t_list_iterator* iterador = list_iterator_create(huecosVacios);
	Hueco_Vacio* hueco;

	while(list_iterator_has_next(iterador)){
		hueco = list_iterator_next(iterador);

		log_info(logger,"Hueco, comienzo: %d, fin: %d, tamaño %d ", hueco->comienzo,hueco->fin,hueco->tamanio);


	}
	list_iterator_destroy(iterador);
}

bool ordenarLista(Hueco_Vacio* hueco1,Hueco_Vacio* hueco2){
	return hueco1->comienzo < hueco2->comienzo?1:0;
}

void ordernarListaHuecosVacios(){
	bool _ordenarLista(void* elemento1, void* elemento2){
		return ordenarLista(elemento1,elemento2);
	}
	list_sort(huecosVacios,_ordenarLista);
}

void conectar_con_kernel()
{


	while(1){

		int processID = recibir_id(socket_kernel);

		if(processID == -1) log_error(logger, "Error al recibir ID de proceso");

		int cod_op = recibir_operacion(socket_kernel);
		switch(cod_op){
		case INICIALIZAR_PROCESO:
			Proceso* nuevoProceso = ingresar_nuevo_proceso(processID);
			log_info(logger,"Creación de Proceso PID: %d", nuevoProceso->idProceso);
			enviar_segmentos(socket_kernel, nuevoProceso->tablaDeSegmentos);
			break;

		case FINALIZAR_PROCESO:
//			loggersHuecos();
			int posicion = obtenerIndiceProceso(processID);
			Proceso* procesoABorrar = list_remove(listaDeProcesos,posicion);
			liberarListaHuecos(procesoABorrar);
			log_info(logger,"Eliminación de Proceso PID: %d", processID);
			list_remove(procesoABorrar->tablaDeSegmentos,0);
			list_destroy_and_destroy_elements(procesoABorrar->tablaDeSegmentos, free);
			free(procesoABorrar);
//			loggersHuecos();
			break;

		case CREAR_SEGMENTO:
			segmento* segmentoNuevo = recibir_solicitud_segmento(socket_kernel);
			if(!buscarHueco(segmentoNuevo->tamanio_segmento)){
				if(espacioVacioEnMemoria() >= segmentoNuevo->tamanio_segmento){
					//ENVIAR MENSAJE DE COMPACTACION
					enviar_operacion(socket_kernel, MEMORY_COMPACTION);
					break;
				}
				//ENVIAR MENSAJE DE ERROR, NO HAY ESPACIO
				enviar_operacion(socket_kernel, OUT_OF_MEMORY);
				break;
			}
			posicion = obtenerIndiceProceso(processID);
			Proceso* procesoActualizar = list_get(listaDeProcesos,posicion);
			int base = aniadirSegmento(procesoActualizar,segmentoNuevo);
			//ENVIAR RESPUSTA A KERNEL
			enviar_operacion(socket_kernel, SUCCESS);
//			loggersCompactacion();
			enviar_segmentos(socket_kernel, procesoActualizar->tablaDeSegmentos);
			log_info(logger,"PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d",procesoActualizar->idProceso, segmentoNuevo->id, segmentoNuevo->direccion_base, segmentoNuevo->tamanio_segmento);

			break;

		case ELIMINAR_SEGMENTO:
			recibir_eliminacion_segmento(processID);
			procesoActualizar = list_get(listaDeProcesos,obtenerIndiceProceso(processID));
			enviar_segmentos(socket_kernel, procesoActualizar->tablaDeSegmentos);
			break;

		case COMPACTAR_SEGMENTOS:
			 pthread_mutex_lock(&operando_cpu_kernel);
			 log_info(logger,"Solicitud de Compactación");

			 sleep(configuracion->RETARDO_COMPACTACION/1000);

			 ordernarListaHuecosVacios();
			 t_list_iterator* iterador = list_iterator_create(huecosVacios);
			 Hueco_Vacio* hueco;

			 while(list_iterator_has_next(iterador)){
				 hueco = list_iterator_next(iterador);

				 segmento* segmentoAnterior = buscarSegmento(hueco->comienzo);
				 segmento* segmentoSiguiente = buscarSegmento(hueco->fin);

				 while(segmentoSiguiente != NULL){
					 memcpy(memoria+segmentoAnterior->direccion_base + segmentoAnterior->tamanio_segmento, memoria + segmentoSiguiente->direccion_base, segmentoSiguiente->tamanio_segmento);
					 segmentoSiguiente->direccion_base = segmentoAnterior->direccion_base + segmentoAnterior->tamanio_segmento;
					 hueco->comienzo = segmentoSiguiente->direccion_base + segmentoSiguiente->tamanio_segmento;
					 hueco->fin = hueco->comienzo + hueco->tamanio;

					 segmentoAnterior = segmentoSiguiente;
					 segmentoSiguiente = buscarSegmento(hueco->fin);
				 }


				 combinarHuecos();
				 if(list_size(huecosVacios) ==1){ //esta mal porque puede quedar 1 hueco en la mitad de la memoria
					 break;
				 }
			 }
			 list_iterator_destroy(iterador);

			 Hueco_Vacio* huecoParaAcomodar = list_get(huecosVacios,0);
			 if(huecoParaAcomodar->fin != configuracion->TAM_MEMORIA){
				 segmento* segmentoSiguiente = buscarSegmento(huecoParaAcomodar->fin);
				 segmentoSiguiente->direccion_base = huecoParaAcomodar->comienzo;
				 huecoParaAcomodar->comienzo = segmentoSiguiente->direccion_base + segmentoSiguiente->tamanio_segmento;
				 huecoParaAcomodar->fin = huecoParaAcomodar->comienzo + huecoParaAcomodar->tamanio;
			 }

			 loggersCompactacion();
			 enviar_lista_procesos(socket_kernel,listaDeProcesos);
			 pthread_mutex_unlock(&operando_cpu_kernel);
			 break;

		default:abort();
		break;
		}
	}
}


void conectar_con_cpu()
{

	while(1){
		log_info(logger, "Esperando operacion de CPU");
		pidCpu = recibir_id(socket_cpu);
		pthread_mutex_lock(&operando_cpu_kernel);
		int cod_op = recibir_operacion(socket_cpu);
		log_info(logger, "Recibi operacion %d de CPU", cod_op);
		sleep(configuracion->RETARDO_MEMORIA/1000);
		switch(cod_op){
		case ACCEDER_ESPACIO_LECTURA:
			recibir_lectura();
			break;
		case ACCEDER_ESPACIO_ESCRITURA:
			recibir_escritura();
			break;
		default: abort();
		break;
		}
		pthread_mutex_unlock(&operando_cpu_kernel);
	}
}

void conectar_con_fileSystem()
{

	while(1){
		log_info(logger, "Esperando operacion de FS");
		pidFs = recibir_id(socket_fileSystem);
		int cod_op = recibir_operacion(socket_fileSystem);
		sleep(configuracion->RETARDO_MEMORIA/1000);
		log_info(logger, "Recibi operacion %d de FS", cod_op);
		switch(cod_op){
		case ACCEDER_ESPACIO_LECTURA:
			leer_archivo();
			break;
		case ACCEDER_ESPACIO_ESCRITURA:
			escribir_archivo();
			break;
		default: abort();
			break;
		}
	}
}

char* leer(uint32_t direccion_fisica, int bytes_a_leer){
	char* informacion = malloc(bytes_a_leer);

	memcpy(informacion, memoria + direccion_fisica, bytes_a_leer);

	return informacion;
}

void escribir(uint32_t direccion_fisica, int bytes_a_escribir, char* informacion){

	memcpy(memoria + direccion_fisica, informacion, bytes_a_escribir);

}

Proceso* ingresar_nuevo_proceso(int id){
	Proceso* nuevoProceso = crear_proceso();
	nuevoProceso->idProceso = id;

	list_add(nuevoProceso->tablaDeSegmentos, segmento0);
	list_add(listaDeProcesos,nuevoProceso);
	return nuevoProceso;
}

void eliminar_segmento(int id_proceso, int id_segmento){
	t_list_iterator* iterador = list_iterator_create(listaDeProcesos);
	segmento* segmento_a_eliminar;
	while(list_iterator_has_next(iterador)){
		Proceso* proceso = list_iterator_next(iterador);
		if(proceso->idProceso == id_proceso){
		t_list_iterator* iterador_segmentos = list_iterator_create(proceso->tablaDeSegmentos);
			while(list_iterator_has_next(iterador_segmentos)){
				segmento* segmento = list_iterator_next(iterador_segmentos);
				if(segmento->id == id_segmento){
					segmento_a_eliminar = list_remove(proceso->tablaDeSegmentos, list_iterator_index(iterador_segmentos));
					log_info(logger,"PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d",id_proceso, segmento_a_eliminar->id, segmento_a_eliminar->direccion_base, segmento_a_eliminar->tamanio_segmento);
					break;
				}
			}
			list_iterator_destroy(iterador_segmentos);
		}
	}

	//Aca iria la logica de agregar el segmento eliminado a los huecos libres
	crearHuecoVacio(segmento_a_eliminar->direccion_base,segmento_a_eliminar->tamanio_segmento);
	combinarHuecos();

	free(segmento_a_eliminar);
	list_iterator_destroy(iterador);
}

void recibir_lectura(){
	int bytes_a_leer;
	uint32_t direccion_fisica;
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = ACCEDER_ESPACIO_LECTURA;
	recv(socket_cpu, &paquete->buffer->size, sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	memcpy(&direccion_fisica, paquete->buffer->stream, sizeof(uint32_t));
	memcpy(&bytes_a_leer, paquete->buffer->stream + sizeof(uint32_t), sizeof(int));

	char* informacion = leer(direccion_fisica,bytes_a_leer);
	log_info(logger,"PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU",pidCpu, direccion_fisica,bytes_a_leer);
	enviar_mensaje_con_tamanio(informacion,socket_cpu,bytes_a_leer);
	eliminar_paquete(paquete);
	free(informacion);
}

void recibir_escritura(){
	uint32_t direccion_fisica;
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = ACCEDER_ESPACIO_ESCRITURA;
	recv(socket_cpu, &paquete->buffer->size, sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_cpu, paquete->buffer->stream, paquete->buffer->size, 0);

	char* informacion = malloc(paquete->buffer->size - sizeof(uint32_t));

	memcpy(&direccion_fisica, paquete->buffer->stream, sizeof(uint32_t));
	memcpy(informacion, paquete->buffer->stream + sizeof(uint32_t), paquete->buffer->size - sizeof(uint32_t));
	escribir(direccion_fisica, paquete->buffer->size - sizeof(uint32_t), informacion);
	log_info(logger,"PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %lu - Origen: CPU", pidCpu, direccion_fisica, (paquete->buffer->size - sizeof(uint32_t)));
	eliminar_paquete(paquete);
	free(informacion);
}

void leer_archivo(){

	t_paquete* paquete = crear_paquete();
	uint32_t direccion_fisica, bytes_leidos;
	int desplazamiento = 0;

	paquete->codigo_operacion = ACCEDER_ESPACIO_LECTURA;

	recv(socket_fileSystem, &paquete->buffer->size, sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_fileSystem, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&direccion_fisica, paquete->buffer->stream, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&bytes_leidos, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	char* informacion = malloc(bytes_leidos);
	memcpy(informacion, paquete->buffer->stream + desplazamiento, bytes_leidos);

	escribir(direccion_fisica, bytes_leidos, informacion);
	log_info(logger,"PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS",pidFs, direccion_fisica,bytes_leidos);
	enviar_operacion(socket_fileSystem, SUCCESS);

	eliminar_paquete(paquete);

}

void escribir_archivo(){

	t_paquete* paquete = crear_paquete();
	uint32_t direccion_fisica, bytes_escritos;
	int desplazamiento = 0;

	paquete->codigo_operacion = ACCEDER_ESPACIO_ESCRITURA;

	recv(socket_fileSystem, &paquete->buffer->size, sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_fileSystem, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&direccion_fisica, paquete->buffer->stream, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&bytes_escritos, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));

	char* informacion = leer(direccion_fisica, bytes_escritos);

	enviar_mensaje(informacion, socket_fileSystem);
	log_info(logger,"PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS",pidFs, direccion_fisica,bytes_escritos);
	free(informacion);

	eliminar_paquete(paquete);

}

void recibir_eliminacion_segmento(int id_proceso){
	int id_segmento;

	recv(socket_kernel, &id_segmento, sizeof(int), MSG_WAITALL);

	eliminar_segmento(id_proceso, id_segmento);
}

void terminar_programa(){
	log_info(logger, "Finalizando memoria");
	close(server_fd);
	close(socket_cpu);
	close(socket_fileSystem);
	close(socket_kernel);
	config_destroy(config);
	log_destroy(logger);
}
