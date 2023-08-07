#include "utils.h"

SuperBloque* superBloque;
t_config* config_superBloque;
t_bitarray* bitMap;
void* bitMap_mapeado;
t_log* logger;
t_config* config;
t_config* templateFcb;
Configuracion* configuracion;
t_dictionary* tabla_archivos_abiertos;
void* archivo_bloques;
int socket_memoria;
int socket_servidor;
int socket_kernel;

int processID;

// Funciones de configuracion y del programa

void leerConfig(){
	configuracion =  malloc(sizeof(Configuracion));

	configuracion->IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	configuracion->PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	configuracion->PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	configuracion->PATH_SUPERBLOQUE = config_get_string_value(config, "PATH_SUPERBLOQUE");
	configuracion->PATH_BITMAP = config_get_string_value(config, "PATH_BITMAP");
	configuracion->PATH_BLOQUES = config_get_string_value(config, "PATH_BLOQUES");
	configuracion->PATH_FCB = config_get_string_value(config, "PATH_FCB");

	configuracion->RETARDO_ACCESO_BLOQUE = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
}

void conectarse_memoria() {
	log_info(logger,"Intentando conectar con Memoria");
	socket_memoria = socket_conectarse_servidor(configuracion->IP_MEMORIA, configuracion->PUERTO_MEMORIA, logger);
	if(socket_memoria == -1) log_error(logger, "Error al establecer conexion con Memoria");
}

void atender_solicitudes_kernel() {

	socket_kernel = accept(socket_servidor, NULL, NULL);
	handshake_servidor(socket_kernel, logger, FILESYSTEM);

	while(1){
		log_info(logger, "Esperando solicitud del Kernel");

		processID = recibir_id(socket_kernel);

		int solicitud_kernel = recibir_operacion(socket_kernel);

		switch(solicitud_kernel) {

		case ABRIR_ARCHIVO:
			abrir_archivo();
			break;

		case CREAR_ARCHIVO:
			crear_archivo();
			break;

		case TRUNCAR_ARCHIVO:
			recibir_truncado();
			break;

		case LEER_ARCHIVO:
			recibir_lectura();
			break;

		case ESCRIBIR_ARCHIVO:
			recibir_escritura();
			break;

		default:
			log_info(logger, "Solicitud invalida");
			abort();
			break;
		}

	}
}

void levantar_archivo_bloques(){

	log_info(logger, "Levantando archivo de bloques....\n");

	int fd = open(configuracion->PATH_BLOQUES, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	int tamanio = superBloque->BLOCK_COUNT * superBloque->BLOCK_SIZE;

    struct stat st;
    if (stat(configuracion->PATH_BLOQUES, &st) == 0) {
        if (st.st_size == 0) {
    		ftruncate(fd, superBloque->BLOCK_COUNT * superBloque->BLOCK_SIZE);

    		archivo_bloques = mmap(NULL, tamanio, PROT_WRITE, MAP_SHARED, fd, 0);

    		char* caracterCero = "0";

    		for (uint32_t offset = 0; offset < tamanio; offset++) {
    			memcpy(archivo_bloques + offset, caracterCero, sizeof(char));
    		}

    		actualizar_archivo_bloques_disco();

        }else{
    		archivo_bloques = mmap(NULL, tamanio, PROT_WRITE, MAP_SHARED, fd, 0);
        }
    }
	close(fd);
}

uint32_t buscar_numero_bloque(FCB* fcb, uint32_t puntero){

	if(puntero <= superBloque->BLOCK_SIZE){
		return fcb->punteroDirecto;
	}else{
		char* bloque = malloc(sizeof(uint32_t));
		puntero -= 64;
		uint32_t cant_bloques = puntero / superBloque->BLOCK_SIZE;
		memcpy(bloque, archivo_bloques + fcb->punteroIndirecto * superBloque->BLOCK_SIZE + cant_bloques * 4, 4);

		uint32_t numero_bloque = atoi(bloque);

		free(bloque);
		return numero_bloque;
	}

}

uint32_t limpiar_numero_bloque(FCB* fcb, uint32_t puntero){

	if(puntero <= superBloque->BLOCK_SIZE){
		return fcb->punteroDirecto;
	}else{
		char* bloque = malloc(sizeof(char) * 5);
		strcpy(bloque,"0000");

		puntero -= 64;
		uint32_t cant_bloques = puntero / superBloque->BLOCK_SIZE;
		memcpy(archivo_bloques + fcb->punteroIndirecto * superBloque->BLOCK_SIZE + cant_bloques * 4, bloque, 4);

		uint32_t numero_bloque = atoi(bloque);
		free(bloque);
		return numero_bloque;
	}

}

void escribir_bloque(char* nombre_archivo, uint32_t puntero, void* informacion_escritura, uint32_t tamanio_escribir){

	FCB* fcb = dictionary_get(tabla_archivos_abiertos, nombre_archivo);

	int bloque_archivo = puntero / superBloque->BLOCK_SIZE; //Este sería el numero de bloque del fcb?

	uint32_t numero_bloque = buscar_numero_bloque(fcb, puntero);

	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",
			fcb->nombre_archivo, bloque_archivo, numero_bloque);

	sleep(configuracion->RETARDO_ACCESO_BLOQUE / 1000);

	uint32_t offset = puntero % superBloque->BLOCK_SIZE;

	memcpy(archivo_bloques + numero_bloque * superBloque->BLOCK_SIZE + offset, informacion_escritura, tamanio_escribir);

	actualizar_archivo_bloques_disco();
}

