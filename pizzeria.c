#include "pizzeria.h"
#include "queue.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

//testes:
//default:
//INE5410_INFO=1 ./program 2 2 40 3 8 40 10

sem_t sGarcons;

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("chamou init");
  sem_init(&sGarcons, 0,n_garcons);
}

void pizzeria_close() {
}

void pizzeria_destroy() {
  sem_destroy(&sGarcons);
}

void pizza_assada(pizza_t* pizza) {
}

int pegar_mesas(int tam_grupo) {
    return -1; //erro: não fui implementado (ainda)!
}

void garcom_tchau(int tam_grupo) {
  sem_post(&sGarcons);
  //implementar logica de liberar as mesas
}

void garcom_chamar() {
  sem_wait(&sGarcons); //se pah essa função eh só isso msm
}

void fazer_pedido(pedido_t* pedido) {
}

int pizza_pegar_fatia(pizza_t* pizza) {
  //na linha 215 o helper atribui 12 a pizza->fatias
  return -1; // erro: não fui implementado (ainda)!
}
