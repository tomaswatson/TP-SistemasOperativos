#include "mmu.h"

segmento* segmento_proceso;


uint32_t calcularDireccionFisica(char* direccionLogica, contexto_ejecucion* contexto) {

	int num_segmento = obtenerNumeroSegmento(direccionLogica, contexto);
	int desplazamiento_segmento = obtenerOffset(direccionLogica, contexto);

	segmento* segmento = obtenerSegmentoPorIndice(num_segmento, contexto->tabla_segmentos);
	return segmento->direccion_base + desplazamiento_segmento;
}

bool operacion_es_valida(uint32_t direccionFisica, int cantidadBytes, contexto_ejecucion* contexto) {

	t_list_iterator* iterador = list_iterator_create(contexto->tabla_segmentos);
	segmento* segmento;

	while(list_iterator_has_next(iterador)){
		segmento = list_iterator_next(iterador);
		if(direccionFisica >= segmento->direccion_base &&
		  (direccionFisica + cantidadBytes) <= (segmento->direccion_base + segmento->tamanio_segmento)){
			return true;
		}
	}
	return false;
}

//Operaciones adicionales:
uint32_t obtenerNumeroSegmento(char* direccionLogica, contexto_ejecucion* contexto) {

	int direccionLogicaEntera = atoi(direccionLogica);

	int num_segmento = floor(direccionLogicaEntera / configuracion->TAM_MAX_SEGMENTO);
	return num_segmento;
}

uint32_t obtenerOffset(char* direccionLogica, contexto_ejecucion* contexto){
	int direccionLogicaEntera = atoi(direccionLogica);

	int desplazamiento_segmento = direccionLogicaEntera % configuracion->TAM_MAX_SEGMENTO;
	return desplazamiento_segmento;
}

segmento* obtenerSegmentoPorIndice(uint32_t id, t_list* listaSegmentos) {

	t_list_iterator* iterador = list_iterator_create(listaSegmentos);
	int posicion = 0;
	segmento* nuevoSegmento;

	while(list_iterator_has_next(iterador)) {

		nuevoSegmento = list_get(listaSegmentos, posicion);

		if(nuevoSegmento->id == id) {

			list_iterator_destroy(iterador);
			return nuevoSegmento;
		}

		posicion ++;
	}
	list_iterator_destroy(iterador);


	return NULL;
}
