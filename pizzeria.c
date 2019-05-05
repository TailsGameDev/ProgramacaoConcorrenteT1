#include "pizzeria.h"
#include "queue.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

//testes:
//s
//default:
//INE5410_INFO=1 ./program 2 2 40 3 8 40 10

//garcons
sem_t sGarcons;

//alocacao de mesas
sem_t sMesas, sLockMesas;

//pegador de fatias, dos clientes
pthread_mutex_t pegaFatia;

//pedidos
queue_t smartDeck;
sem_t sProduzPedido, sConsomePedido;

//forno
sem_t sForno; // <- controla tamanho do forno
pizza_t** pizzaDoPizzaiolo; //os 3 arrays controlam a situacao de colocar
pthread_mutex_t* esperaTuaPizzaAssar;//uma pizza do forno e tirar a mesma assada
int* indicesPizzaiolo;

//espaco ao lado da smart deck
pthread_mutex_t espacoParaPizza;

//pizzaiolos
pthread_t *pizzaiolos;
int tamanhoArrayPizzaiolos;
pthread_mutex_t pahDePizza;

int pizzariaAberta = 1; //famigerado True

//smart deck usa logica do buffer circular (produtor e consumidor)
void *pizzaiolo(void *arg){
  int i = *((int*)arg); // identificador do pizzaiolo
  while (pizzariaAberta){
    //pega pedido da smart deck
    pedido_t *pedido;
    sem_wait(&sConsomePedido);
    pedido = (pedido_t*) queue_wait(&smartDeck);
    sem_post(&sProduzPedido);

    //monta pizza
    pizzaDoPizzaiolo[i] = pizzaiolo_montar_pizza(pedido);

    //poe pizza no forno
    pthread_mutex_lock(&pahDePizza);
    sem_wait(&sForno); // <- ocupa um espaco no forno
    pthread_mutex_lock(&esperaTuaPizzaAssar[i]); //o unlock eh qd fica pronta
    pizzaiolo_colocar_forno(pizzaDoPizzaiolo[i]);
    pthread_mutex_unlock(&pahDePizza);

    //tira pizza do forno
    pthread_mutex_lock(&esperaTuaPizzaAssar[i]); //para ateh ficar pronta
    pthread_mutex_lock(&pahDePizza);
    pizzaiolo_retirar_forno(pizzaDoPizzaiolo[i]);
    sem_post(&sForno); // desocupa espaco do forno
    pthread_mutex_unlock(&pahDePizza);
    pthread_mutex_unlock(&esperaTuaPizzaAssar[i]); //destrava pra proxima iteracao

    //espera-se que a pizza esteja pronta aqui.
    printf("pizzaiolo %d terminou uma pizza\n",i);

    //poe a pizza em local seguro
    pthread_mutex_lock(&espacoParaPizza);

    //chama o garcom e lhe entrega a pizza
    sem_wait(&sGarcons);
    pthread_mutex_unlock(&espacoParaPizza);

    //######fazer uma thread para o garcom entregar ##########
    garcom_entregar(pizzaDoPizzaiolo[i]);
    sem_post(&sGarcons);

  }
  pthread_exit(NULL);
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("######LISTINHA DO QUE FALTA FAZER######\n\n");
  printf("pizzaiolo:\n");
  printf("garcom levar pizza ser uma thread\n");
  printf("pegador de pizza individual de cada pizza\n");
  printf("\n######FIM DA LISTINHA DO QUE FALTA FAZER######\n");
  fflush(NULL);

  //garcons
  sem_init(&sGarcons, 0,n_garcons);

  //alocacao de mesas
  sem_init(&sMesas, 0, n_mesas);
  sem_init(&sLockMesas,0, 1); // decidi usar semaforo para que haja fila de clientes
  //se fosse mutex, qualquer um poderia pegar quando liberasse. semaforo gera fila.

  //pegador de fatias, dos clientes
  pthread_mutex_init(&pegaFatia, NULL);

  //pedidos
  queue_init(&smartDeck, tam_deck);
  sem_init(&sProduzPedido, 0 , tam_deck);
  sem_init(&sConsomePedido, 0 , 0);

  //forno
  sem_init(&sForno,0,tam_forno);
  pizzaDoPizzaiolo = malloc(n_pizzaiolos*sizeof(pizza_t*));
  esperaTuaPizzaAssar = (pthread_mutex_t*) malloc(n_pizzaiolos*sizeof(pthread_mutex_t));
  indicesPizzaiolo = malloc(n_pizzaiolos*sizeof(int*));

  //espaco ao lado da smart deck
  pthread_mutex_init(&espacoParaPizza, NULL);

  //pizzaiolos

  pthread_mutex_init(&pahDePizza, NULL);
  tamanhoArrayPizzaiolos = n_pizzaiolos;
  pizzaiolos = (pthread_t*) malloc(n_pizzaiolos*sizeof(pthread_t));
  for (int i = 0; i < n_pizzaiolos; i++) { //i servirah como identificador
    indicesPizzaiolo[i] = i;
    pthread_mutex_init(&esperaTuaPizzaAssar[i], NULL);
    pthread_create(&pizzaiolos[i], NULL, pizzaiolo, (void*) &indicesPizzaiolo[i]);
  }

}

