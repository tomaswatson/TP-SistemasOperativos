#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sockets.h>
#include <commons/log.h>
#include <commons/config.h>
#include <math.h>
#include "mmu.h"
#include "utils.h"
#include "comunicacion.h"


bool enEjecucion;

void recibirKernel(void);
void inicializarRegistros(void);
void ejecutarProceso(contexto_ejecucion* pcb);
void cambiarContexto(contexto_ejecucion* pcb);
void guardarContexto(contexto_ejecucion* pcb);

/*
 *  F_READ,
	F_WRITE,
	SET,
	MOV_IN,
	MOV_OUT,
	F_TRUNCATE,
	F_SEEK,
	CREATE_SEGMENT,
	IO,
	WAIT,
	SIGNAL,
	F_OPEN,
	F_CLOSE,
	DELETE_SEGMENT,
	EXIT,
	YIELD
*/

//Instrucciones
void f_F_READ(char* nombreArchivo, char* direccionLogica, char* cantidadBytes, contexto_ejecucion* contexto);
void f_F_WRITE(char* nombreArchivo, char* direccionLogica, char* cantidadBytes, contexto_ejecucion* contexto);
void f_SET(char* registro, char* valor);
void f_MOV_IN(char* registro, char* direccionLogica, contexto_ejecucion* contexto);
void f_MOV_OUT(char* direccionLogica, char* registro, contexto_ejecucion* contexto);
void f_F_TRUNCATE(char* nombreArchivo, char* tamanio, contexto_ejecucion* contexto);
void f_F_SEEK(char* nombreArchivo, char* posicion, contexto_ejecucion* contexto);
void f_CREATE_SEGMENT(contexto_ejecucion* contexto, char* idSegmento, char* tamanio);
void f_IO(contexto_ejecucion* pcb, char* tiempo);
void f_WAIT(contexto_ejecucion* contexto, char* recurso);
void f_SIGNAL(contexto_ejecucion* contexto, char* recurso);
void f_F_OPEN(contexto_ejecucion* contexto, char* nombreArchivo);
void f_F_CLOSE(char* nombreArchivo, contexto_ejecucion* contexto);
void f_DELETE_SEGMENT(char* idSegmento,contexto_ejecucion*);
void f_EXIT(contexto_ejecucion* pcb);
void f_YIELD(contexto_ejecucion* pcb);

#endif /* CPU_H_ */
