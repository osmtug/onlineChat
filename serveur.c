/* SERVEUR. Lancer ce programme en premier (pas d'arguments). */

#include <stdio.h>                  /* fichiers d'en-tête classiques */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>             /* fichiers d'en-tête "réseau" */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT_SERVEUR 5015;          /* Numéro de port pour le serveur */
#define MAX_CLIENTS   128;           /* Nombre maximum de clients */
#define BUFFER_SIZE  1024;           /* Taille maximum des messages */

volatile int secoute;
volatile int sservice;

void handle_sigint(int sig) {
    close(secoute);

    exit(0);
}

int main(int argc, char *argv[]) {

  /* 1. On déroute les signaux */
  signal(SIGINT, handle_sigint);

  /* 2. On crée la socket d'écoute. */
  secoute = socket(AF_INET, SOCK_STREAM, 0);
  if (secoute < 0) {
      perror("error socket()");
      exit(1);
  }

  /* 3. On prépare l'adresse du serveur. */
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_SERVEUR);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* 4. On attache la socket a l'adresse du serveur. */
    if (bind(secoute, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erreur lors de l'attachement de la socket");
        close(secoute);
        exit(1);
    }

  /* 5. Enregistrement auprès du système. */
  if (listen(secoute, MAX_CLIENTS) < 0) {
      perror("error listen()");
      close(secoute);
      exit(1);
  }

  fd_set ensemble, temp;
  FD_ZERO(&ensemble);
  FD_SET(secoute,&ensemble);
  int max = secoute;
  char message[BUFFER_SIZE];
  int nbLus;

  while (1) {
    temp = ensemble;
    printf("(père) Serveur en attente de nouveaux clients ou messages.\n");
    select(max+1, &temp, NULL, NULL, NULL);

    for (int fd = 0; fd <= max; fd++) {
      if (!FD_ISSET(fd, &temp)) continue;

      /* 6. Si on a reçu une demande sur la socket d'écoute... */
      if (fd == secoute) {
        int ss = accept(secoute, NULL, NULL);
        FD_SET(ss, &ensemble);
        printf("(père) Nouveau client enregistré.\n");
        if (ss > max) max = ss;
        continue;
      }

      /* 7. Si on a reçu des données sur une socket de service... */
      if((nbLus = read(fd, message, BUFFER_SIZE)) <= 0){
        FD_CLR(fd, &ensemble);
        shutdown(fd, SHUT_RDWR);
        printf("(fils %d) Connexion au client terminée\n", fd);
        close(fd); continue;
      }
      printf("(fils %d) Reçu message : %s\n", fd, message);
      for(int client = 0; client <= max; client++){
        if(client == fd || client == secoute) continue;
        if(FD_ISSET(client, &ensemble)) write(client, message, nbLus);
      }
    }
  }

  /* 8. On termine et libère les ressources. */
  close(secoute);

  return 0;
}
