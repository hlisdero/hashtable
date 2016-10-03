#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <string.h>
#define TAM_INICIAL 67
#define FACTOR_CARGA_MAX 2
#define FACTOR_CARGA_MIN 0.3
#define FACTOR_ACHIQUE 67
#define FACTOR_AGRANDAMIENTO 67

/* Definiciones de estructuras de la tabla de hash */

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
} nodo_t;

struct data_rehash_nodo {
    bool hubo_error;
    hash_t * hash;
};

/* Funciones del nodo */

static nodo_t* nodo_crear(const char *clave, void *dato) {
    /* Todo nodo debe tener una clave, no se crean nodos vacios */
    if (!clave) return NULL;
    nodo_t *nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    /* Reservar espacio para la clave */
    nuevo->clave = malloc((strlen(clave)+1)*sizeof(char));
    if (!(nuevo->clave)) {
        free(nuevo);
        return NULL;
    }
    /* Copiar la clave y guardar el dato */
    strcpy(nuevo->clave, clave);
    nuevo->dato = dato;
    return nuevo;
}

static bool nodo_esta_vacio(const nodo_t *nodo) {
    return (!nodo);
}

static char *nodo_ver_clave(nodo_t *nodo) {
    return (nodo_esta_vacio(nodo)? NULL: nodo->clave);
}

static void *nodo_ver_dato(nodo_t *nodo) {
    return (nodo_esta_vacio(nodo)? NULL: nodo->dato);
}

static void nodo_destruir(nodo_t *nodo, hash_destruir_dato_t destruir_dato) {
    if (!nodo) return;
    if (destruir_dato)
        destruir_dato(nodo->dato);
    free(nodo->clave);
    free(nodo);
}

/* Funciones auxiliares */

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
    nuevo->tam = TAM_INICIAL;
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

/* Crea la lista para un índice si es necesario y el iterador para recorrerla, devuelve false en caso de error */
static bool crear_lista_con_iter(hash_t * hash, lista_iter_t ** iter, size_t indice) {
    lista_iter_t *nuevo_iter;
    if (!hash->datos[indice]) {
        hash->datos[indice] = lista_crear();
        if (!hash->datos[indice])
            return false;
    }
    nuevo_iter = lista_iter_crear(hash->datos[indice]);
    if (!nuevo_iter) {
        destruir_lista_con_iter(hash, NULL, indice);
        return false;
    }
    *iter = nuevo_iter;
    return true;
}

/* Compara las claves, evitando pasarle NULL a strcmp, devuelve true si son iguales */
static bool comparar_claves(const char * clave1, const char * clave2) {
    if (!clave1 || !clave2)
        return false;
    else
        return !strcmp(clave1, clave2);
}

/* Busca la clave en la lista, devuelve true si la encuentra, false en caso contrario.
 * Deja el iterador en la clave si la encontró o al final en caso contrario.
 */
static bool buscar_clave_lista(const char * clave, lista_iter_t * iter) {
    char *clave_lista;
    /* Mientras se pueda avanzar, buscamos la clave */
    do {
        clave_lista = nodo_ver_clave(lista_iter_ver_actual(iter));
        if (comparar_claves(clave, clave_lista))
            return true;
    } while (lista_iter_avanzar(iter));
    return false;
}