char* leer_bloque(FCB* fcb, uint32_t puntero, uint32_t tamanio_leer){


	int bloque_archivo = puntero / superBloque->BLOCK_SIZE; //Este sería el numero de bloque del fcb?

	char* contenido = malloc(tamanio_leer + 1);

	uint32_t numero_bloque = buscar_numero_bloque(fcb, puntero);

	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d",
			fcb->nombre_archivo, bloque_archivo, numero_bloque);

	sleep(configuracion->RETARDO_ACCESO_BLOQUE / 1000);

	uint32_t offset = puntero % superBloque->BLOCK_SIZE;

	memcpy(contenido, archivo_bloques + numero_bloque * superBloque->BLOCK_SIZE + offset, tamanio_leer);

	contenido[tamanio_leer] = '\0';

	//123456789\0 9

	return contenido;
}

void abrir_archivo() {

	char* nombreArchivo = recibir_mensaje(socket_kernel);

	log_info(logger, "Abrir archivo: %s",nombreArchivo);

	if(dictionary_has_key(tabla_archivos_abiertos, nombreArchivo)) {
		log_info(logger, "El archivo existe");
		enviar_operacion(socket_kernel, ARCHIVO_EXISTE);
	}
	else{
		log_info(logger, "No existe el archivo");
		enviar_operacion(socket_kernel, ARCHIVO_NO_EXISTE);
	}

	free(nombreArchivo);
}

void crear_archivo() {

	char* nombreArchivo = recibir_mensaje(socket_kernel);

	log_info(logger, "Crear archivo: %s\n",nombreArchivo);

	crear_fcb(nombreArchivo, 0, -1, -1);

	enviar_operacion(socket_kernel, ARCHIVO_EXISTE);
}