void pizzeria_close() {
  //se pah a funcao eh soh isso:
  pizzariaAberta = 0; //famigerado false
}

void pizzeria_destroy() {
  //garcons
  sem_destroy(&sGarcons);

  //alocacao de mesas
  sem_destroy(&sMesas);
  sem_destroy(&sLockMesas);

  //pegador de fatias, dos clientes
  pthread_mutex_destroy(&pegaFatia);

  //pedidos
  queue_destroy(&smartDeck);
  //foi deixado para destruir semaforos depois dos pizzaiolos que os usam

  //forno
  sem_destroy(&sForno);
  //foi deixado para destruir mutex e mais coisas que o pizzaiolo usa depois

  //espaco ao lado da smart deck
  pthread_mutex_destroy(&espacoParaPizza);

  //pizzaiolos
  pthread_mutex_destroy(&pahDePizza);
  for (int i = 0; i < tamanhoArrayPizzaiolos; i++) {
    pthread_join(pizzaiolos[i], NULL);
    pthread_mutex_destroy(&esperaTuaPizzaAssar[i]);
  }
  sem_destroy(&sConsomePedido);
  sem_destroy(&sProduzPedido);
  free(pizzaiolos);
  free(pizzaDoPizzaiolo);
  free(esperaTuaPizzaAssar);
  free(indicesPizzaiolo);
}

void pizza_assada(pizza_t* pizza) {
  for (int i = 0; i < tamanhoArrayPizzaiolos; i++){
    if (pizzaDoPizzaiolo[i] == pizza){
      pthread_mutex_unlock(&esperaTuaPizzaAssar[i]);
      return;
    }
  }
}

//funcao que calcula o numero de mesas necessárias para um grupo
int numMesas(int tam_grupo){
  int numeroDeMesas = 0;
  int tam = tam_grupo;
  while (tam > 0){
    numeroDeMesas++;
    tam-=4;
  }
  return numeroDeMesas;
}

/*Logica de escolher mesas
//
//acho que o canale eh um grupo de clientes pegar mesas de cada vez,
//dae ele pega todas as mesas que precisa e entao destrava pros outros
*/
int pegar_mesas(int tam_grupo) {
  //printf("numeroDeMesas: %d; tam_grupo: %d\n", numeroDeMesas, tam_grupo); ok!
  if (pizzariaAberta) {
    int numeroDeMesas = numMesas(tam_grupo);
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

  printf("garcom_tchau. tam_grupo: %d\n", tam_grupo);
  for(int i = 0; i<numMesas(tam_grupo); i++){
    sem_post(&sMesas); // Libera as mesas quando sinaliza que o grupo vai embora
  }
  sem_post(&sGarcons);
}

void garcom_chamar() {
  sem_wait(&sGarcons); //se pah essa função eh só isso msm
}

//pedidos
//smart deck usa logica do buffer circular (produtor e consumidor)
void fazer_pedido(pedido_t* pedido) {
  sem_wait(&sProduzPedido);
  queue_push_back(&smartDeck, (void*) pedido);
  sem_post(&sConsomePedido);
}

//estah usando um pegador para a pizzaria toda!!!
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
