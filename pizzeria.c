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
// tamForno npizzaiolos, nmesas, ngarcons, tamdeck, ngrupos, tempo
//INE5410_INFO=1 ./program
//mini forno: INE5410_INFO=1 ./program 2 10 40 40 40 40 5
//greve de pizzaiolos: INE5410_INFO=1 ./program 4 2 40 40 40 40 5
//Inflacao moveleira: INE5410_INFO=1 ./program 10 10 10 10 40 40 5
//greve de garcons: INE5410_INFO=1 ./program 10 10 40 2 40 40 5
//escassez de fichas: INE5410_INFO=1 ./program 10 10 40 40 3 40 5

//INE5410_GOH=1

//garcons
sem_t sGarcons;

//alocacao de mesas
sem_t sAlteraMesas;
int maxMesas, mesasLivres;
pthread_mutex_t mMesas;

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
int ultimoClienteVazou = 0;//no fim vai ser mandado tipo uns pedidos sq
//quer dizer pros pizzaiolos encerrarem

void *garcomEntregaPizza(void *arg){
  pizza_t *pizza = (pizza_t *) arg;
  garcom_entregar(pizza);
  sem_post(&sGarcons);
  pthread_exit(NULL);
}

//smart deck usa logica do buffer circular (produtor e consumidor)
void *pizzaiolo(void *arg){
  int i = *((int*)arg); // identificador do pizzaiolo
  //pizzaiolo soh encerra quando a pizzaria tah fechada e nao tem mais cliente
  while (pizzariaAberta || mesasLivres!=maxMesas){ // 0 eh false
    //pega pedido da smart deck
    pedido_t *pedido;
    //printf("pizzaiolo esperando pedido\n"); fflush(NULL);
    sem_wait(&sConsomePedido);
    if (ultimoClienteVazou){
      pthread_exit(NULL);
    }
    //printf("removerei pedido\n"); fflush(NULL);
    pedido = (pedido_t*) queue_wait(&smartDeck);
    sem_post(&sProduzPedido);

    //monta pizza
    pizzaDoPizzaiolo[i] = pizzaiolo_montar_pizza(pedido);

    //printf("pizzaiolo esperando pah\n"); fflush(NULL);
    //poe pizza no forno
    //printf("pizzaiolo %d esperando forno\n", i); fflush(NULL);
    sem_wait(&sForno); // <- ocupa um espaco no forno
    //printf("pizzaiolo %d poe pizza pra assar\n",i); fflush(NULL);
    pthread_mutex_lock(&esperaTuaPizzaAssar[i]); //o unlock eh qd fica pronta
    pthread_mutex_lock(&pahDePizza);
    pizzaiolo_colocar_forno(pizzaDoPizzaiolo[i]);
    pthread_mutex_unlock(&pahDePizza);

    //tira pizza do forno
    //printf("pizzaiolo %d espera pizza assando\n",i); fflush(NULL);
    pthread_mutex_lock(&esperaTuaPizzaAssar[i]); //para ateh ficar pronta
    //printf("pizzaiolo %d espera pah\n",i); fflush(NULL);
    pthread_mutex_lock(&pahDePizza);
    pizzaiolo_retirar_forno(pizzaDoPizzaiolo[i]);
    sem_post(&sForno); // desocupa espaco do forno
    pthread_mutex_unlock(&pahDePizza);
    pthread_mutex_unlock(&esperaTuaPizzaAssar[i]); //destrava pra proxima iteracao

    //espera-se que a pizza esteja pronta aqui.
    //printf("pizzaiolo %d terminou uma pizza\n",i); fflush(NULL);

    //poe a pizza em local seguro
    //printf("pizzaiolo espera espaco seguro p pizza\n"); fflush(NULL);
    pthread_mutex_lock(&espacoParaPizza);

    //chama o garcom e lhe entrega a pizza
    //printf("pizzaiolo chamou garcom\n"); fflush(NULL);
    sem_wait(&sGarcons);
    pthread_mutex_unlock(&espacoParaPizza);
    pthread_mutex_init(&pizzaDoPizzaiolo[i]->pegador, NULL);
    //######fazer uma thread para o garcom entregar ##########
    pthread_t garcom;
    pthread_create(&garcom, NULL, garcomEntregaPizza, (void*) pizzaDoPizzaiolo[i]);
    //printf("compararei: mesasLivres: %d; maxMesas: %d\n", mesasLivres,maxMesas);
  }
  pthread_exit(NULL);
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas, int n_garcons, int tam_deck, int n_grupos) {
  printf("######LISTINHA DO QUE FALTA FAZER######\n\n");
  printf("otimizar tempo de entrega de pizza\n");
  printf("\n######FIM DA LISTINHA DO QUE FALTA FAZER######\n");
  fflush(NULL);

  //garcons
  sem_init(&sGarcons, 0,n_garcons);

  //alocacao de mesas
  pthread_mutex_init(&mMesas, NULL);
  maxMesas = n_mesas;
  pthread_mutex_lock(&mMesas);
  mesasLivres = n_mesas;
  pthread_mutex_unlock(&mMesas);
  sem_init(&sAlteraMesas,0, 1);//semaforo acorda gente quando mesas sao liberadas

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

//destruindo na ordem inversa a de construcao, mais ou menos
void pizzeria_destroy() {
  //pizzaiolos
  pthread_mutex_destroy(&pahDePizza);
  for (int i = 0; i < tamanhoArrayPizzaiolos; i++) {
    pthread_join(pizzaiolos[i], NULL);
    pthread_mutex_destroy(&esperaTuaPizzaAssar[i]);
  }
  free(pizzaiolos);

  //espaco ao lado da smart deck
  pthread_mutex_destroy(&espacoParaPizza);

  //forno
  sem_destroy(&sForno);
  free(pizzaDoPizzaiolo);
  free(esperaTuaPizzaAssar);
  free(indicesPizzaiolo);

  //pedidos
  queue_destroy(&smartDeck);
  sem_destroy(&sConsomePedido);
  sem_destroy(&sProduzPedido);

  //alocacao de mesas
  pthread_mutex_destroy(&mMesas);
  sem_destroy(&sAlteraMesas);

  //garcons
  sem_destroy(&sGarcons);
}

void pizza_assada(pizza_t* pizza) {
  for (int i = 0; i < tamanhoArrayPizzaiolos; i++){
    if (pizzaDoPizzaiolo[i] == pizza){
      pthread_mutex_unlock(&esperaTuaPizzaAssar[i]);
      return;
    }
  }
}

/*Logica de escolher mesas
//
//acho que o canale eh um grupo de clientes pegar mesas de cada vez,
//dae ele pega todas as mesas que precisa e entao destrava pros outros
*/
int pegar_mesas(int tam_grupo) {
  int restoMesas = tam_grupo % 4;
  int mesasDesejadas = tam_grupo/4;
  if (restoMesas != 0) mesasDesejadas++;
  //printf("OI!! numeroDeMesas: %d; tam_grupo: %d\n", mesasDesejadas, tam_grupo);
  pthread_mutex_lock(&mMesas);
  while (mesasDesejadas > mesasLivres){
    pthread_mutex_unlock(&mMesas);
    //inseri a seguinte linha na expectativa que ele acorde o proximo
    //sem incrementar de fato o semafro, como diz no slide que faz quando
    //há alguém já na fila do semáforo
    sem_post(&sAlteraMesas);
    if (!pizzariaAberta) return -1;
    sem_wait(&sAlteraMesas);
    pthread_mutex_lock(&mMesas);
  }
  mesasLivres -= mesasDesejadas;
  pthread_mutex_unlock(&mMesas);
  //se tiver sido acordado e a pizzaria já tiver fechado, reverter mudanças
  //e retornar -1. Isso segue a lógica de fazer o caso comum mais otimizado.
  if (!pizzariaAberta) {
    pthread_mutex_lock(&mMesas);
    mesasLivres += mesasDesejadas;
    pthread_mutex_unlock(&mMesas);
    sem_post(&sAlteraMesas); //acorda mais clientes que podem estar esperando
    sem_post(&sAlteraMesas); //já acorda dois.. pra agilizar se tiver mta gente
    return -1;
  }
  return 0;
}

void garcom_tchau(int tam_grupo) {
  int restoMesas = tam_grupo % 4;
  int mesasDesejadas = tam_grupo/4;
  if (restoMesas != 0) mesasDesejadas++;
  //printf("TCHAU. tam_grupo: %d. nPosts: %d\n", tam_grupo,numMesas(tam_grupo));
  pthread_mutex_lock(&mMesas);
  mesasLivres += mesasDesejadas;
  pthread_mutex_unlock(&mMesas);
  sem_post(&sAlteraMesas);
  sem_post(&sGarcons);

  int souUltimoGrupoNaPizzariaFechada= !pizzariaAberta&&(mesasLivres==maxMesas);

  if(souUltimoGrupoNaPizzariaFechada){
    ultimoClienteVazou = 1;
    //o seeguinte for mata todos os pizzaiolos vivos
    for(int i = 0; i < tamanhoArrayPizzaiolos; i++){
      sem_post(&sConsomePedido);
    }
  }
}

void garcom_chamar() {
  sem_wait(&sGarcons); //se pah essa função eh só isso msm
}

//pedidos
//smart deck usa logica do buffer circular (produtor e consumidor)
void fazer_pedido(pedido_t* pedido) {
  sem_wait(&sProduzPedido);
  //printf("fazendo pedido\n"); fflush(NULL);
  queue_push_back(&smartDeck, (void*) pedido);
  sem_post(&sConsomePedido);
}

//estah usando um pegador para a pizzaria toda!!!
int pizza_pegar_fatia(pizza_t* pizza) {
  //na linha 215 o helper atribui 12 a pizza->fatias
  pthread_mutex_lock(&pizza->pegador);
    if (pizza->fatias > 0){
      pizza->fatias--;
      pthread_mutex_unlock(&pizza->pegador);
      return 0;
    } else {
      pthread_mutex_unlock(&pizza->pegador);
      return -1;
    }
}
