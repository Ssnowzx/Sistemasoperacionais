# Projeto: Skate Best Trick Session (Produtor-Consumidor)

Este projeto implementa o clássico problema do **Produtor-Consumidor** utilizando a temática de um campeonato de skate no half-pipe ("Best Trick"). Ele foi desenvolvido para a disciplina de Sistemas Operacionais, demonstrando o uso de **Threads**, **Semáforos** e **Mutexes** em linguagem C.

## 1. Como Compilar e Rodar

O projeto conta com um `Makefile` para facilitar a execução.

1.  **Compilar:**
    ```bash
    make
    ```
2.  **Executar:**
    ```bash
    ./skate_session
    ```
3.  **Limpar arquivos compilados:**
    ```bash
    make clean
    ```

---

## 2. Verificação dos Requisitos

O projeto atende integralmente às exigências propostas:

- "Conter pelo menos um buffer com uma zona crítica": Implementado via buffer circular `fila_best_trick` (tamanho 5). O acesso de inserção e remoção é sempre envolto por `pthread_mutex_lock(&mutex_fila)` / `pthread_mutex_unlock(&mutex_fila)`, garantindo exclusão mútua e caracterizando a Zona Crítica.
- "Pelo menos um produtor e consumidor": A função `produtor_skatista` (thread produtora) insere skatistas com manobras na fila; a função `consumidor_obstaculo` (thread consumidora) retira e processa cada entrada. A coordenação é feita pelos semáforos `sem_vagas` (controle de espaço livre) e `sem_ocupados` (itens disponíveis).
- "A temática será única por grupo": Tema escolhido: **Skate Best Trick no Half-Pipe**. Mapeamento: Produtor = Skatista que chega com uma manobra; Consumidor = Juiz/Obstáculo que avalia e libera a próxima vaga. A fila representa a ordem de espera no topo do half.

---

## 3. Funcionamento dos Processos (Threads)

O código utiliza a biblioteca `pthread` para criar threads que rodam concorrentemente.

### A. O Produtor (O Skatista)

- **Função:** `produtor_skatista`
- **Papel:** Gera um dado (escolhe uma manobra) e tenta inseri-lo na fila de espera.
- **Fluxo de Execução:**
  1.  **Preparação:** O skatista escolhe uma manobra aleatória e aguarda um tempo aleatório (2 a 7s) para chegar.
  2.  **Verificação de Vaga (Semáforo `sem_vagas`):**
      - Chama `sem_wait(sem_vagas)`.
      - _Se houver vaga (> 0):_ Decrementa o contador e entra.
      - _Se NÃO houver vaga (0):_ A thread **bloqueia (dorme)** até que uma vaga seja liberada pelo Consumidor.
  3.  **Zona Crítica (Entrada na Fila):**
      - Bloqueia o acesso com `pthread_mutex_lock`.
      - Insere o skatista no buffer `fila_best_trick`.
      - Atualiza o índice de entrada circularmente.
      - Libera o acesso com `pthread_mutex_unlock`.
  4.  **Notificação:** Chama `sem_post(sem_ocupados)` para sinalizar que há um novo item na fila.

### B. O Consumidor (O Obstáculo/Juiz)

- **Função:** `consumidor_obstaculo`
- **Papel:** Retira o dado da fila e processa (executa/avalia a manobra).
- **Fluxo de Execução:**
  1.  **Verificação de Disponibilidade (Semáforo `sem_ocupados`):**
      - Chama `sem_wait(sem_ocupados)`.
      - _Se houver itens (> 0):_ Decrementa o contador e prossegue.
      - _Se a fila estiver vazia (0):_ A thread **bloqueia (dorme)** aguardando a chegada de um skatista.
  2.  **Zona Crítica (Saída da Fila):**
      - Bloqueia o acesso com `pthread_mutex_lock`.
      - Retira o skatista do buffer `fila_best_trick`.
      - Atualiza o índice de saída circularmente.
      - Libera o acesso com `pthread_mutex_unlock`.
  3.  **Processamento:** Simula a execução da manobra com `sleep(8)` (tempo aumentado para visualização).
  4.  **Liberação de Vaga:** Chama `sem_post(sem_vagas)` para sinalizar que um espaço no buffer foi liberado.

### C. Detalhes Técnicos Importantes

- **Compatibilidade macOS:** O código utiliza `sem_open` (Named Semaphores) em vez de `sem_init` para garantir compatibilidade total com macOS.
- **Visualização:** O terminal exibe cores ANSI e uma representação visual da fila (`| Fila: [ 🛹 . . ]`) em tempo real.
- **Sincronização:** Os tempos de `sleep` foram ajustados para permitir uma apresentação clara do funcionamento da fila enchendo e esvaziando.

---

## 4. Resumo da Analogia

O sistema funciona como uma **porta giratória controlada** no topo do half-pipe:

1.  **Skatista chega:** Pega uma senha de entrada (Semáforo de Vagas).
2.  **Entra na área restrita (Mutex):** Coloca o skate na fila e sai.
3.  **Avisa o Juiz:** Sinaliza que está pronto (Semáforo de Ocupados).
4.  **Juiz atua:** Vê o sinal, entra na área restrita (Mutex), pega o skatista da vez e libera o drop no half.
5.  **Ciclo reinicia:** O Juiz avisa que liberou um espaço na fila (Semáforo de Vagas), permitindo que um novo skatista entre.
