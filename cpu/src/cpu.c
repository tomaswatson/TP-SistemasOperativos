#include "cpu.h"

bool enEjecucion;
int processID;

int main(int argc, char **argv) {

	//argv[1] = "./cfg/seg256.config";
	config = iniciar_config(argv[1]);
	logger = iniciar_logger("./logs.txt");
	leerConfig();

	inicializarRegistros();

	log_info(logger, "Inicio del CPU.");
	socket_servidor = socket_crear_servidor(servidor_cpu.puerto, logger);



//1- Conexion con Memoria - Solamente lo conecta, la conexion no se mantiene en el tiempo.

	conectarse_a_memoria();

//2- Esperar Kernel - Solamente lo espera, no mantiene la conexion y no permite el envio de mensajes.
	esperar_kernel();
	recibirKernel();

	free(registros);
	return EXIT_SUCCESS;
}

void recibirKernel(){
	//recibe el paquete
	while(1){
		processID = recibir_id(socket_kernel);
		contexto_ejecucion* contexto = recibir_contexto(socket_kernel, logger);
		enEjecucion = true;
		cambiarContexto(contexto);

		while(enEjecucion){
			ejecutarProceso(contexto);
		}
		guardarContexto(contexto);
		t_paquete* paquete = paquete_contexto(contexto);
		enviar_paquete(paquete, socket_kernel);
		liberar_contexto(contexto);
		eliminar_paquete(paquete);
	}

}

void ejecutarProceso(contexto_ejecucion* contexto){
	Instruccion* miInstruccion = (Instruccion*) list_get(contexto->instrucciones, contexto->program_counter);
	int operacionInstruccion = miInstruccion->instruccion;
	contexto->solicitud->instruccion  = operacionInstruccion;
	contexto->program_counter++;
	char* param1;
	char* param2;
	char* param3;
	//obtenerParametros(miInstruccion->parametros, param1, param2, param3);
	char* parametros = malloc(strlen(miInstruccion->parametros));
	strcpy(parametros, miInstruccion->parametros);
	param1 = strtok(parametros, " \0");
	param2 = strtok(NULL, " \0");
	param3 = strtok(NULL, " \0");
		switch (operacionInstruccion) {
		        case F_READ:
		            f_F_READ(param1,param2,param3, contexto);
		            break;

		        case F_WRITE:
		            f_F_WRITE(param1, param2, param3, contexto);
		            break;

		        case SET:
		        	f_SET(param1,param2);
		        	sleep(configuracion->RETARDO_INSTRUCCION/1000);
		            break;

		        case MOV_IN:
		            f_MOV_IN(param1,param2, contexto);
		            break;

		        case MOV_OUT:
		            f_MOV_OUT(param1,param2, contexto);
		            break;

		        case F_TRUNCATE:
		            f_F_TRUNCATE(param1,param2, contexto);
		            break;

		        case F_SEEK:
		            f_F_SEEK(param1,param2, contexto);
		            break;

		        case CREATE_SEGMENT:
		            f_CREATE_SEGMENT(contexto, param1,param2);
		            break;

		        case IO:
		            f_IO(contexto, param1);
		            break;

		        case WAIT:
		            f_WAIT(contexto, param1);
		            break;

		        case SIGNAL:
		            f_SIGNAL(contexto, param1);
		            break;

		        case F_OPEN:
		            f_F_OPEN(contexto, param1);
		            break;

		        case F_CLOSE:
		            f_F_CLOSE(param1, contexto);
		            break;

		        case DELETE_SEGMENT:
		            f_DELETE_SEGMENT(param1,contexto);
		            break;

		        case EXIT:
		        	f_EXIT(contexto);
		            break;

		        case YIELD:
		            f_YIELD(contexto);
		            break;

		        default:
		            log_info(logger,"INSTRUCCION NO VALIDA");
		            abort();
		            break;
		    }
free(parametros);
}

