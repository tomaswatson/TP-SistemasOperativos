#ifndef MMU_H_
#define MMU_H_

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "utils.h"


uint32_t calcularDireccionFisica(char*, contexto_ejecucion*);
bool operacion_es_valida(uint32_t, int, contexto_ejecucion*);

//Operaciones adicionales
uint32_t obtenerNumeroSegmento(char*, contexto_ejecucion*);
uint32_t obtenerOffset(char*, contexto_ejecucion*);
segmento* obtenerSegmentoPorIndice(uint32_t, t_list*);


extern segmento* segmento_proceso;

#endif /* MMU_H_ */
