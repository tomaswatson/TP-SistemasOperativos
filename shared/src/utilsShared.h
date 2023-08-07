//Utils de la shared library
#ifndef UTILSSHARED_H_
#define UTILSSHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include "sockets.h"
#include "comunicacion.h"
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>

t_log* iniciar_logger(char* pathRelativo);
t_config* iniciar_config(char* pathRelativo);

Proceso* extraer_proceso_por_id(t_list*, int);
char* codigo_error(int);


#endif /* UTILSSHARED_H_ */