void f_F_READ(char* nombreArchivo, char* direccionLogica, char* cantidadBytes, contexto_ejecucion* contexto){

	log_info(logger,"PID: %d - Ejecutando: F_READ %s %s %s",processID, nombreArchivo, direccionLogica, cantidadBytes);

	uint32_t direccionFisica = calcularDireccionFisica(direccionLogica, contexto);
	int bytes = atoi(cantidadBytes);
	int tamanio_parametro = strlen(nombreArchivo) + cantidadDigitos(direccionFisica) + strlen(cantidadBytes) + 3;

	char* parametros_solicitud = malloc(tamanio_parametro);

	if(operacion_es_valida(direccionFisica, bytes, contexto)){

		sprintf(parametros_solicitud,"%s %d %s", nombreArchivo, direccionFisica, cantidadBytes);
		contexto->solicitud->parametro = parametros_solicitud;
		contexto->solicitud->size = tamanio_parametro;

	} else {
		contexto->estado_error = SEG_FAULT;
	}

	enEjecucion = false;
}

void f_F_WRITE(char* nombreArchivo, char* direccionLogica, char* cantidadBytes,contexto_ejecucion* contexto){

	log_info(logger,"PID: %d - Ejecutando: F_WRITE %s %s %s",processID, nombreArchivo, direccionLogica, cantidadBytes);

	uint32_t direccionFisica = calcularDireccionFisica(direccionLogica, contexto);
	int tamanio_parametro = strlen(nombreArchivo) + cantidadDigitos(direccionFisica) + strlen(cantidadBytes) + 3;

	char* parametros_solicitud = malloc(tamanio_parametro);

	if(operacion_es_valida(direccionFisica, atoi(cantidadBytes), contexto)){

		sprintf(parametros_solicitud,"%s %d %s", nombreArchivo, direccionFisica, cantidadBytes);
		contexto->solicitud->parametro = parametros_solicitud;
		contexto->solicitud->size = tamanio_parametro;

	} else {
		contexto->estado_error = SEG_FAULT;
	}

	enEjecucion = false;
}

void f_SET(char* registro, char* valor){
	log_info(logger,"PID: %d - Ejecutando: SET %s %s",processID, registro, valor);
	setear_registro(registro, valor);
}

void f_MOV_IN(char* registro, char* direccionLogica, contexto_ejecucion* contexto){

	log_info(logger,"PID: %d - Ejecutando: MOV_IN %s %s",processID, registro, direccionLogica);

	uint32_t direccionFisica = calcularDireccionFisica(direccionLogica, contexto);

	int num_registro = obtener_numRegistro(registro);
	int tamanio_registro = obtener_tamanio_registros(num_registro);
	int num_segmento = obtenerNumeroSegmento(direccionLogica,contexto);
	char* lecturaMemoria;

	if(operacion_es_valida(direccionFisica, tamanio_registro, contexto)){

		informar_id(socket_memoria, processID);
		lecturaMemoria = leer_memoria(direccionFisica, tamanio_registro);
		setear_registro(registro, lecturaMemoria);

		lecturaMemoria = (char*) realloc(lecturaMemoria, tamanio_registro + 1);
		lecturaMemoria[tamanio_registro] = '\0';
		log_info(logger,"PID: %d - Acción: LEER - Segmento: %d - Direccion Física: %d - Valor: %s", processID, num_segmento, direccionFisica, lecturaMemoria);


		//free(lecturaMemoria);

		} else {
			contexto->estado_error = SEG_FAULT;
			enEjecucion = false;
	}
}

