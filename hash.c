/* *****************************************************************
 *                    FACUNDO MAYER HALFON 95452
 * 
 *                    HORACIO LISDERO SCAFFINO 100132
 * 
 *                    AYUDANTE: JAVIER CHOQUE
 * *****************************************************************/
#define _POSIX_C_SOURCE 200809L
#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define TAM_INICIAL 67
#define FACTOR_CARGA_MAX 2
#define FACTOR_CARGA_MIN 0.3
#define FACTOR_ACHIQUE 3
#define FACTOR_AGRANDAMIENTO 3

/* *****************************************************************
 *          Definiciones de estructuras de la tabla de hash        *
 * *****************************************************************/

struct hash {
    lista_t **datos;
    size_t cantidad;
    size_t tam;
    hash_destruir_dato_t destruir_dato;
};

struct hash_iter {
    const hash_t *hash;
    lista_iter_t *lista_iter;
    size_t pos;
};

/* Estructura para guardar los datos */
typedef struct nodo {
    char *clave;
    void *dato;
} hash_nodo_t;

/* *****************************************************************
 *                    Funciones del nodo                           *
 * *****************************************************************/

static hash_nodo_t* nodo_crear(const char *clave, void *dato) {
    /* Todo nodo debe tener una clave, no se crean nodos vacios */
    if (!clave) return NULL;
    hash_nodo_t *nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    /* Reservar espacio para la clave */
    nuevo->clave=strdup(clave);
    nuevo->dato = dato;
    return nuevo;
}


static void nodo_destruir(hash_nodo_t *nodo, hash_destruir_dato_t destruir_dato) {
    if (!nodo) return;
    if (destruir_dato)
        destruir_dato(nodo->dato);
    free(nodo->clave);
    free(nodo);
}

/* *****************************************************************
 *                    Funciones auxiliares                         *
 * *****************************************************************/

/* Crea una estructura hash nueva con un tamaño dado */
static hash_t *hash_crear_tam_variable(hash_destruir_dato_t destruir_dato, size_t tam) {
    hash_t *nuevo = malloc(sizeof(*nuevo));
    lista_t **datos = calloc(tam, sizeof(*datos));
    if (!nuevo || !datos){
        free(nuevo);
        free(datos);
        return NULL;
    }
    /* Caso general */
    nuevo->datos = datos;
    nuevo->tam = tam;
    nuevo->cantidad = 0;
    nuevo->destruir_dato = destruir_dato;
    return nuevo;
}

/* Destruye el iterador y la lista del indice, si está vacía */
static void destruir_lista_con_iter(hash_t * hash, lista_iter_t * iter, size_t indice) {
    if (iter)
        lista_iter_destruir(iter);
    if (lista_esta_vacia(hash->datos[indice])) {
        lista_destruir(hash->datos[indice], NULL);
        hash->datos[indice] = NULL;
    }
}


/* Busca la clave en la lista, devuelve true si la encuentra, false en caso contrario.
 * Deja el iterador en la clave si la encontró o al final en caso contrario.
 */
static bool buscar_clave_lista(const char * clave, lista_iter_t * iter) {
    hash_nodo_t *nodo;
    /* Mientras se pueda avanzar, buscamos la clave */
    do {
		nodo=lista_iter_ver_actual(iter);
		if (!nodo) return false;
        if (strcmp(nodo->clave, clave) == 0)
            return true;
    } while (lista_iter_avanzar(iter));
    return false;
}

/* Devuelve el indice a una lista distinta de NULL en la tabla de hash */
static size_t buscar_siguiente_lista_hash(const hash_t * hash, size_t inicio) {
    size_t i;

    for (i = inicio; i < hash->tam; i++) {
        if (hash->datos[i])
            return i;
    }
    return hash->tam;
}

/* Actualiza el iterador de la lista, asumiendo que la posición ya fue actualizada */
static bool actualizar_lista_iter(hash_iter_t * iter) {
    if (hash_iter_al_final(iter)) {
        iter->lista_iter = NULL;
    } else {
        iter->lista_iter = lista_iter_crear(iter->hash->datos[iter->pos]);
        if (!iter->lista_iter) {
            return false;
        }
    }
    return true;
}



/* Destruye las listas de la tabla de hash */
static void hash_listas_destruir(hash_t * hash) {
    size_t i = 0;

    while ((i = buscar_siguiente_lista_hash(hash, i)) != hash->tam) {
        while (!lista_esta_vacia(hash->datos[i])) {
            nodo_destruir(lista_borrar_primero(hash->datos[i]), hash->destruir_dato);
        }
        lista_destruir(hash->datos[i], NULL);
        hash->datos[i]=NULL;
    }
}

