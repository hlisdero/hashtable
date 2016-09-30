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
}nodo_t;


/* Funciones del nodo */

nodo_t* nodo_crear(char *clave, void *dato) {
    /* Todo nodo debe tener una clave, no se crean nodos vacios */
    if (!clave) return NULL;
    nodo_t *nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    /* Reservar espacio para la clave */
    nuevo->clave = malloc((strlen(clave)+1));
    if (!(nuevo->clave)) {
        free(nuevo);
        return NULL;
    }
    /* Copiar la clave y guardar el dato */
    strcpy(nuevo->clave, clave);
    nuevo->dato = dato;
    return nuevo;
}

char *nodo_ver_clave(nodo_t nodo) {
    return nodo->clave;
}

void *nodo_ver_dato(nodo_t nodo) {
    return nodo->dato;
}

void nodo_destruir(nodo_t nodo, destruir_dato_t destruir_dato) {
    if (!nodo) return NULL;
    if (destruir_dato)
        destruir_dato(nodo->dato);
    free(nodo->clave);
    free(nodo);
}

/* Funciones auxiliares */
static bool crear_lista_para_hash(lista_t ** lista) {
    lista_t *aux = lista_crear();
    if (!aux)
        return false;
    *lista = aux;
    return true;
}

static bool crear_iter_para_hash(lista_iter_t ** iter) {
    lista_iter_t *aux = lista_iter_crear();
    if (!aux)
        return false;
    *iter = aux;
    return true;
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

size_t hash_conseguir_indice(const hash_t *hash, const char *clave){
    unsigned char* tempstr;
    tempstr=(unsigned char*)clave;
    unsigned long indice = 5381;
    unsigned int c;
    
    while((c=*tempstr++)!=0){
		indice=((indice<<5)+indice)+c; /* indice * 33 + c */
	}
	return indice%hash->tam;
}

/* Guarda un elemento en el hash, si la clave ya se encuentra en la
 * estructura, la reemplaza. De no poder guardarlo devuelve false.
 * Pre: La estructura hash fue inicializada
 * Post: Se almacenó el par (clave, dato)
 */
bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
    size_t indice = hash_conseguir_indice(hash, clave);
    nodo_t *nuevo = nodo_crear(clave, dato);
    lista_iter_t *iter;
    if (!nuevo) return false;

    /* Se crea la lista si no existe, si la lista no se puede crear devuelve false */
    if (!hash->datos[i] || !crear_lista_para_hash(hash->datos+i) || !crear_iter_para_hash(&iter))
        lista_destruir(hash->datos[i]);
        nodo_destruir(nuevo);
        return false;
    if (buscar_clave_lista(iter)) {
        lista_iter_borrar(iter);
    }
    lista_iter_insertar(iter, nuevo);
    return true;
}

/* Borra un elemento del hash y devuelve el dato asociado.  Devuelve
 * NULL si el dato no estaba.
 * Pre: La estructura hash fue inicializada
 * Post: El elemento fue borrado de la estructura y se lo devolvió,
 * en el caso de que estuviera guardado.
 */
void *hash_borrar(hash_t *hash, const char *clave){
	if(hash->cantidad==0)
	    return NULL;
	size_t indice=hash_conseguir_indice(hash,clave);
	if(hash->datos[indice]==NULL)
	    return NULL;
	else{
		lista_iter_t* iter = lista_iter_crear(hash->datos[indice]);
		if (iter == NULL)
			return NULL;
		void* dato=lista_iter_ver_actual(iter);
		hash->cantidad--;
		lista_destruir(hash->datos[indice],NULL);
		hash->datos[indice]=NULL;
		lista_iter_destruir(iter);
		return dato;
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
		void* dato=lista_iter_ver_actual(iter);
		while(!lista_iter_al_final(iter)){
			dato=lista_iter_ver_actual(iter);
		    lista_iter_avanzar(iter);
		}    
		lista_iter_destruir(iter);
		return dato;
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
	return true;
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
    int i;
    for (i = 0; i < hash->tam-1; i++) {
		if(hash->datos[i]!=NULL)
            lista_destruir(hash->datos[i], hash->destruir_dato); // lista_destruir no falla con NULL
    }
    free(hash->datos);
    free(hash);
}

/****************************************
 **  Primitivas del Iterador del hash  **
 ****************************************/

// Crea iterador
hash_iter_t *hash_iter_crear(const hash_t *hash){
	hash_iter_t* iter=malloc(sizeof(hash_iter_t));
	if(iter==NULL)
	    return NULL;
	iter->hash=hash;
	iter->lista_iter=NULL;
	iter->pos=0;
	return iter;
}

// Avanza iterador
bool hash_iter_avanzar(hash_iter_t *iter);

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
const char *hash_iter_ver_actual(const hash_iter_t *iter) {
    return (hash_iter_al_final(iter) ? NULL: nodo_ver_clave(lista_iter_ver_actual(iter->lista_iter)));
}

// Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t *iter) {
    return (iter->pos == iter->hash->tam);
}

// Destruye iterador
void hash_iter_destruir(hash_iter_t *iter) {
    lista_iter_destruir(iter->lista_iter);
    free(iter);
}

