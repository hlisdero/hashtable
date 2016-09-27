#include "hash.h"
#include <stdlib.h>
#define TAM_INICIAL 64

/* Definiciones de estructuras de la tabla de hash */

struct hash {
    lista_t **datos;
    hash_function hashear;
    size_t cantidad;
    size_t tam;
    destruir_dato_t destruir_dato;
};

struct hash_iter {
    const hash_t *hash;
    lista_iter_t *lista_iter;
    size_t post;
};

/**************************************
 **  Primitivas de la Tabla de hash  **
 **************************************/

hash_t *hash_crear(hash_destruir_dato_t destruir_dato) {
    hash_t *nuevo = malloc(sizeof(*nuevo));
    lista_t **datos = calloc(TAM_INICIAL, sizeof(*datos));
    if (!nuevo)
        return NULL;
    if (!datos) {
        free(nuevo;
        return NULL;
    }
    /* Caso general */
    nuevo->datos = datos;
    nuevo->hashear = NULL // POR AHORA NO TENEMOS FUNCION
    nuevo->tam = TAM_INICIAL;
    nuevo->cantidad = 0;
    nuevo->destruir_dato = destruir_dato;
    return nuevo;
}

/* Guarda un elemento en el hash, si la clave ya se encuentra en la
 * estructura, la reemplaza. De no poder guardarlo devuelve false.
 * Pre: La estructura hash fue inicializada
 * Post: Se almacenó el par (clave, dato)
 */
bool hash_guardar(hash_t *hash, const char *clave, void *dato);

/* Borra un elemento del hash y devuelve el dato asociado.  Devuelve
 * NULL si el dato no estaba.
 * Pre: La estructura hash fue inicializada
 * Post: El elemento fue borrado de la estructura y se lo devolvió,
 * en el caso de que estuviera guardado.
 */
void *hash_borrar(hash_t *hash, const char *clave);

/* Obtiene el valor de un elemento del hash, si la clave no se encuentra
 * devuelve NULL.
 * Pre: La estructura hash fue inicializada
 */
void *hash_obtener(const hash_t *hash, const char *clave);

/* Determina si clave pertenece o no al hash.
 * Pre: La estructura hash fue inicializada
 */
bool hash_pertenece(const hash_t *hash, const char *clave);

/* Devuelve la cantidad de elementos del hash.
 * Pre: La estructura hash fue inicializada
 */
size_t hash_cantidad(const hash_t *hash) {
    return hash->cantidad;
}

/* Destruye la estructura liberando la memoria pedida y llamando a la función
 * destruir para cada par (clave, dato).
 * Pre: La estructura hash fue inicializada
 * Post: La estructura hash fue destruida
 */
void hash_destruir(hash_t *hash) {
    int i;
    if (!hash)
        return NULL;

    for (i = 0; i < hash->tam; i++) {
        lista_destruir(hash->datos[i], hash->destruir_dato); // lista_destruir no falla con NULL
    }
    free(hash->datos);
    free(hash);
}

/****************************************
 **  Primitivas del Iterador del hash  **
 ****************************************/

// Crea iterador
hash_iter_t *hash_iter_crear(const hash_t *hash) {
    if (!hash)
        return NULL;
    hash_iter_t *nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    /* Caso general */
    nuevo->hash = hash;
    nuevo->lista_iter = NULL;
    nuevo->pos = 0;
    return nuevo;
}

// Avanza iterador
bool hash_iter_avanzar(hash_iter_t *iter);

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
const char *hash_iter_ver_actual(const hash_iter_t *iter) {
    // INCOMPLETO, HAY QUE CODEAR UNA FUNCION QUE DE LA ESTRUCTURA QUE DEVUELVA LISTA_VER_ACTUAL SAQUE LA CLAVE
    return (hash_iter_al_final(iter) ? NULL: lista_ver_actual(iter->lista_iter));
}

// Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t *iter) {
    return (iter->pos == iter->hash->tam);
}

// Destruye iterador
void hash_iter_destruir(hash_iter_t* iter) {
    free(iter);
}