/* Devuelve clave actual, ese dato no se puede modificar ni liberar 
static void *hash_iter_ver_dato(const hash_iter_t *iter) {
	if (hash_iter_al_final(iter))
		return NULL;
     La clave está dentro del nodo dentro del nodo de la lista apuntado por lista_iter 
       lista_iter va a apuntar a algo válido, por lo que ninguna función va a devolver NULL 
    nodo_t* nodo=lista_iter_ver_actual(iter->lista_iter);
    return nodo->dato;
}*/

/* Función de hash:
 * Implementación sencilla de la función de hash de K&R */
static size_t hash_conseguir_indice(const hash_t *hash, const char *clave) {
    size_t hashval;

    for (hashval = 0; *clave != '\0'; clave++)
        hashval = *clave + 31 * hashval;
	return hashval % hash->tam;
}

/* Funciones de redimensionamiento del hash */

static bool debe_agrandar(const hash_t * hash) {
    return (hash->cantidad / hash->tam > FACTOR_CARGA_MAX);
}

static bool debe_achicar(const hash_t * hash) {
    if (hash->tam < FACTOR_ACHIQUE*TAM_INICIAL)
        return false;
    else if (hash->cantidad / hash->tam < FACTOR_CARGA_MIN)
        return true;
    else
        return false;
}

static bool hash_redimensionar(hash_t * hash, size_t tam_nuevo) {
    hash_t * nuevo_hash = hash_crear_tam_variable(hash->destruir_dato, tam_nuevo);
    hash_iter_t * iter = hash_iter_crear(hash);
    if (!nuevo_hash || !iter) {
        free(nuevo_hash);
        free(iter);
        return false;
    }

    /* Se guardan los elementos del hash viejo en el nuevo */
    do {
		if (hash_iter_al_final(iter))
		    break;
		hash_nodo_t* nodo=lista_iter_ver_actual(iter->lista_iter);
        if (!hash_guardar(nuevo_hash, hash_iter_ver_actual(iter), nodo->dato))
            break;
    } while (hash_iter_avanzar(iter));

    hash_iter_destruir(iter);

    /* Si se copiaron todos los elementos, entonces terminó bien el while */
    if (hash->cantidad == nuevo_hash->cantidad) {
        /* Se destruyen las listas y la tabla, que son las cosas que se reemplazan */
        /* Los datos NO deben que destruirse, por eso se le saca la función destruir_dato temporalmente */
        hash->destruir_dato = NULL;
        hash_listas_destruir(hash);
        free(hash->datos);
        /* Se reemplaza por la nueva tabla, el nuevo tamaño y la función destruir_dato */
        hash->datos = nuevo_hash->datos;
        hash->tam = tam_nuevo;
        hash->destruir_dato = nuevo_hash->destruir_dato;
        free(nuevo_hash); // Solo se elimina nuevo_hash como cascarón de la tabla nueva que se creó
        return true;
    } else {
        /* Si falló se destruye todo el hash nuevo, excepto los datos */
        nuevo_hash->destruir_dato = NULL;
        hash_destruir(nuevo_hash);
        return false;
    }
}

/* *****************************************************************
 *                    Primitivas de la tabla hash                  *
 * *****************************************************************/

hash_t *hash_crear(hash_destruir_dato_t destruir_dato) {
    return hash_crear_tam_variable(destruir_dato, TAM_INICIAL);
}

bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
    if (!clave) return false; // Debe recibir una clave válida
    size_t indice = hash_conseguir_indice(hash, clave);
    hash_nodo_t *nuevo = nodo_crear(clave, dato);
    if (!nuevo) return false;

    /* Se crea la lista si no existe, si la lista o el iterador de esa lista no se pueden crear devuelve false */
    if (!hash->datos[indice]) {
        hash->datos[indice] = lista_crear();
        if (!hash->datos[indice]){
			nodo_destruir(nuevo, hash->destruir_dato);
            return false;
        }
    }
    lista_iter_t *iter = lista_iter_crear(hash->datos[indice]);
    if (!iter) {
        destruir_lista_con_iter(hash, NULL, indice);
        return false;
    }

    /* Busca la clave, si la encuentra tiene que borrar el elemento que va a ser reemplazado */
    if (buscar_clave_lista(clave,iter)) {
        nodo_destruir(lista_iter_borrar(iter), hash->destruir_dato); // El iter quedó en la posición del elemento repetido
        --(hash->cantidad);
    }
    /* Si no puede insertarlo tiene que destruir el nodo, el iter y la lista si está vacía */
    if (!lista_iter_insertar(iter, nuevo)) {
        nodo_destruir(nuevo, hash->destruir_dato);
        destruir_lista_con_iter(hash, iter, indice);
        return false;
    }
    lista_iter_destruir(iter);
    ++(hash->cantidad);
    if (debe_agrandar(hash))
        hash_redimensionar(hash, (hash->tam)*FACTOR_AGRANDAMIENTO);
    return true;
}