void f_MOV_OUT(char* direccionLogica, char* registro, contexto_ejecucion* contexto){

	log_info(logger,"PID: %d - Ejecutando: MOV_OUT %s %s",processID, registro, direccionLogica);
	log_info(logger, "Contexto: size: %d", contexto->segmentos_size);

	uint32_t direccionFisica = calcularDireccionFisica(direccionLogica, contexto);


	int num_registro = obtener_numRegistro(registro);
	int tamanio_registro = obtener_tamanio_registros(num_registro);
	char* valorAEscribir = leer_registro(registro);

	int num_segmento = obtenerNumeroSegmento(direccionLogica, contexto);

	if(operacion_es_valida(direccionFisica, tamanio_registro, contexto)) {
		informar_id(socket_memoria, processID);
		escribir_memoria(direccionFisica, valorAEscribir);

		valorAEscribir = realloc(valorAEscribir, obtener_tamanio_registros(obtener_numRegistro(registro))+1);
		valorAEscribir[obtener_tamanio_registros(obtener_numRegistro(registro))] = '\0';
		log_info(logger,"PID: %d - Acción: ESCRIBIR - Segmento: %d - Direccion Física: %d - Valor: %s", processID, num_segmento, direccionFisica, valorAEscribir);

	} else {
		int offset = obtenerOffset(direccionLogica, contexto);
		log_info(logger, "PID: %d - Error SEG_FAULT - Segmento: %d - Offset: %d, Tamaño: %d",processID,num_segmento, offset, tamanio_registro);
		contexto->estado_error = SEG_FAULT;
		enEjecucion = false;
	}
}

void f_F_TRUNCATE(char* nombreArchivo, char* tamanio, contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: F_TRUNCATE %s %s",processID, nombreArchivo, tamanio);

	char* parametros_enviar = malloc(strlen(nombreArchivo) + strlen(tamanio) + 1);
	strcpy(parametros_enviar, nombreArchivo);
	strcat(parametros_enviar, " ");
	strcat(parametros_enviar, tamanio);
	contexto->solicitud->parametro = parametros_enviar;
	contexto->solicitud->size = strlen(parametros_enviar)+1;

	enEjecucion = false;
}

void f_F_SEEK(char* nombreArchivo, char* posicion, contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: F_SEEK %s %s",processID, nombreArchivo, posicion);

	char* parametros_enviar = malloc(strlen(nombreArchivo) + strlen(posicion) + 1);
	strcpy(parametros_enviar, nombreArchivo);
	strcat(parametros_enviar, " ");
	strcat(parametros_enviar, posicion);
	contexto->solicitud->parametro = parametros_enviar;
	contexto->solicitud->size = strlen(parametros_enviar)+1;

	enEjecucion = false;
}

void f_CREATE_SEGMENT(contexto_ejecucion* contexto, char* idSegmento, char* tamanio){
	log_info(logger,"PID: %d - Ejecutando: CREATE_SEGMENT %s %s",processID, idSegmento, tamanio);

	char* parametros_enviar = malloc(strlen(idSegmento) + strlen(tamanio) + 1);
	strcpy(parametros_enviar, idSegmento);
	strcat(parametros_enviar, " ");
	strcat(parametros_enviar, tamanio);
	contexto->solicitud->parametro = parametros_enviar;
	contexto->solicitud->size = strlen(parametros_enviar)+1;

	enEjecucion = false;
}

void f_IO(contexto_ejecucion* contexto, char* tiempo){
	log_info(logger,"PID: %d - Ejecutando: I/O - %s",processID, tiempo);
	contexto->tiempo_bloqueo = atoi(tiempo);
	enEjecucion = false;
}

void f_WAIT(contexto_ejecucion* contexto, char* recurso){
	char* recurso_a_enviar = malloc(strlen(recurso));
	strcpy(recurso_a_enviar, recurso);
	log_info(logger,"PID: %d - Ejecutando: WAIT %s",processID, recurso_a_enviar);
	contexto->solicitud->parametro = recurso_a_enviar;
	contexto->solicitud->size = strlen(recurso_a_enviar)+1;
	enEjecucion = false;
}

void f_SIGNAL(contexto_ejecucion* contexto, char* recurso){
	char* recurso_a_enviar = malloc(strlen(recurso));
	strcpy(recurso_a_enviar, recurso);
	log_info(logger,"PID: %d - Ejecutando: SIGNAL %s",processID, recurso_a_enviar);
	contexto->solicitud->parametro = recurso_a_enviar;
	contexto->solicitud->size = strlen(recurso_a_enviar)+1;
	enEjecucion = false;
}

