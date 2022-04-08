#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

// functie pentru verificarea pornirii clientului printr-o comanda potrivita
void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ); // se dezactiveaza bufferingul la afisare

    if (argc != 4) { // verificare numar de argumente
    	usage(argv[0]);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));

    int ret;

    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    DIE (ret < 0, "connect");

	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);

	// trimitere ID catre server
	sprintf(buffer, "%s\n", argv[1]);

	int s;
    s = send(sockfd, buffer, strlen(buffer)+1 , 0);
    DIE(s == -1, "send");

    // multimea de citire folosita in select()
    fd_set read_fds;

    // multimea folosita temporar
    fd_set tmp_fds;

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    // cream multimea de file descriptori pentru citire
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd, &read_fds);

    int maxfd = sockfd;

    // se dezactiveaza algoritmul Nagle
    int enable = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

    loop {
    	tmp_fds = read_fds;

    	int r = select(maxfd + 1, &tmp_fds, NULL, NULL, NULL);
    	DIE(r < 0, "select");

    	for (int i = 0; i <= maxfd; i++) {
    		if (FD_ISSET(i, &tmp_fds)) {
    			if (i == STDIN_FILENO) { // STDIN
                    bool shouldSend = 0; // flag pentru trimiterea de mesaje catre server

                    simple_message sMessage;  
                    memset(sMessage.buffer, 0, BUFLEN);
                    fgets(sMessage.buffer, BUFLEN, stdin);
                    sMessage.sf = false;

    				if (!strcmp(sMessage.buffer, "exit\n")) { // comanda exit
    					exit(0);
    				} else {
                        char copy[strlen(sMessage.buffer)];
                        strcpy(copy, sMessage.buffer); // strtok modifica variabila => lucram pe o copie
                        char* p = strtok(copy, " ");
                        if (!strcmp(p, "subscribe")) { // comanda subscribe
                            p = strtok(NULL, " ");
                            if (p != nullptr && strlen(p) <= 50) {
                                p = strtok(NULL, " ");
                                if (p != nullptr && (atoi(p) == 0 || atoi(p) == 1)) {
                                    if (strtok(NULL, " ") == NULL) { // subscribe <TOPIC> <SF>
                                        sMessage.sf = atoi(p);
                                        printf("Subscribed to topic.\n");
                                        shouldSend = true;
                                    }
                                }
                            }

                        } else if (!strcmp(p, "unsubscribe")) { // comanda unsubscribe
                            p = strtok(NULL, " ");
                            if (p != nullptr && strlen(p) <= 50) {
                                if (strtok(NULL, " ") == NULL) {  // unsubscribe <TOPIC>
                                    printf("Unsubscribed from topic.\n");
                                    shouldSend = true;
                                }

                            }
                        }
                    }
                    if (shouldSend) {
                        s = send(sockfd, (char*) &sMessage, sizeof(sMessage), 0);
                        DIE(s == -1, "send");
                    }

    			} else { // socket primire
    				memset(buffer, 0, sizeof(complex_message));

    				int n = recv(i, buffer, sizeof(complex_message), 0);
    				DIE(n < 0, "recv");

                    complex_message* cMessage = (complex_message*)buffer;

    				if (n == 0) { // s-a inchis server-ul
    					exit(0);
    				} else {
    					if (!strcmp(cMessage->message, DUPLICATE_ID)) { // deja conectat
    						exit(0);
    					} else { // afisare mesaj in formatul precizat in cerinta
		    				printf("%s:%hu - %s - %s - %s\n",
                                cMessage->ip_client_udp,
                                cMessage->port_client_udp,
                                cMessage->topic,
                                cMessage->tip_date,
                                cMessage->message);
		    			}
    				}
    			}
    		}
    	}
    }
}