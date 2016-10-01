#ifndef LISTA_H
#define LISTA_H

#include <stdbool.h>
#include <stddef.h>

/* Declaraciones de estructuras */
typedef struct lista lista_t;
typedef struct lista_iter lista_iter_t;

/* Primitivas de la lista */

/* Crea una lista. Devuelve NULL en caso de error.
 * Post: devuelve una nueva lista vacía.
 */
lista_t *lista_crear(void);

/* Devuelve verdadero o falso, según si la lista tiene o no elementos insertados.
 * Pre: la lista fue creada.
 */
bool lista_esta_vacia(const lista_t *lista);

/* Agrega un nuevo elemento al principio de la lista. Devuelve falso en caso de error.
 * Pre: la lista fue creada.
 * Post: se agregó un nuevo elemento a la lista, valor se encuentra al principio de la lista
 */
bool lista_insertar_primero(lista_t *lista, void *dato);

/* Agrega un nuevo elemento al final de la lista. Devuelve falso en caso de error.
 * Pre: la lista fue creada.
 * Post: se agregó un nuevo elemento a la lista, valor se encuentra al final de la lista.
 */
bool lista_insertar_ultimo(lista_t *lista, void *dato);

/* Saca el primer elemento de la lista. Si la lista tiene elementos, se quita el
 * primero de la lista, y se devuelve su valor, si está vacía, devuelve NULL.
 * Pre: la lista fue creada.
 * Post: se devolvió el valor del primer elemento anterior, la lista
 * contiene un elemento menos, si la lista no estaba vacía.
 */
void *lista_borrar_primero(lista_t *lista);

/* Obtiene el valor del primer elemento de la lista. Si la lista tiene
 * elementos, se devuelve el valor del primero, si está vacía devuelve NULL.
 * Pre: la lista fue creada.
 * Post: se devolvió el primer elemento de la lista, cuando no está vacía.
 */
void *lista_ver_primero(const lista_t *lista);

/* Devuelve el largo de la lista.
 * Pre: la lista fue creada.
 */
size_t lista_largo(const lista_t *lista);

/* Destruye la lista. Si se recibe la función destruir_dato por parámetro,
 * para cada uno de los elementos de la lista llama a destruir_dato.
 * Pre: destruir_dato es una función capaz de destruir
 * los datos de la lista, o NULL en caso de que no se la utilice.
 * Post: se eliminaron todos los elementos de la lista.
 */
void lista_destruir(lista_t *lista, void destruir_dato(void *));

/* Primitivas del iterador */

/* Crea un nuevo iterador para la lista
 * Devuelve NULL en caso de error.
 * Pre: la lista fue creada.
 * Post: devuelve un iterador situado en el primer elemento de la lista, si es que existe
 */
lista_iter_t *lista_iter_crear(lista_t *lista);

/* Avanza el iterador a la posición siguiente.
 * Devuelve false si iter está al final de la lista, true en caso contrario.
 * Pre: el iterador fue creado, la lista fue creada.
 */
bool lista_iter_avanzar(lista_iter_t *iter);

/* Devuelve el dato almacenado en la posición actual. Devuelve NULL si falla.
 * Pre: el iterador fue creado, la lista fue creada.
 */
void *lista_iter_ver_actual(const lista_iter_t *iter);

/* Devuelve true si el iterador está al principio, false en caso contrario
 * Pre: el iterador fue creado, la lista fue creada.
 */
bool lista_iter_al_principio(const lista_iter_t *iter);

/* Devuelve true si el iterador está al final, false en caso contrario
 * Pre: el iterador fue creado, la lista fue creada.
 */
bool lista_iter_al_final(const lista_iter_t *iter);

/* Destruye el iterador */
void lista_iter_destruir(lista_iter_t *iter);

/* Inserta un elemento en la posición actual del iterador. Devuelve false si falló.
 * Deja el iterador en la posición del elemento insertado.
 * Pre: el iterador fue creado, la lista fue creada.
 * Post: el elemento insertado es el actual, el que estaba en esa posición es el siguiente.
 */
bool lista_iter_insertar(lista_iter_t *iter, void *dato);

/* Borra el elemento en la posición actual del iterador. Devuelve NULL si falló.
 * Deja el iterador en la posición del elemento siguiente al borrado.
 * Pre: el iterador fue creado, la lista fue creada.
 * Post: el elemento anterior tiene como siguiente al siguiente del elemento borrado.
 */
void *lista_iter_borrar(lista_iter_t *iter);

/* Primitivas de iterador interno */
/* Itera sobre la lista hasta que lista se termine o hasta que la función visitar devuelva false.
 * El parámetro extra se pasa a la función visitar. La función visitar puede utilizarlo o no.
 * Pre: la lista fue creada.
 */
void lista_iterar(lista_t *lista, bool (*visitar)(void *dato, void *extra), void *extra);

/* Ejecuta pruebas unitarias sobre la implementación */
void pruebas_lista_alumno(void);

#endif // LISTA_H
