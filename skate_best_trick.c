#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h> // Necessário para O_CREAT

// Cores para o terminal
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"

#define TAMANHO_FILA 5   // Tamanho do Buffer
#define NUM_SKATISTAS 10 // Quantidade de manobras a serem geradas

// O "Produto"
typedef struct
{
  int id;
  char *manobra;
} Skatista;

// O Buffer (Recurso Compartilhado)
Skatista fila_best_trick[TAMANHO_FILA];
int indice_entrada = 0;
int indice_saida = 0;
int count_visual = 0; // Apenas para efeito visual da fila

// Controles de Concorrência (Ponteiros para usar sem_open no macOS)
sem_t *sem_vagas;             // Conta quantos espaços vazios restam
sem_t *sem_ocupados;          // Conta quantos skatistas estão na fila
pthread_mutex_t mutex_acesso; // O "segurança" da zona crítica

// Função auxiliar para desenhar a fila
void desenhar_fila()
{
  printf("\n                                                                        | Fila: [ ");
  for (int i = 0; i < TAMANHO_FILA; i++)
  {
    if (i < count_visual)
      printf("🛹 ");
    else
      printf(".  ");
  }
  printf("] (%d/%d)\n", count_visual, TAMANHO_FILA);
}

// 3. O Código do Produtor (Skatista)
void *produtor_skatista(void *arg)
{
  sleep(2); // Simula os skatistas chegando no evento (dá tempo do Juiz aparecer)
  char *manobras[] = {"Handplant (Invert)", "McTwist", "Method Air", "Kickflip Indy", "Backside Mute 900"};

  for (int i = 0; i < NUM_SKATISTAS; i++)
  {
    // 1. Cria o dado (Skatista prepara a manobra)
    Skatista s;
    s.id = i + 1;
    s.manobra = manobras[rand() % 5];

    sleep((rand() % 6) + 2); // Tempo aleatório entre 2 e 7 segundos para chegar

    // 2. Tenta entrar na fila (Decrementar semáforo de vagas)
    sem_wait(sem_vagas);

    // 3. ZONA CRÍTICA (Entrando na fila)
    pthread_mutex_lock(&mutex_acesso);

    fila_best_trick[indice_entrada] = s;
    count_visual++; // Incrementa visualização

    printf(ANSI_COLOR_GREEN ANSI_BOLD "[+] PRODUTOR:   Skatista %02d entrou na fila!" ANSI_COLOR_RESET " Manobra: %s", s.id, s.manobra);
    desenhar_fila();

    // Lógica circular do buffer
    indice_entrada = (indice_entrada + 1) % TAMANHO_FILA;

    pthread_mutex_unlock(&mutex_acesso);
    // FIM ZONA CRÍTICA    // 4. Avisa que tem gente pronta (Incrementa semáforo de ocupados)
    sem_post(sem_ocupados);
  }
  pthread_exit(NULL);
}

// 4. O Código do Consumidor (Obstáculo/Juiz)
void *consumidor_obstaculo(void *arg)
{
  printf(ANSI_COLOR_YELLOW ANSI_BOLD "[-] CONSUMIDOR: O Juiz está posicionado e aguardando...\n" ANSI_COLOR_RESET);
  for (int i = 0; i < NUM_SKATISTAS; i++)
  {
    Skatista s;

    // 1. Espera ter alguém na fila (Wait no semáforo de ocupados)
    sem_wait(sem_ocupados);

    // 2. ZONA CRÍTICA (Saindo da fila para dropar)
    pthread_mutex_lock(&mutex_acesso);

    s = fila_best_trick[indice_saida];
    count_visual--; // Decrementa visualização

    printf(ANSI_COLOR_YELLOW ANSI_BOLD "[-] CONSUMIDOR: Juiz liberou %02d!" ANSI_COLOR_RESET " Manobra: %s", s.id, s.manobra);
    desenhar_fila();

    // Lógica circular
    indice_saida = (indice_saida + 1) % TAMANHO_FILA;

    pthread_mutex_unlock(&mutex_acesso);
    // FIM ZONA CRÍTICA

    // 4. Processamento (A manobra acontecendo)
    printf(ANSI_COLOR_CYAN "    >>> [DROP] Skatista %02d dropou o half e mandou %s...\n" ANSI_COLOR_RESET, s.id, s.manobra);

    // 3. Libera a vaga na fila (Signal no semáforo de vagas)
    // Movido para cá para evitar que o Produtor imprima no meio da fala do Juiz
    sem_post(sem_vagas);

    sleep(8); // Simula o tempo da manobra (Aumentado para 8s)
    printf(ANSI_COLOR_MAGENTA "    >>> [SUCESSO] Manobra do Skatista %02d finalizada!\n\n" ANSI_COLOR_RESET, s.id);
  }
  pthread_exit(NULL);
}

// 5. Main (Configuração Inicial)
int main()
{
  pthread_t t_prod, t_cons;

  // Limpeza preventiva de semáforos anteriores (caso o programa tenha fechado errado antes)
  sem_unlink("/sem_vagas");
  sem_unlink("/sem_ocupados");

  // Inicialização para macOS (Named Semaphores)
  // sem_vagas começa com o TAMANHO_FILA (5)
  sem_vagas = sem_open("/sem_vagas", O_CREAT, 0644, TAMANHO_FILA);
  // sem_ocupados começa com 0
  sem_ocupados = sem_open("/sem_ocupados", O_CREAT, 0644, 0);

  if (sem_vagas == SEM_FAILED || sem_ocupados == SEM_FAILED)
  {
    perror("Erro ao inicializar semaforos");
    exit(1);
  }

  pthread_mutex_init(&mutex_acesso, NULL);

  printf(ANSI_COLOR_MAGENTA "\n========================================\n");
  printf("--- INICIANDO BEST TRICK SESSION ---\n");
  printf("========================================\n\n" ANSI_COLOR_RESET);

  // Criação das threads
  pthread_create(&t_prod, NULL, produtor_skatista, NULL);
  pthread_create(&t_cons, NULL, consumidor_obstaculo, NULL);

  // Espera as threads terminarem
  pthread_join(t_prod, NULL);
  pthread_join(t_cons, NULL);

  // Limpeza de memória e sistema
  sem_close(sem_vagas);
  sem_close(sem_ocupados);
  sem_unlink("/sem_vagas");
  sem_unlink("/sem_ocupados");
  pthread_mutex_destroy(&mutex_acesso);

  return 0;
}