/* Devuelve el indice a una lista distinta de NULL en la tabla de hash */
static size_t buscar_lista_hash(const hash_t * hash, size_t inicio) {
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

/* Implementación sencilla de la función de hash de K&R */
static size_t hash_conseguir_indice(const hash_t *hash, const char *clave) {
    size_t hashval;

    for (hashval = 0; *clave != '\0'; clave++)
        hashval = *clave + 31 * hashval;
	return hashval % hash->tam;
}

/* Destruye los nodos de la lista y la lista */
static void hash_lista_destruir(lista_t * lista, hash_destruir_dato_t destruir_dato) {
    while (!lista_esta_vacia(lista)) {
        nodo_destruir(lista_borrar_primero(lista), destruir_dato);
    }
    lista_destruir(lista, NULL);
}

/* Destruye las listas de la tabla de hash */
static void hash_listas_destruir(hash_t * hash) {
    size_t i = 0;

    while ((i = buscar_lista_hash(hash, i)) != hash->tam) {
        hash_lista_destruir(hash->datos[i], hash->destruir_dato);
        hash->datos[i] = NULL;
    }
}

/* Funciones de redimensionamiento (re-hash) del hash */

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

/* Rehashea un nodo, usando una estructura auxiliar con el hash nuevo y un booleano de salida */
static bool rehash_nodo(nodo_t * nodo, struct data_rehash_nodo * data) {
    if (!nodo) return false;

    data->hubo_error = hash_guardar(data->hash, nodo_ver_clave(nodo), nodo_ver_dato(nodo));
    if (data->hubo_error)
        return false;
    else
        return true;
}

/* Rehashea una lista, sin modificar los elementos de ella */
static bool rehash_lista(hash_t * hash, lista_t * lista) {
    struct data_rehash_nodo data;
    data.hash = hash;
    data.hubo_error = false;

    lista_iterar(lista, (bool (*)(void *, void*)) rehash_nodo, &data);
    if (data.hubo_error)
        return false;
    else
        return true;
}

static bool hash_redimensionar(hash_t * hash, size_t tam_nuevo) {
    size_t i = 0;
    hash_t * nuevo_hash = hash_crear_tam_variable(hash->destruir_dato, tam_nuevo);
    if (!nuevo_hash) return false;

    while ((i = buscar_lista_hash(hash, i)) != hash->tam) {
        if (!rehash_lista(nuevo_hash, hash->datos[i]))
            break;
    }
    /* Si copie todos los elementos, entonces terminó bien el while */
    if (hash->cantidad == nuevo_hash->cantidad) {
        /* Destruyo las listas y la tabla, que son las cosas que reemplazamos */
        hash_listas_destruir(hash);
        free(hash->datos);
        hash->datos = nuevo_hash->datos;
        hash->tam = tam_nuevo;
        free(nuevo_hash); // Solo elimino nuevo_hash como cascarón de la tabla nueva que creamos
        return true;
    } else {
        /* Si falló tengo que destruir todo el hash nuevo, sin excepción */
        hash_destruir(nuevo_hash);
        return false;
    }
}

/**************************************
 **  Primitivas de la Tabla de hash  **
 **************************************/

hash_t *hash_crear(hash_destruir_dato_t destruir_dato) {
    return hash_crear_tam_variable(destruir_dato, TAM_INICIAL);
}

/* Guarda un elemento en el hash, si la clave ya se encuentra en la
 * estructura, la reemplaza. De no poder guardarlo devuelve false.
 * Pre: La estructura hash fue inicializada
 * Post: Se almacenó el par (clave, dato)
 */
bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
    if (!clave) return false; // Debe recibir una clave válida
    size_t indice = hash_conseguir_indice(hash, clave);
    nodo_t *nuevo = nodo_crear(clave, dato);
    lista_iter_t *iter;
    if (!nuevo) return false;

    /* Se crea la lista si no existe, si la lista o el iterador de esa lista no se pueden crear devuelve false */
    if (!crear_lista_con_iter(hash, &iter, indice)) {
        nodo_destruir(nuevo, hash->destruir_dato);
        return false;
    }
    /* Busco la clave, si la encuentra tiene que borrar el elemento que va a ser reemplazado */
    if (buscar_clave_lista(clave, iter)) {
        nodo_destruir(lista_iter_borrar(iter), hash->destruir_dato); // El iter quedó en la posición del elemento repetido
        --(hash->cantidad);
    }
    /* Si no puedo insertarlo tengo que destruir el nodo, el iter y la lista si está vacía */
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

/* Borra un elemento del hash y devuelve el dato asociado.  Devuelve
 * NULL si el dato no estaba.
 * Pre: La estructura hash fue inicializada
 * Post: El elemento fue borrado de la estructura y se lo devolvió,
 * en el caso de que estuviera guardado.
 */
void *hash_borrar(hash_t *hash, const char *clave) {
	size_t indice = hash_conseguir_indice(hash,clave);
    lista_iter_t * iter;
    nodo_t * nodo_salida;
    void * dato_salida;

    /* Si la lista es NULL o no logra crear un iterador para recorrerla devuelve NULL */
	if(!hash->datos[indice] || !(iter = lista_iter_crear(hash->datos[indice])) )
        return NULL;
    /* Caso general */
    if (buscar_clave_lista(clave, iter)) {
        nodo_salida = lista_iter_borrar(iter); // El iter quedó en la posición que debemos borrar
        dato_salida = nodo_ver_dato(nodo_salida);
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

/* Obtiene el valor de un elemento del hash, si la clave no se encuentra
 * devuelve NULL.
 * Pre: La estructura hash fue inicializada
 */
void *hash_obtener(const hash_t *hash, const char *clave) {
	size_t indice = hash_conseguir_indice(hash, clave);
    lista_iter_t *iter;
    void *dato_salida;

    /* Si la lista es NULL o no logra crear un iterador para recorrerla devuelve NULL */
	if (!hash->datos[indice] || !(iter = lista_iter_crear(hash->datos[indice])))
	    return NULL;
    /* Caso general */
    if (buscar_clave_lista(clave, iter)) {
        /* El iter quedó en la posición del elemento que debemos devolver */
        dato_salida = nodo_ver_dato(lista_iter_ver_actual(iter));
        lista_iter_destruir(iter);
        return dato_salida;
    } else {
        lista_iter_destruir(iter);
        return NULL;
    }
}

/* Determina si clave pertenece o no al hash.
 * Pre: La estructura hash fue inicializada
 */
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
    hash_listas_destruir(hash);
    free(hash->datos);
    free(hash);
}

/****************************************
 **  Primitivas del Iterador del hash  **
 ****************************************/

// Crea iterador
hash_iter_t *hash_iter_crear(const hash_t *hash){
	hash_iter_t* iter = malloc(sizeof(*iter));
	if (!iter)
	    return NULL;
	iter->hash = hash;
    /* Hay que buscar una lista distinta de NULL y crear un iter para ella */
    iter->pos = buscar_lista_hash(hash, 0);
    if (!actualizar_lista_iter(iter)) {
        free(iter);
        return NULL;
    }
	return iter;
}

// Avanza iterador
bool hash_iter_avanzar(hash_iter_t *iter) {
	if (hash_iter_al_final(iter))
	    return false;
    lista_iter_avanzar(iter->lista_iter);
	if (lista_iter_al_final(iter->lista_iter)) {
        lista_iter_destruir(iter->lista_iter);
        iter->pos = buscar_lista_hash(iter->hash, iter->pos + 1);
        if (!actualizar_lista_iter(iter))
            return false;
    }
    return true;
}

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
const char *hash_iter_ver_actual(const hash_iter_t *iter) {
	if (hash_iter_al_final(iter))
		return NULL;
    /* La clave está dentro del nodo dentro del nodo de la lista apuntado por lista_iter */
    /* lista_iter va a apuntar a algo válido, por lo que ninguna función va a devolver NULL */
    return nodo_ver_clave(lista_iter_ver_actual(iter->lista_iter));
}

// Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t *iter) {
    return (iter->pos == iter->hash->tam);
}

// Destruye iterador
void hash_iter_destruir(hash_iter_t *iter) {
	if (iter->lista_iter)
	    lista_iter_destruir(iter->lista_iter);
	free(iter);
}
