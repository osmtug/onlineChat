/* CLIENT. Donner l'adresse IP et un pseudo en paramètre */
/* exemple "client 127.0.0.1 dr.ced", lancer en dernier. */

#include <stdio.h>             /* fichiers d'en-tête classiques */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include <sys/socket.h>        /* fichiers d'en-tête "réseau" */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT_SERVEUR 5015     /* Numéro de port pour le serveur */
#define BUFFER_SIZE  1024      /* Taille maximum des messages */

int main(int argc, char *argv[]) {

  /* 1. On crée la socket du client. */
  int sclient = socket(AF_INET, SOCK_STREAM, 0);
  if (sclient < 0) {
    perror("error socket()");
    exit(1);
  }

  /* 2. On prépare l'adresse du serveur. */
  if(argc <= 2){
    printf("too few argument\n");
    return 2;
  }
  char *serverIP = argv[1];
  char *pseudo = argv[2];

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_SERVEUR);
  //addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_addr.s_addr = inet_addr(serverIP);

  if (addr.sin_addr.s_addr == INADDR_NONE) {
      fprintf(stderr, "Invalid IP address\n");
      close(sclient);
      return 1;
  }



  /* 3. On demande une connexion au serveur, tant qu'on y arrive pas. */

  int cpt = 0;

  while(connect(sclient, (struct sockaddr*) &addr, sizeof(addr)) == -1) {

    printf("%d tentative de connexion \n", cpt);
    cpt++;
    if(cpt > 100){
        shutdown(sclient, SHUT_RDWR);
        close(sclient);

        return 0;
    }
  }

  /* 4. On communique. */
  fd_set ensemble, temp;

  printf("Client de chat ouvert :\n");
  char message[BUFFER_SIZE];
  FD_ZERO(&ensemble);
  FD_SET(0,&ensemble);
  FD_SET(sclient,&ensemble);

  char sendPseudo[strlen(pseudo) + 3 + 1];
  sendPseudo[0] = '\0';
  strcat(sendPseudo, pseudo);
  strcat(sendPseudo, " : ");
  char fullMessage[BUFFER_SIZE];

  int lus;
  while (1) {
      printf("%s> ", pseudo);
      fflush(stdout);

      temp = ensemble;
      select(sclient+1, &temp, NULL, NULL, NULL);

      if (FD_ISSET(0, &temp)) {
        lus = read(0, message, BUFFER_SIZE - strlen(sendPseudo));

        if (lus > 0) {
          message[lus] = '\0';
          strcpy(fullMessage, sendPseudo);
          strcat(fullMessage, message);

          write(sclient, fullMessage, strlen(fullMessage)+1);
        } else break;
      }

      if (FD_ISSET(sclient, &temp)) {
        lus = read(sclient, message, BUFFER_SIZE);

        if (lus > 0) {
        printf("\n");
        write(1, message, lus);
        printf("\n");
        } else break;

      }
  }



  /* 5. On termine et libère les ressources. */

  shutdown(sclient, SHUT_RDWR);
  close(sclient);

  return 0;
}
