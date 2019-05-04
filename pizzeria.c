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

sem_t sGarcons, sMesas, sLockMesas;

pthread_mutex_t pegaFatia;

queue_t smartDeck;

//pizzaiolos
sem_t sPizzaiolos;

int pizzariaAberta = 0; //famigerado True


void *pizzaiolo(void *arg){
  sem_wait(&sPizzaiolos);
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("chamou init");
  sem_init(&sGarcons, 0,n_garcons);
  pthread_mutex_init(&pegaFatia, NULL);

  //para pegar as mesas:
  sem_init(&sMesas, 0, n_mesas);
  sem_init(&sLockMesas,0, 1); // decidi usar semaforo para que haja fila de clientes
  //se fosse mutex, qualquer um poderia pegar quando liberasse. semaforo gera fila.

  //pedidos
  queue_init(&smartDeck, tam_deck);
  
  //pizzaiolos
  sem_init(&sPizzaiolos,0, 0);
  pthread_t pizzaiolos[n_pizzaiolos];
  for (int i = 0; i < n_pizzaiolos; i++) {
    pthread_create(&pizzaiolos[i], NULL, pizzaiolo, NULL);
  }
  for (int i = 0; i < n_pizzaiolos; i++) {
    pthread_join(threads[i], NULL);
  }
}

void pizzeria_close() {
  //se pah a funcao eh soh isso:
  pizzariaAberta = 1; //famigerado false
}

void pizzeria_destroy() {
  sem_destroy(&sGarcons);
  pthread_mutex_destroy(&pegaFatia);
  sem_destroy(&sMesas);
  sem_destroy(&sLockMesas);
  queue_destroy(&smartDeck);
  //pizzaiolos
  sem_destroy(&sPizzaiolos);
}

void pizza_assada(pizza_t* pizza) {
  printf("pizza_assada\n");
}

int numMesas(int tam_grupo){
  int numeroDeMesas = 0;
  int tam = tam_grupo;
  while (tam > 0){
    numeroDeMesas++;
    tam-=4;
  }
  return numeroDeMesas;
}

int pegar_mesas(int tam_grupo) {

  //printf("numeroDeMesas: %d; tam_grupo: %d\n", numeroDeMesas, tam_grupo); ok!

  if (pizzariaAberta) {
      printf("pegar_mesas\n");
      int numeroDeMesas = numMesas(tam_grupo); //funcao que calcula o numero de mesas necessárias para o grupo
    
    /*Logica de escolher mesas
    //
    //acho que o canale eh um grupo de clientes pegar mesas de cada vez,
    //dae ele pega todas as mesas que precisa e entao destrava pros outros
    //se usar 2 semaforos ao inves de um mutex e um semaforo, acho q fica
    //fila mais justa pros clientes
    */
    sem_wait(&sLockMesas);
      if(!pizzariaAberta){ //caso a pizzaria fechou enquanto esperava
        sem_post(&sLockMesas);
        return -1;
      }
      //caso padrao eh alocar umas mesas
      for (int i = numeroDeMesas; i > 0; i--){
          sem_wait(&sMesas);
      }
    sem_post(&sLockMesas);
    return 0;
  }
  return -1;
}

void garcom_tchau(int tam_grupo) {
  
  printf("garcom_tchau\n");
  for(int i = 0; i<numMesas(tam_grupo); i++){
    sem_post(&sMesas); // Libera as mesas quando sinaliza que o grupo vai embora
  }
  sem_post(&sGarcons);
}

void garcom_chamar() {
  printf("garcom_tchau\n");
  sem_wait(&sGarcons); //se pah essa função eh só isso msm
}

void fazer_pedido(pedido_t* pedido) { // se pah ta pronto, reler no trabalho
  queue_push_back(&smartDeck, (void*) pedido);
  sem_post(&sPizzaiolos);
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

//Beleza agora consigo editar
