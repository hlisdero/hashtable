#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

nodo_t* nodo_crear(const char *clave, void *dato) {
    /* Todo nodo debe tener una clave, no se crean nodos vacios */
    if (!clave) return NULL;
    nodo_t *nuevo = malloc(sizeof(nodo_t));
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

char *nodo_ver_clave(nodo_t* nodo) {
    return nodo->clave;
}

void *nodo_ver_dato(nodo_t* nodo) {
    return nodo->dato;
}

void nodo_destruir(nodo_t* nodo, hash_destruir_dato_t destruir_dato) {
    if (destruir_dato)
        destruir_dato(nodo->dato);
    free(nodo->clave);
    free(nodo);
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

static size_t hash_conseguir_indice(const hash_t *hash, const char *clave){
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
    nodo_t* nodo;
    if(hash->datos[indice]==NULL){
		hash->datos[indice]=lista_crear();
		nodo=nodo_crear(clave,dato);
		if(lista_insertar_ultimo(hash->datos[indice],nodo)){
			hash->cantidad++;
			/*MIRAR REDIMENSIONAR*/
			return true;
	    }
	    else{
			free(nodo->clave);
			free(nodo);
			return false;
		}
	}else{
		lista_iter_t* iter=lista_iter_crear(hash->datos[indice]);
		if(iter==NULL)
		    return false;
		bool encontrado=false;
		nodo_t* nodo;
		while((!encontrado)&&(!lista_iter_al_final(iter))){
			nodo=lista_iter_ver_actual(iter);
			if(strcmp(nodo->clave,clave)==0){
				encontrado=true;
				if(hash->destruir_dato!=NULL)
				    hash->destruir_dato(nodo->dato);
				nodo->dato=dato;
				lista_iter_destruir(iter);
				return true;
			}else{
				lista_iter_avanzar(iter);
			}
		}
		lista_iter_destruir(iter);
		//Esto ocurre solo si no encontró al elemento
		
		nodo=nodo_crear(clave,dato);
		if(lista_insertar_ultimo(hash->datos[indice],nodo)){
			hash->cantidad++;
			/*hash_revisar_redimensionamiento(hash)*/;
			return true;
		}else{
			free(nodo);
			return false;
		}
	}
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
		nodo_t* nodo;
		while(!lista_iter_al_final(iter)){
			nodo=lista_iter_ver_actual(iter);
			if(strcmp(nodo->clave,clave)==0){
				nodo_t* elem=lista_iter_borrar(iter);
				lista_iter_destruir(iter);
				void* dato=elem->dato;
				free(elem->clave);
				free(elem);
				hash->cantidad--;
				if(lista_esta_vacia(hash->datos[indice])){
					lista_destruir(hash->datos[indice],NULL);
					hash->datos[indice]=NULL;
				}
		        /* REVISAR REDIMENSIONAR */
		        return dato;
	        }else
	            lista_iter_avanzar(iter);
	    }
	    lista_iter_destruir(iter);
	    return NULL;
	}
}

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
			

	
size_t hash_cantidad(const hash_t *hash){
	return hash->cantidad;
}

void hash_destruir(hash_t *hash) {
    size_t elem_a_borrar=hash->cantidad;
    lista_t* lista;
    nodo_t* nodo;
    int i=0;
    while((elem_a_borrar>0) && (i<hash->tam)){
		lista=hash->datos[i];
		if(lista!=NULL){
			while(!lista_esta_vacia(lista)){
				nodo=lista_borrar_primero(lista);
				free(nodo->clave);
				if(hash->destruir_dato!=NULL)
				    hash->destruir_dato(nodo->dato);
				free(nodo);
				elem_a_borrar--;
			}
            lista_destruir(hash->datos[i], hash->destruir_dato); // lista_destruir no falla con NULL
         }
         i++;
    }
    free(hash->datos);
    free(hash);
}


			
static bool hash_iter_avance_interno(hash_iter_t* iter,size_t* i,lista_iter_t** aux){
	while((*aux==NULL)&&(*i<(iter->hash->tam))){
		if(iter->hash->datos[*i]!=NULL){
			*aux=lista_iter_crear(iter->hash->datos[*i]);
			if(*aux==NULL)
			    return false;
			if(lista_iter_al_final(*aux)){
				lista_iter_destruir(*aux);
				*aux=NULL;
			}
		}else{
			(*i)++;
		}
	}
	return true;
}
		


			
hash_iter_t *hash_iter_crear(const hash_t *hash){
	hash_iter_t* iter=malloc(sizeof(hash_iter_t));
	if(iter==NULL)
	    return NULL;
	iter->hash=hash;
	iter->lista_iter=NULL;
	iter->pos=0;
	//se inicializa de tener elementos
	if(hash->cantidad>0){
		size_t i=0;
		lista_iter_t* tempact=NULL;
		if(!hash_iter_avance_interno(iter,&i,&tempact)){
			free(iter);
			return NULL;
		}else{
			iter->lista_iter=tempact;
			iter->pos=i;
		}
	}
	return iter;
}

bool hash_iter_avanzar(hash_iter_t *iter){
	if(hash_iter_al_final(iter))
	    return false;
	if(lista_iter_al_final(iter->lista_iter)){
		size_t i=iter->pos+1;
		lista_iter_t* aux=NULL;
		if(hash_iter_avance_interno(iter,&i,&aux)){
			lista_iter_destruir(iter->lista_iter);
			iter->lista_iter=aux;
			iter->pos=i;
			return true;
		}else
			return false;
	}else if (lista_iter_avanzar(iter->lista_iter)){
		    if(lista_iter_al_final(iter->lista_iter)){
			    size_t i=iter->pos+1;
			    lista_iter_t* aux=NULL;
			    if(hash_iter_avance_interno(iter,&i,&aux)){
				    lista_iter_destruir(iter->lista_iter);
				    iter->lista_iter=aux;
				    iter->pos=i;
				    return true;
			    }else
				    return false;
		    }else
			    return true;
	    }else
		    return false;
}

const char *hash_iter_ver_actual(const hash_iter_t *iter){
	if(hash_iter_al_final(iter)){
		return NULL;
	}else{
		lista_iter_t* iter_actual=iter->lista_iter;
		nodo_t* nodo_actual=lista_iter_ver_actual(iter_actual);
		return nodo_actual->clave;
	}
}

bool hash_iter_al_final(const hash_iter_t *iter) {
    return (iter->hash->cantidad==0 || iter->hash->tam==iter->pos);
}

void hash_iter_destruir(hash_iter_t* iter){
	if(iter->lista_iter!=NULL)
	    lista_iter_destruir(iter->lista_iter);
	free(iter);
}




