#include "utilsShared.h"

t_log* iniciar_logger(char* pathRelativo)
{
	t_log* nuevo_logger;

	t_log_level log_level = LOG_LEVEL_INFO;

	nuevo_logger = log_create(pathRelativo,"log_informacion", true, log_level);

	if(nuevo_logger == NULL){
			abort();
		}else{
			return nuevo_logger;
		}
}

t_config* iniciar_config(char* pathRelativo)
{
	t_config* nuevo_config;

	nuevo_config = config_create(pathRelativo);

	if(nuevo_config == NULL){
		puts("Iniciar Config retornó null");
		abort();
	}else{
		return nuevo_config;
	}
}

Proceso* extraer_proceso_por_id(t_list* lista_procesos, int id){
	t_list_iterator* iterador = list_iterator_create(lista_procesos);
	while(list_iterator_has_next(iterador)){
		Proceso* proceso = list_iterator_next(iterador);
		if(proceso->idProceso == id) return proceso;
	}
	return NULL;
}

char* codigo_error(int codigo_e){
	char* codigo = malloc(30);
	switch (codigo_e){
		case SUCCESS: strcpy(codigo, "SUCCESS");
						break;

		case SEG_FAULT: strcpy(codigo, "SEGMENTATION FAULT");
						break;

		case OUT_OF_MEMORY: strcpy(codigo, "OUT OF MEMORY");
							break;

		case SIGNAL_INEXISTENTE: strcpy(codigo, "SIGNAL A RECURSO INEXISTENTE");
								 break;

		case WAIT_INEXISTENTE: strcpy(codigo, "WAIT A RECURSO INEXISTENTE");
							   break;

		default: strcpy(codigo, "CONNECTION LOST");
				 break;
	}
	return codigo;
}