void *hash_borrar(hash_t *hash, const char *clave) {
	size_t indice = hash_conseguir_indice(hash,clave);
    lista_iter_t * iter;
    hash_nodo_t * nodo_salida;
    void * dato_salida;

    /* Si la lista es NULL o no logra crear un iterador para recorrerla devuelve NULL */
	if(!hash->datos[indice] || !(iter = lista_iter_crear(hash->datos[indice])) )
        return NULL;
    /* Caso general */
    if (buscar_clave_lista(clave, iter)) {
        nodo_salida = lista_iter_borrar(iter); // El iter quedó en la posición que se debe borrar
        dato_salida = nodo_salida->dato;
        nodo_destruir(nodo_salida, NULL);
        --(hash->cantidad);
        destruir_lista_con_iter(hash, iter, indice);
        if (debe_achicar(hash))
            hash_redimensionar(hash, (hash->tam)/FACTOR_ACHIQUE);
        return dato_salida;
    /* Caso no encontró la clave en la lista */
    } else {
        lista_iter_destruir(iter);
        return NULL;
    }
}

void *hash_obtener(const hash_t *hash, const char *clave) {
	size_t indice = hash_conseguir_indice(hash, clave);
    lista_iter_t *iter;
    void *dato_salida;
    hash_nodo_t* nodo;
    
    /* Si la lista es NULL o no logra crear un iterador para recorrerla devuelve NULL */
	if (!hash->datos[indice] || !(iter = lista_iter_crear(hash->datos[indice])))
	    return NULL;
    /* Caso general */
    if (buscar_clave_lista(clave, iter)) {
        /* El iter quedó en la posición del elemento que debemos devolver */
        nodo=lista_iter_ver_actual(iter);
        dato_salida = nodo->dato;
        lista_iter_destruir(iter);
        return dato_salida;
    } else {
        lista_iter_destruir(iter);
        return NULL;
    }
}

bool hash_pertenece(const hash_t *hash, const char *clave) {
	size_t indice = hash_conseguir_indice(hash,clave);
    lista_iter_t *iter;
    bool encontro_clave;
    /* Si la lista es NULL o no logra crear un iterador para recorrerla devuelve false */
	if (!hash->datos[indice] || !(iter = lista_iter_crear(hash->datos[indice])))
	    return false;
    encontro_clave = buscar_clave_lista(clave, iter);
    lista_iter_destruir(iter);
    return encontro_clave;
}

size_t hash_cantidad(const hash_t *hash) {
    return hash->cantidad;
}

void hash_destruir(hash_t *hash) {
    hash_listas_destruir(hash);
    free(hash->datos);
    free(hash);
}

/* *****************************************************************
 *                    Primitivas del iterador hash                 *
 * *****************************************************************/

hash_iter_t *hash_iter_crear(const hash_t *hash){
	hash_iter_t* iter = malloc(sizeof(*iter));
	if (!iter)
	    return NULL;
	iter->hash = hash;
    /* Hay que buscar una lista distinta de NULL y crear un iter para ella */
    iter->pos = buscar_siguiente_lista_hash(hash, 0);
    if (!actualizar_lista_iter(iter)) {
        free(iter);
        return NULL;
    }
	return iter;
}

bool hash_iter_avanzar(hash_iter_t *iter) {
	if (hash_iter_al_final(iter))
	    return false;
    lista_iter_avanzar(iter->lista_iter);
	if (lista_iter_al_final(iter->lista_iter)) {
        lista_iter_destruir(iter->lista_iter);
        /* Se actualiza la posición con la siguiente lista válida */
        iter->pos = buscar_siguiente_lista_hash(iter->hash, iter->pos + 1);
        if (!actualizar_lista_iter(iter))
            return false;
    }
    return true;
}

const char *hash_iter_ver_actual(const hash_iter_t *iter) {
	if (hash_iter_al_final(iter))
		return NULL;
    /* La clave está dentro del nodo dentro del nodo de la lista apuntado por lista_iter */
    /* lista_iter va a apuntar a algo válido, por lo que ninguna función va a devolver NULL */
    hash_nodo_t* nodo=lista_iter_ver_actual(iter->lista_iter);
    return nodo->clave;
}

bool hash_iter_al_final(const hash_iter_t *iter) {
    return (iter->pos == iter->hash->tam);
}

void hash_iter_destruir(hash_iter_t *iter) {
	if (iter->lista_iter)
	    lista_iter_destruir(iter->lista_iter);
	free(iter);
}