void truncar_archivo(char* nombre_archivo, int tamanio) {

	log_info(logger, "Truncar archivo: %s - Tamano: %d",nombre_archivo, tamanio);

	FCB* fcb = dictionary_get(tabla_archivos_abiertos, nombre_archivo);

	if(fcb->tamanio_archivo < tamanio){

		log_info(logger, "Agrandando el archivo\n");

		uint32_t cant_bloques_asignar = divisionRedondeada(tamanio, superBloque->BLOCK_SIZE);

		if(fcb->punteroDirecto == -1){
			cant_bloques_asignar--;
			fcb->punteroDirecto = ocupar_bloque_vacio();
		}

		if(cant_bloques_asignar > 0){
			fcb->punteroIndirecto = ocupar_bloque_vacio();
		}

		int ciclos = 0;
		while(cant_bloques_asignar > 0){

			uint32_t bloque = ocupar_bloque_vacio();

		    char* nuevo_bloque = malloc(sizeof(char)*4);

		    snprintf(nuevo_bloque, 5, "%04d", bloque); // 5 = sizeof(nuevo_bloque)

			sleep(configuracion->RETARDO_ACCESO_BLOQUE / 1000);

			log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System %d",
					fcb->nombre_archivo, fcb->punteroIndirecto, fcb->punteroIndirecto);

		    memcpy(archivo_bloques + fcb->punteroIndirecto * superBloque->BLOCK_SIZE + ciclos * 4 , nuevo_bloque, 4);

			cant_bloques_asignar--;

			ciclos++;

			free(nuevo_bloque);
		}

	}else{

		log_info(logger, "Achicando el archivo\n");

		uint32_t cant_bloques_desasignar = divisionRedondeada(tamanio, superBloque->BLOCK_SIZE);

		if( tamanio/superBloque->BLOCK_SIZE > 1){
			cant_bloques_desasignar--;
		}

		while(cant_bloques_desasignar > 0){

			uint32_t ultimo_bloque = buscar_numero_bloque(fcb, tamanio);

			limpiar_numero_bloque(fcb, tamanio);

			bitarray_clean_bit(bitMap, ultimo_bloque); //Limpio la estructura

			log_info(logger, "Acceso a bitmap - Bloque: %d - Estado: %d", ultimo_bloque, bitarray_test_bit(bitMap, ultimo_bloque));

			actualizar_bitmap_disco();

			cant_bloques_desasignar--;
		}
	}

	fcb->tamanio_archivo = tamanio;

	persistir_fcb(fcb);

	actualizar_bitmap_disco();

	actualizar_archivo_bloques_disco();

	enviar_operacion(socket_kernel, SUCCESS);
}

