#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <string.h>
#define TAM_INICIAL 67

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

static bool nodo_esta_vacio(nodo_t *nodo) {
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

/* Crea la lista para un índice y el iterador para recorrerla, devuelve false en caso de error */
static bool crear_lista_con_iter(lista_t ** lista, lista_iter_t ** iter) {
    lista_t *nueva_lista = lista_crear();
    lista_iter_t *nuevo_iter;
    if (!nueva_lista)
        return false;
    nuevo_iter = lista_iter_crear(nueva_lista);
    if (!nuevo_iter) {
        lista_destruir(nueva_lista, NULL);
        return false;
    }
    *lista = nueva_lista;
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

static size_t buscar_lista_hash(const hash_t * hash, size_t inicio) {
    size_t i;

    for (i = inicio; i < hash->tam; i++) {
        if (hash->datos[i])
            return i;
    }
    return hash->tam;
}

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

static size_t hash_conseguir_indice(const hash_t *hash, const char *clave) {
    unsigned char* tempstr;
    tempstr=(unsigned char*)clave;
    unsigned long indice = 5381;
    unsigned int c;

    while((c=*tempstr++)!=0){
		indice=((indice<<5)+indice)+c; /* indice * 33 + c */
	}
	return indice%hash->tam;
}

static void hash_lista_destruir(lista_t * lista, hash_destruir_dato_t destruir_dato) {
    while (!lista_esta_vacia(lista)) {
        nodo_destruir(lista_borrar_primero(lista), destruir_dato);
    }
    lista_destruir(lista, NULL);
}

/**************************************
 **  Primitivas de la Tabla de hash  **
 **************************************/

hash_t *hash_crear(hash_destruir_dato_t destruir_dato) {
    hash_t *nuevo = malloc(sizeof(*nuevo));
    lista_t **datos = calloc(TAM_INICIAL, sizeof(*datos));
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
    if (!hash->datos[indice] && !crear_lista_con_iter(hash->datos+indice, &iter) ) {
        nodo_destruir(nuevo, hash->destruir_dato);
        return false;
    }
    /* Busco la clave, si la encuentra tiene que borrar el elemento que va a ser reemplazado */
    if (buscar_clave_lista(clave, iter)) {
        lista_iter_borrar(iter); // El iter quedó en la posición del elemento repetido
    }
    lista_iter_insertar(iter, nuevo);
    lista_iter_destruir(iter);
    ++(hash->cantidad);
    return true;
}

/* Borra un elemento del hash y devuelve el dato asociado.  Devuelve
 * NULL si el dato no estaba.
 * Pre: La estructura hash fue inicializada
 * Post: El elemento fue borrado de la estructura y se lo devolvió,
 * en el caso de que estuviera guardado.
 */
void *hash_borrar(hash_t *hash, const char *clave){
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
        lista_iter_destruir(iter);
        /* Caso que la lista quedo vacía, la borramos para evitar problemas después */
        if (lista_esta_vacia(hash->datos[indice])) {
            lista_destruir(hash->datos[indice], NULL);
            hash->datos[indice] = NULL;
        }
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
void *hash_obtener(const hash_t *hash, const char *clave){
	if(hash->cantidad==0)
	    return NULL;
	size_t indice = hash_conseguir_indice(hash, clave);
	if(hash->datos[indice]==NULL)
	    return NULL;
	else{
		lista_iter_t* iter=lista_iter_crear(hash->datos[indice]);
		if(iter==NULL)
		    return NULL;
		nodo_t* nodo;
		while(!lista_iter_al_final(iter)){
			nodo=lista_iter_ver_actual(iter);
			if(strcmp(nodo->clave,clave)==0){
				lista_iter_destruir(iter);
				return nodo->dato;
			}else
		        lista_iter_avanzar(iter);
		}
		lista_iter_destruir(iter);
		return NULL;
	}
}
/* Determina si clave pertenece o no al hash.
 * Pre: La estructura hash fue inicializada
 */
bool hash_pertenece(const hash_t *hash, const char *clave){
	if(hash->cantidad==0)
	    return false;
	size_t indice=hash_conseguir_indice(hash,clave);
	if(hash->datos[indice]==NULL)
	    return false;
	else{
		lista_iter_t* iter=lista_iter_crear(hash->datos[indice]);
		if (iter==NULL){
			return false;
		}
		nodo_t* nodo;
		while(!lista_iter_al_final(iter)){
			nodo=lista_iter_ver_actual(iter);
			if (strcmp(nodo->clave,clave)==0){
				lista_iter_destruir(iter);
				return true;
			}else
			    lista_iter_avanzar(iter);
		}
		lista_iter_destruir(iter);
		return false;
	}
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
    size_t i = 0;

    while ((i = buscar_lista_hash(hash, i)) != hash->tam) {
        hash_lista_destruir(hash->datos[i], hash->destruir_dato);
        hash->datos[i] = NULL;
    }
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

