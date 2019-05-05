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

//pizzaiolos
pthread_t *pizzaiolos;
int tamanhoArrayPizzaiolos;

int pizzariaAberta = 1; //famigerado True

//smart deck usa logica do buffer circular (produtor e consumidor)
void *pizzaiolo(void *arg){
  while (pizzariaAberta){
    sem_wait(&sConsomePedido);
    sem_post(&sProduzPedido);
  }
  pthread_exit(NULL);
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("######LISTINHA DO QUE FALTA FAZER######\n\n");
  printf("implementar limite smart deck\n");
  printf("pizzaiolo:\n");
  printf("-pega pedido\n");
  printf("-monta pizza\n");
  printf("-pega a pah\n");
  printf("-poe no forno (que tem tamanho limitado)\n");
  printf("-tira pizza pronta do forno\n");
  printf("-coloca pizza em local seguro\n");
  printf("-espaco para soh uma pizza ao lado do deck\n");
  printf("garcom pega pizza e leva ate a mesa correspondente\n");
  printf("funcao pizza_assada\n");
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
  //pizzaiolos
  tamanhoArrayPizzaiolos = n_pizzaiolos;
  pizzaiolos = (pthread_t*) malloc(n_pizzaiolos*sizeof(pthread_t));
  for (int i = 0; i < n_pizzaiolos; i++) {
    pthread_create(&pizzaiolos[i], NULL, pizzaiolo, NULL);
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

  //pizzaiolos
  for (int i = 0; i < tamanhoArrayPizzaiolos; i++) {
    pthread_join(pizzaiolos[i], NULL);
  }
  sem_destroy(&sConsomePedido);
  sem_destroy(&sProduzPedido);
  free(pizzaiolos);
}

void pizza_assada(pizza_t* pizza) {
  printf("pizza_assada\n");
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
