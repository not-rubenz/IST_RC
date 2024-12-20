# IST_RC

Este projeto tem como objetivo o desenvolvimento do jogo RC Master Mind. Em que o jogador tem de acertar uma chave de quatro cores num tempo e número de jogadas limitado.

O jogo tem um servidor que consegue interagir com clientes recebendo e enviaando mensagens através de UDP e TCP.

Os clientes enviam comandos ao servidor que por sua vez atualiza o seu estado de jogo notificando o cliente.

## Como compilar

A compilação pode ser feita ao correr o comando *make* no terminal.

## Player

Para iniciar o player use o comando *./player* com duas flags possíveis:

- *-n GSIP*: para utilizar um IP customizado *GSIP*. IP Default: *local host*

- *-p GSport*: para utilizar um port customizado *GSport*. Porto Default: *58036*

## Server

Para iniciar o servidor use o comando *./server* com duas flags possíveis:

- *-p GSport*: para utilizar um port customizado *GSport*. Porto Default: *58036*

- *-v*: para ativar verbose

## Organização dos ficheiros

### constants.hpp

Ficheiro com definição de constantes utilizados.

### utils.cpp

Ficheiro com funções auxiliares utilizados.

### utils.hpp

Header file com definição e declaração das funções no ficheiro utils.cpp

### player.cpp

Ficheiro interface que permite a interação do utilizador com o jogo.
Envia e recebe mensagens estruturadas ao servidor obtendo os estados do jogo.
Cria ficheiros STATE onde se guarda as jogadas. Cria ficheiros TOPSCORES onde se guarda os 10 jogadores com melhor pontuação.

### player.hpp

Header file com definição e declaração das funções no ficheiro player.cpp

### server.cpp

Atualiza os estados de jogo.
Recebe e envia mensagens estruturadas ao cliente informando sobre os estados do jogo.
Guarda os jogos terminados e em curso na pastas GAMES/. Guarda a pontuação dos jogos terminados com sucesso na pasta SCORES/.

### server.hpp

Header file com definição e declaração das funções no ficheiro server.cpp

### GAMES/GAME_plid.txt

Ficheiro onde são guardados o estado de jogo em curso do player *plid*.

### GAMES/plid/date_time_mode.txt

Ficheiro onde são guardados o estado de jogo terminado do player *plid* terminado do dia *date* hora *time* e modo de terminação *mode*

### SCORES/score_plid_date_time.txt

Ficheiro onde são guardados a pontuação *score* do jogo terminado do player *plid* terminado do dia *date* hora *time* e modo de jogo e a chave do jogo.

### STATE_plid.txt

Ficheiro onde é guardado as jogadas do utilizador *plid*.

### TOPSCORE_n.txt

Ficheiro onde é guardado os 10 jogadores com melhor pontuação.

## Trabalho realizado por:

*Chenyang Ying 106535*

*Ruben Zhang Shan 106538* 