void f_F_OPEN(contexto_ejecucion* contexto, char* nombreArchivo){
	log_info(logger,"PID: %d - Ejecutando: F_OPEN %s",processID, nombreArchivo);

	char* nombre_archivo = malloc(strlen(nombreArchivo));
	strcpy(nombre_archivo, nombreArchivo);
	contexto->solicitud->parametro = nombre_archivo;
	contexto->solicitud->size = strlen(nombre_archivo)+1;

	enEjecucion = false;
}

void f_F_CLOSE(char* nombreArchivo, contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: F_CLOSE %s",processID, nombreArchivo);

	char* nombre_archivo = malloc(strlen(nombreArchivo));
	strcpy(nombre_archivo, nombreArchivo);
	contexto->solicitud->parametro = nombre_archivo;
	contexto->solicitud->size = strlen(nombre_archivo)+1;

	enEjecucion = false;
}

void f_DELETE_SEGMENT(char* idSegmento, contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: DELETE_SEGMENT %s",processID, idSegmento);
	char* segmento_a_borrar = malloc(strlen(idSegmento));
	strcpy(segmento_a_borrar, idSegmento);
	contexto->solicitud->parametro = segmento_a_borrar;
	contexto->solicitud->size = strlen(segmento_a_borrar)+1;
	enEjecucion = false;
}

void f_EXIT(contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: EXIT",processID);
	enEjecucion = false;
}

void f_YIELD(contexto_ejecucion* contexto){
	log_info(logger,"PID: %d - Ejecutando: YIELD", processID);
	enEjecucion = false;
}

void inicializarRegistros(){
	registros = malloc(sizeof(Registros));

	inicializar_registros(registros);
}

void cambiarContexto(contexto_ejecucion* contexto){

	memcpy(registros->AX, contexto->registros->AX, 4 * sizeof(char));
	memcpy(registros->BX, contexto->registros->BX, 4 * sizeof(char));
	memcpy(registros->CX, contexto->registros->CX, 4 * sizeof(char));
	memcpy(registros->DX, contexto->registros->DX, 4 * sizeof(char));
	memcpy(registros->EAX, contexto->registros->EAX, 8 * sizeof(char));
	memcpy(registros->EBX, contexto->registros->EBX, 8 * sizeof(char));
	memcpy(registros->ECX, contexto->registros->ECX, 8 * sizeof(char));
	memcpy(registros->EDX, contexto->registros->EDX, 8 * sizeof(char));
	memcpy(registros->RAX, contexto->registros->RAX, 16 * sizeof(char));
	memcpy(registros->RBX, contexto->registros->RBX, 16 * sizeof(char));
	memcpy(registros->RCX, contexto->registros->RCX, 16 * sizeof(char));
	memcpy(registros->RDX, contexto->registros->RDX, 16 * sizeof(char));

}

void guardarContexto(contexto_ejecucion* contexto){

	memcpy(contexto->registros->AX, registros->AX, 4 * sizeof(char));
	memcpy(contexto->registros->BX, registros->BX, 4 * sizeof(char));
	memcpy(contexto->registros->CX, registros->CX, 4 * sizeof(char));
	memcpy(contexto->registros->DX, registros->DX, 4 * sizeof(char));
	memcpy(contexto->registros->EAX, registros->EAX, 8 * sizeof(char));
	memcpy(contexto->registros->EBX, registros->EBX, 8 * sizeof(char));
	memcpy(contexto->registros->ECX, registros->ECX, 8 * sizeof(char));
	memcpy(contexto->registros->EDX, registros->EDX, 8 * sizeof(char));
	memcpy(contexto->registros->RAX, registros->RAX, 16 * sizeof(char));
	memcpy(contexto->registros->RBX, registros->RBX, 16 * sizeof(char));
	memcpy(contexto->registros->RCX, registros->RCX, 16 * sizeof(char));
	memcpy(contexto->registros->RDX, registros->RDX, 16 * sizeof(char));

}


