#include "utils.h"

t_log* logger;
t_config* config;
EstructuraConfiguracion* configuracion;
conexion conexion_memoria;
conexion servidor_cpu;
Registros* registros;
int socket_servidor;
int socket_kernel;
int socket_memoria;


// Funciones de conexiones

void conectarse_a_memoria() {
	log_info(logger, "Intentando conectar con Memoria");
	socket_memoria = socket_conectarse_servidor(conexion_memoria.ip, conexion_memoria.puerto, logger);
	if(socket_memoria == -1) log_error(logger, "Error al conectar con Memoria");
	//close(socket_memoria);

}


void esperar_kernel() {
	log_info(logger, "Listo para recibir al Kernel");
	socket_kernel = accept(socket_servidor, NULL, NULL);
	handshake_servidor(socket_kernel, logger, CPU);
	if(socket_kernel != -1) log_info(logger, "Inicializando conexion con Kernel");
	else log_error(logger, "Error al conectar con Kernel");
}

// Funciones relacionadas a instrucciones

void setear_registro(char *registro, char *valor) {

	int numero_registro = obtener_numRegistro(registro);

	switch(numero_registro) {

	case 1:
	strcpy(registros->AX, valor);
	break;
	 case 2:
	 strcpy(registros->BX, valor);
	 break;
	  case 3:
	  strcpy(registros->CX, valor);
	  break;
	   case 4:
	   strcpy(registros->DX, valor);
	   break;
	    case 5:
		strcpy(registros->EAX, valor);
		break;
	     case 6:
		 strcpy(registros->EBX, valor);
		 break;
	      case 7:
	      strcpy(registros->ECX, valor);
		  break;
	       case 8:
	       strcpy(registros->EDX, valor);
	       break;
	        case 9:
	       	strcpy(registros->RAX, valor);
	     	break;
	         case 10:
	         strcpy(registros->RBX, valor);
	         break;
	          case 11:
	          strcpy(registros->RCX, valor);
	          break;
	           case 12:
	           strcpy(registros->RDX, valor);
	           break;
	           	  default:
	           		  printf("No se ha encontrado el registro buscado\n");

	}
}

char* leer_registro(char* registro) {

	char* lectura = malloc(obtener_tamanio_registros(obtener_numRegistro(registro)));

	int numero_registro = obtener_numRegistro(registro);

		switch(numero_registro) {

		case 1:
		strcpy(lectura, registros->AX);
		break;
		 case 2:
			 strcpy(lectura, registros->BX);
		 break;
		  case 3:
			  strcpy(lectura, registros->CX);
		  break;
		   case 4:
			   strcpy(lectura, registros->DX);
		   break;
		    case 5:
		    	strcpy(lectura, registros->EAX);
			break;
		     case 6:
		    	 strcpy(lectura, registros->EBX);
			 break;
		      case 7:
		    	  strcpy(lectura, registros->ECX);
			  break;
		       case 8:
		    	   strcpy(lectura, registros->EDX);
		       break;
		        case 9:
		        	strcpy(lectura, registros->RAX);
		     	break;
		         case 10:
		        	 strcpy(lectura, registros->RBX);
		         break;
		          case 11:
		        	  strcpy(lectura, registros->RCX);
		          break;
		           case 12:
		        	   strcpy(lectura, registros->RDX);
		           break;
		           	  default:
		           		  printf("No se ha encontrado el registro buscado\n");
		}

	return lectura;
}

int obtener_numRegistro(char *registro) {

int numeroRegistro=0;

    if (strcmp(registro, "AX") == 0) {
        numeroRegistro = 1;
    } else if (strcmp(registro, "BX") == 0) {
    	numeroRegistro = 2;
    } else if (strcmp(registro, "CX") == 0) {
    	numeroRegistro = 3;
    }  else if (strcmp(registro, "DX") == 0) {
    	numeroRegistro = 4;
    }  else if (strcmp(registro, "EAX") == 0) {
    	numeroRegistro = 5;
    }  else if (strcmp(registro, "EBX") == 0) {
    	numeroRegistro = 6;
    }  else if (strcmp(registro, "ECX") == 0) {
    	numeroRegistro = 7;
    }  else if (strcmp(registro, "EDX") == 0) {
    	numeroRegistro = 8;
    }  else if (strcmp(registro, "RAX") == 0) {
    	numeroRegistro = 9;
    }  else if (strcmp(registro, "RBX") == 0) {
    	numeroRegistro = 10;
    }  else if (strcmp(registro, "RCX") == 0) {
    	numeroRegistro = 11;
    }  else if (strcmp(registro, "RDX") == 0) {
    	numeroRegistro = 12;
    }  else {
        printf("Campo no valido\n");
    }

return numeroRegistro;
}

void obtenerParametros(const char* cadena, char** cad1, char** cad2, char** cad3) {
    const char* delimitador = "/0";
    char* copiaCadena = strdup(cadena);
    char* token = strtok(copiaCadena, delimitador);

    if (token != NULL) {
        *cad1 = strdup(token);
    }

    token = strtok(NULL, delimitador);

    if (token != NULL) {
        *cad2 = strdup(token);
    }

    token = strtok(NULL, delimitador);

    if (token != NULL) {
        *cad3 = strdup(token);
    }

    free(copiaCadena);
}

int obtener_tamanio_registros(int num_registro) {

	if(num_registro <= 4) {
		return 4;
	} if(num_registro <=8) {
		return 8;
	} if(num_registro <=12) {
		return 16;
	}

	return 0;
}


// Funciones de configuracion y del programa

void leerConfig(){
	configuracion =  malloc(sizeof(EstructuraConfiguracion));
	servidor_cpu.ip = malloc(15);

	configuracion->RETARDO_INSTRUCCION = config_get_int_value(config, "RETARDO_INSTRUCCION");
	configuracion->TAM_MAX_SEGMENTO = config_get_int_value(config, "TAM_MAX_SEGMENTO");

	conexion_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
	conexion_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	servidor_cpu.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	strcpy(servidor_cpu.ip, "127.0.0.1");

}

void terminar_programa(void){

	log_info(logger, "Finalizando programa");

	free(configuracion);

	config_destroy(config);

	log_destroy(logger);
}

int cantidadDigitos(int numero) {

int contador = 0;

	while(numero > 0) {

		numero = numero /10;
		contador++;
	}

	return contador;
}

char* leer_memoria(uint32_t direccion_fisica, int cant_bytes){

	t_paquete* paquete = crear_paquete();

	paquete->codigo_operacion = ACCEDER_ESPACIO_LECTURA;

	paquete->buffer->size = sizeof(uint32_t) + sizeof(int);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &direccion_fisica, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &cant_bytes, sizeof(int));

	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);

	if(recibir_operacion(socket_memoria) == -1){
		log_error(logger,"Error al recibir la informacion de memoria en Escribir Archivo");
	}

	char* lectura = recibir_mensaje(socket_memoria);

	return lectura;
}

void escribir_memoria(uint32_t direccion_fisica, char* mensaje){
	t_paquete* paquete = crear_paquete();

	paquete->codigo_operacion = ACCEDER_ESPACIO_ESCRITURA;

	paquete->buffer->size = sizeof(uint32_t) + strlen(mensaje);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &direccion_fisica, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), mensaje, strlen(mensaje));

	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);
}



