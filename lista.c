#include "lista.h"
#include <stdlib.h>

/* Definicion de la estructura lista */
typedef struct nodo nodo_t;
struct nodo {
    void * dato;   // Puntero al dato guardado
    nodo_t * sig;   // Puntero al siguiente nodo
};

struct lista {
    nodo_t * primer;   // Puntero al primer nodo
    nodo_t * ultimo;   // Puntero al ultimo nodo
    size_t largo;
};

struct lista_iter {
    nodo_t * actual;
    nodo_t * anterior;
    lista_t * lista;
};

/* Funciones auxiliares */
static nodo_t * crear_nodo(void * dato) {
    nodo_t * nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    nuevo->dato = dato;
    nuevo->sig = NULL;
    return nuevo;
}

static void lista_destruir_nodos(nodo_t * nodo, void destruir_dato(void*)) {
    if (!nodo)
        return;
    if (destruir_dato)
        destruir_dato(nodo->dato);
    lista_destruir_nodos(nodo->sig, destruir_dato);
    free(nodo);
    return;
}

/* Primitivas de la lista */
lista_t *lista_crear(void) {
    lista_t * nueva = malloc(sizeof(*nueva));
    if (!nueva)
        return NULL;
    nueva->primer = NULL;
    nueva->ultimo = NULL;
    nueva->largo = 0;
    return nueva;
}

bool lista_esta_vacia(const lista_t *lista) {
    return (!lista->primer);
}

bool lista_insertar_primero(lista_t *lista, void *dato) {
    nodo_t * nuevo = crear_nodo(dato);
    if (!nuevo)
        return false;
    if (lista_esta_vacia(lista))
        lista->ultimo = nuevo;
    nuevo->sig = lista->primer;
    lista->primer = nuevo;
    ++(lista->largo);
    return true;
}

bool lista_insertar_ultimo(lista_t *lista, void *dato) {
    nodo_t * nuevo = crear_nodo(dato);
    if (!nuevo)
        return false;
    if (lista_esta_vacia(lista)) {
        lista->primer = nuevo;
    } else {
        lista->ultimo->sig = nuevo;
    }
    lista->ultimo = nuevo;
    ++(lista->largo);
    return true;
}

void *lista_borrar_primero(lista_t *lista) {
    void * primer_dato;
    nodo_t * primer_nodo = lista->primer;
    if (lista_esta_vacia(lista))
        return NULL;
    /* Si primer era el único nodo, entonces último es NULL */
    if (!lista->primer->sig)
        lista->ultimo = NULL;
    primer_dato = lista->primer->dato;
    lista->primer = lista->primer->sig;
    --(lista->largo);
    free(primer_nodo);
    return primer_dato;
}

void *lista_ver_primero(const lista_t *lista) {
    return (lista_esta_vacia(lista) ? NULL : lista->primer->dato);
}

size_t lista_largo(const lista_t *lista) {
    return lista->largo;
}

void lista_destruir(lista_t *lista, void destruir_dato(void *)) {
    if (!lista)
        return;
    lista_destruir_nodos(lista->primer, destruir_dato);
    free(lista);
    return;
}

/* Primitivas del iterador */
lista_iter_t *lista_iter_crear(lista_t *lista) {
    lista_iter_t * nuevo;
    /* La lista debe estar creada */
    if (!lista) return NULL;

    nuevo = malloc(sizeof(*nuevo));
    if (!nuevo)
        return NULL;
    nuevo->actual = lista->primer;
    nuevo->anterior = NULL;
    nuevo->lista = lista;
    return nuevo;
}

bool lista_iter_avanzar(lista_iter_t *iter) {
    if (lista_iter_al_final(iter))
        return false;
    iter->anterior = iter->actual;
    iter->actual = iter->actual->sig;
    return true;
}

void *lista_iter_ver_actual(const lista_iter_t *iter) {
    return (lista_iter_al_final(iter) ? NULL : iter->actual->dato);
}

bool lista_iter_al_principio(const lista_iter_t *iter) {
    return (!iter->anterior);
}

bool lista_iter_al_final(const lista_iter_t *iter) {
    return (!iter->actual);
}

void lista_iter_destruir(lista_iter_t *iter) {
    free(iter);
}

bool lista_iter_insertar(lista_iter_t *iter, void *dato) {
    nodo_t * nuevo = crear_nodo(dato);
    if (!nuevo)
        return false;

    /* Si está al final de la lista el último va a ser el nodo insertado */
    if (lista_iter_al_final(iter))
        iter->lista->ultimo = nuevo;
    /* Si está al principio de la lista el primer va a ser el nodo insertado */
    if (lista_iter_al_principio(iter)) {
        iter->lista->primer = nuevo;
    } else {
        iter->anterior->sig = nuevo;
    }
    nuevo->sig = iter->actual;
    iter->actual = nuevo;
    ++(iter->lista->largo);
    return true;
}

void *lista_iter_borrar(lista_iter_t *iter) {
    void * dato;
    nodo_t * aux;
    /* Nada para borrar */
    if (lista_iter_al_final(iter))
        return NULL;

    aux = iter->actual->sig;
    /* Si está al principio el primer va a ser el siguiente al actual */
    if (lista_iter_al_principio(iter)) {
        iter->lista->primer = aux;
    } else {
        iter->anterior->sig = aux; /* Caso general */
    }
    /* Si el actual era el último, el último va a ser el anterior */
    if (!aux) {
        iter->lista->ultimo = iter->anterior;
    }
    dato = iter->actual->dato;
    free(iter->actual);
    iter->actual = aux;
    --(iter->lista->largo);
    return dato;
}

/* Primitivas de iterador interno */
void lista_iterar(lista_t *lista, bool (*visitar)(void *dato, void *extra), void *extra) {
    nodo_t * actual;

    for (actual = lista->primer; actual != NULL; actual = actual->sig) {
        if (!visitar(actual->dato, extra)) {
            return;
        }
    }
}