void leer_archivo(char* nombreArchivo, uint32_t punteroArchivo, uint32_t direccionFisica, uint32_t cantidadBytes) {
	log_info(logger, "Leer Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n",
			nombreArchivo, punteroArchivo, direccionFisica, cantidadBytes);

	informar_id(socket_memoria, processID);

	FCB* fcbArchivo = dictionary_get(tabla_archivos_abiertos, nombreArchivo);
	char* informacion = leer_bloque(fcbArchivo, punteroArchivo, cantidadBytes);
	t_paquete* paqueton = crear_paquete();
	int desplazamiento = 0;

	paqueton->codigo_operacion = ACCEDER_ESPACIO_LECTURA;
	paqueton->buffer->size = sizeof(uint32_t) + sizeof(int) + strlen(informacion);
	paqueton->buffer->stream = malloc(paqueton->buffer->size);

	memcpy(paqueton->buffer->stream, &direccionFisica, sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(paqueton->buffer->stream + desplazamiento, &cantidadBytes, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(paqueton->buffer->stream + desplazamiento, informacion, cantidadBytes);

	log_info(logger,"Informacion leida del bloque que se enviará a memoria: %s", informacion);
	free(informacion);
	enviar_paquete(paqueton, socket_memoria);
	eliminar_paquete(paqueton);

	if(recibir_operacion(socket_memoria) != SUCCESS){
		log_error(logger, "No recibi SUCCESS de FS");
	}

	enviar_operacion(socket_kernel, SUCCESS);
}

void escribir_archivo(char* nombreArchivo, uint32_t punteroArchivo, uint32_t direccionFisica, uint32_t cantidadBytes) {

	log_info(logger, "Escribir archivo: %s - Puntero: %d - Memoria: %d, Tamaño: %d\n",
			nombreArchivo, punteroArchivo, direccionFisica, cantidadBytes);

	informar_id(socket_memoria, processID);

	t_paquete* paqueton = crear_paquete();
	int desplazamiento = 0;

	paqueton->codigo_operacion = ACCEDER_ESPACIO_ESCRITURA;
	paqueton->buffer->size = sizeof(uint32_t) + sizeof(int);
	paqueton->buffer->stream = malloc(paqueton->buffer->size);

	memcpy(paqueton->buffer->stream, &direccionFisica, sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(paqueton->buffer->stream + desplazamiento, &cantidadBytes, sizeof(int));
	desplazamiento+= sizeof(int);

	enviar_paquete(paqueton, socket_memoria);

	if(recibir_operacion(socket_memoria) == -1){
		log_error(logger,"Error al recibir la informacion de memoria en Escribir Archivo");
	}

	char* lecturaMemoria = recibir_mensaje(socket_memoria);

	lecturaMemoria = (char*) realloc(lecturaMemoria, cantidadBytes + 1);

	lecturaMemoria[cantidadBytes] = '\0';

	escribir_bloque(nombreArchivo, punteroArchivo, lecturaMemoria, cantidadBytes);

	log_info(logger,"Información recibida de memoria para escribir: %s",lecturaMemoria);

	free(lecturaMemoria);

	eliminar_paquete(paqueton);

	enviar_operacion(socket_kernel, SUCCESS);
}

void levantar_superBloque() {
	log_info(logger, "Levantando superBloque");

	config_superBloque = iniciar_config(configuracion->PATH_SUPERBLOQUE);

	superBloque = malloc(sizeof(SuperBloque));

	int BLOCK_COUNT = config_get_int_value(config_superBloque, "BLOCK_COUNT");
	superBloque->BLOCK_COUNT = BLOCK_COUNT;
	int BLOCK_SIZE = config_get_int_value(config_superBloque, "BLOCK_SIZE");
	superBloque->BLOCK_SIZE = BLOCK_SIZE;

	config_destroy(config_superBloque);

}

void levantar_bitMap() {
    log_info(logger, "Levantando bitMap");

    int fd = open(configuracion->PATH_BITMAP, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); //Abro el archivo

    struct stat st;
    if (stat(configuracion->PATH_BITMAP, &st) == 0) {
        if (st.st_size == 0) {
            ftruncate(fd, superBloque->BLOCK_COUNT / 8);
        }
    }

    bitMap_mapeado = mmap(NULL, superBloque->BLOCK_COUNT, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	bitMap = bitarray_create_with_mode(bitMap_mapeado, superBloque->BLOCK_COUNT / 8, LSB_FIRST);

    close(fd);

    actualizar_bitmap_disco();
}

void crear_estructuras_administrativas() {

	
	char* rutaDirectorio = configuracion->PATH_FCB;

	DIR* dir = opendir(rutaDirectorio);

	if(dir != NULL){

		struct dirent* entrada;

		while((entrada = readdir(dir)) != NULL) {

			if(entrada->d_type == DT_REG) {

				procesar_archivo(rutaDirectorio, entrada->d_name);

			}

		}
		closedir(dir);
	}
	else {
		log_error(logger, "No se ha encontrado el directorio");
	}
}

void procesar_archivo(char* ruta, char* nombreArchivo) {

	// Calculo el tamaño de la ruta completa (ruta del directorio + nombre del archivo);
	int tamanioRuta = strlen(ruta) + strlen(nombreArchivo) + 1;
	char rutaCompleta[tamanioRuta];

	strcpy(rutaCompleta, ruta);
	strcat(rutaCompleta, "/");
	strcat(rutaCompleta, nombreArchivo);

	templateFcb = iniciar_config(rutaCompleta);

    FCB* fcb = malloc(sizeof(FCB));

    fcb->nombre_archivo = config_get_string_value(templateFcb, "NOMBRE_ARCHIVO");
    fcb->tamanio_archivo = atoi(config_get_string_value(templateFcb, "TAMANIO_ARCHIVO"));
    fcb->punteroDirecto = atoi(config_get_string_value(templateFcb, "PUNTERO_DIRECTO"));
    fcb->punteroIndirecto = atoi(config_get_string_value(templateFcb, "PUNTERO_INDIRECTO"));
    dictionary_put(tabla_archivos_abiertos, nombreArchivo, fcb);

    //config_destroy(templateFcb);
}

void imprimir_bloques_ocupados(){
	uint32_t index = 0;

	printf("Bloques ocupados: ");
	while(index < superBloque->BLOCK_COUNT){

		if(bitarray_test_bit(bitMap, index)){

			bitarray_set_bit(bitMap, index);
			printf("%d ",index);
		}
		index++;
	}
	printf("\n");

}

uint32_t ocupar_bloque_vacio(){
	uint32_t index = 0;

	while(index < superBloque->BLOCK_COUNT){

		if(bloque_libre(index)){

			bitarray_set_bit(bitMap, index);
			log_info(logger, "Acceso a bitmap - Bloque: %d - Estado: %d", index, bitarray_test_bit(bitMap, index));
			return index;
		}

		index++;

	}

	log_error(logger, "No hay espacio en el fs");
	abort();

}

int actualizar_bitmap_disco(){

    if (msync(bitMap->bitarray, superBloque->BLOCK_COUNT, MS_SYNC) == -1) {
        log_error(logger, "No se pudo sincronizar con el archivo");
        munmap(bitMap_mapeado, superBloque->BLOCK_COUNT);
        return 1;
    }
    log_info(logger,"Se actualizó el bitmap exitosamente");
    return 0;
}

int actualizar_archivo_bloques_disco(){

    if (msync(archivo_bloques, superBloque->BLOCK_COUNT * superBloque->BLOCK_SIZE, MS_SYNC) == -1) {
    	log_error(logger, "No se pudo sincronizar con el archivo");
        munmap(archivo_bloques, superBloque->BLOCK_COUNT * superBloque->BLOCK_SIZE);
        return 1;
    }
    log_info(logger,"Se actualizó el archivo de bloques exitosamente");
    return 0;
}

FCB* crear_fcb(char* nombreArchivo, int tamanioFCB, uint32_t punteroDirecto, uint32_t punteroIndirecto) {


	FCB* nuevoFCB = malloc(sizeof(FCB));
	nuevoFCB->nombre_archivo = nombreArchivo;
	nuevoFCB->tamanio_archivo = tamanioFCB;
	nuevoFCB->punteroDirecto = punteroDirecto;
	nuevoFCB->punteroIndirecto = punteroIndirecto;

	dictionary_put(tabla_archivos_abiertos, nombreArchivo, nuevoFCB);
	persistir_fcb(nuevoFCB);


	log_info(logger, "Levantando FCB....");

	return nuevoFCB;
}

void levantar_estructuras(){
	tabla_archivos_abiertos = dictionary_create();
}

void terminar_programa(void){

	log_info(logger, "Finalizando FileSystem");

	//desmapeo el bitmap de memoria
    if (munmap(bitMap, superBloque->BLOCK_COUNT) == -1) {
        perror("No se pudo desmapear el archivo");
    }
	dictionary_destroy(tabla_archivos_abiertos);

	free(configuracion);

	free(superBloque);

	config_destroy(config);
	log_destroy(logger);

	bitarray_destroy(bitMap);
}

void recibir_lectura(){

	uint32_t direccionFisica, cantidadBytes, punteroArchivo;

	t_paquete* paquete = crear_paquete();
	int tamanioNombre, desplazamiento = 0;

	paquete->codigo_operacion = LEER_ARCHIVO;

	recv(socket_kernel, &paquete->buffer->size, sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&tamanioNombre, paquete->buffer->stream, sizeof(int));
	desplazamiento += sizeof(int);
	char* nombre_archivo = malloc(tamanioNombre);
	memcpy(nombre_archivo, paquete->buffer->stream + desplazamiento, tamanioNombre);
	desplazamiento += tamanioNombre;
	memcpy(&cantidadBytes, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&direccionFisica, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&punteroArchivo, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));

	eliminar_paquete(paquete);

	leer_archivo(nombre_archivo, punteroArchivo, direccionFisica, cantidadBytes);
	free(nombre_archivo);
}

void recibir_escritura(){
	uint32_t direccionFisica, cantidadBytes, punteroArchivo;

	t_paquete* paquete = crear_paquete();
	int tamanioNombre, desplazamiento = 0;

	paquete->codigo_operacion = ESCRIBIR_ARCHIVO;

	recv(socket_kernel, &paquete->buffer->size, sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&tamanioNombre, paquete->buffer->stream, sizeof(int));
	desplazamiento += sizeof(int);
	char* nombre_archivo = malloc(tamanioNombre);
	memcpy(nombre_archivo, paquete->buffer->stream + desplazamiento, tamanioNombre);
	desplazamiento += tamanioNombre;
	memcpy(&cantidadBytes, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&direccionFisica, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&punteroArchivo, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));

	eliminar_paquete(paquete);

	escribir_archivo(nombre_archivo, punteroArchivo, direccionFisica, cantidadBytes);
	free(nombre_archivo);
}

void recibir_truncado(){
	uint32_t nuevoTamanio;

	t_paquete* paquete = crear_paquete();
	int tamanioNombre, desplazamiento = 0;

	paquete->codigo_operacion =TRUNCAR_ARCHIVO;

	recv(socket_kernel, &paquete->buffer->size, sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket_kernel, paquete->buffer->stream, paquete->buffer->size, 0);

	memcpy(&tamanioNombre, paquete->buffer->stream, sizeof(int));
	desplazamiento += sizeof(int);
	char* nombre_archivo = malloc(tamanioNombre);
	memcpy(nombre_archivo, paquete->buffer->stream + desplazamiento, tamanioNombre);
	desplazamiento += tamanioNombre;
	memcpy(&nuevoTamanio, paquete->buffer->stream + desplazamiento, sizeof(uint32_t));

	eliminar_paquete(paquete);

	truncar_archivo(nombre_archivo,nuevoTamanio);
	free(nombre_archivo);
}

void persistir_fcb(FCB* fcb) {

	t_config* config_auxiliar;
	char* rutaCompleta = malloc(60);
	strcpy(rutaCompleta, configuracion->PATH_FCB);
	strcat(rutaCompleta, "/");
	strcat(rutaCompleta, fcb->nombre_archivo);

	FILE* f = fopen(rutaCompleta, "a+");

	fclose(f);

	char tamanioArchivo[20], punteroDirecto[20], punteroIndirecto[20];

    sprintf(tamanioArchivo, "%d", fcb->tamanio_archivo);
    sprintf(punteroDirecto, "%d", fcb->punteroDirecto);
    sprintf(punteroIndirecto, "%d", fcb->punteroIndirecto);

	config_auxiliar = iniciar_config(rutaCompleta);
	config_set_value(config_auxiliar,"NOMBRE_ARCHIVO", fcb->nombre_archivo);
	config_set_value(config_auxiliar,"TAMANIO_ARCHIVO", tamanioArchivo);
	config_set_value(config_auxiliar,"PUNTERO_DIRECTO", punteroDirecto);
	config_set_value(config_auxiliar,"PUNTERO_INDIRECTO", punteroIndirecto);
	config_save_in_file(config_auxiliar, rutaCompleta);


	free(rutaCompleta);
	config_destroy(config_auxiliar);

}

bool bloque_libre(int index){
	return (!bitarray_test_bit(bitMap, index));
}

uint32_t divisionRedondeada(uint32_t numerador, uint32_t denominador){
    double resultado = (double)numerador / denominador;
    uint32_t resultado_entero = (uint32_t)resultado;

    if (resultado - resultado_entero == 0) {
        return resultado_entero;
    } else {
        return (uint32_t)ceil(resultado);
    }
}

t_config* iniciar_fcb() {

	char* rutaCompleta = malloc(60);
	strcpy(rutaCompleta, configuracion->PATH_FCB);
	strcat(rutaCompleta, "/templateFcb.config");

	t_config* config_auxiliar = iniciar_config(rutaCompleta);
	free(rutaCompleta);


	return config_auxiliar;
}

