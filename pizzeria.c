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

pthread_mutex_t pegaFatia;

int pizzariaAberta = 0; //famigerado True

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("chamou init");
  sem_init(&sGarcons, 0,n_garcons);
  pthread_mutex_init(&pegaFatia, NULL);
}

void pizzeria_close() {
}

void pizzeria_destroy() {
  sem_destroy(&sGarcons);
  pthread_mutex_destroy(&pegaFatia);
}

void pizza_assada(pizza_t* pizza) {
}

int pegar_mesas(int tam_grupo) {
  if (pizzariaAberta) {
    //TODO: logica de escolher mesas
    //acho que o canale eh um grupo de clientes pegar mesas de cada vez,
    //dae ele pega todas as mesas que precisa e entao destrava pros outros
    //se usar 2 semaforos ao inves de um mutex e um semaforo, acho q fica
    //fila mais justa pros clientes
    return 0;
  }
  return -1;
}

void garcom_tchau(int tam_grupo) {
  sem_post(&sGarcons);
  //TODO: implementar logica de liberar as mesas
}

void garcom_chamar() {
  sem_wait(&sGarcons); //se pah essa função eh só isso msm
}

void fazer_pedido(pedido_t* pedido) {
}

int pizza_pegar_fatia(pizza_t* pizza) {
  //na linha 215 o helper atribui 12 a pizza->fatias
  pthread_mutex_lock(&pegaFatia);
    if (pizza->fatias > 0){
      pizza->fatias--;
      pthread_mutex_unlock(&pegaFatia);
      return 0;
    } else {
      pthread_mutex_unlock(&pegaFatia);
      return -1;
    }
}
