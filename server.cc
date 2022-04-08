#include <algorithm>
#include <cmath>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"

// functie pentru verificarea pornirii server-ului printr-o comanda potrivita
void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

//functie pentru stergerea unui element dintr-un vector
bool eraseVector(std::vector<std::string> &v, std::string id) {
	auto it = std::find(v.begin(), v.end(), id);
	if (it != v.end()) {
		v.erase(it);
		return true;
	}
	return false;
}

// used to print an unordered_map to STDIN
void DEBUG_show_umap(std::unordered_map<int, std::string> umap) {
	if (DEBUG) {
		for (auto const &pair: umap) {
	        std::cout << "{" << pair.first << ": " << pair.second << "}\n";
	    }
	}
}

// used to print a vector of unordered_map to STDIN
void DEBUG_show_abonati(std::unordered_map<std::string, std::vector<std::string>> abonati) {
	if (DEBUG) {
		for (auto const &pair: abonati) {
			std::cout << pair.first << ": ";
	        for (auto const& it: pair.second) {
	        	std::cout << it << " ";
	        }
	        std::cout << std::endl;
	    }
		std::cout << std::endl;
	}
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ); // se dezactiveaza bufferingul la afisare

    std::unordered_map<int, std::string> umap; // dictionar socket / id
    std::unordered_map<std::string, std::vector<std::string>> abonati; // dictionar topic / id-uri

    if (argc != 2) { // verificare numar de argumente
        usage(argv[0]);
    }
    
    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds; // multimea folosita temporar

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    int portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

    struct sockaddr_in udp_addr;
    memset((char *) &udp_addr, 0, sizeof(udp_addr));

    // socket UDP
    socklen_t udpSockLen = sizeof(sockaddr); 
    int udpSock = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udpSock < 0, "socket"); 

    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(portno);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    // socket TCP
    int tcpSock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcpSock < 0, "socket");

    // se dezactiveaza algoritmul Nagle
    int enable = 1;
	setsockopt(tcpSock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    int ret; // variabila folosita pentru verificarea apelurilor de sistem

    ret = bind(udpSock, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = bind(tcpSock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

    ret = listen(tcpSock, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// cream multimea de file descriptori pentru citire
	FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(tcpSock, &read_fds);
	FD_SET(udpSock, &read_fds);

    int fdmax = tcpSock > udpSock ? tcpSock : udpSock; // valoare maxima fd din multimea read_fds

    loop {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		char buffer[BUFLEN]; // buffer pentru procesarea inputului

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcpSock) { // socketul inactiv (de listen)
					socklen_t clilen = sizeof(cli_addr);
					int newsockfd = accept(tcpSock, (struct sockaddr*) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

                    // adaugam socketul intors de accept() la multimea read_fds
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                    	fdmax = newsockfd;
                    }

                    memset(buffer, 0, BUFLEN);

					int n = recv(newsockfd, buffer, sizeof(buffer), 0); // id-ul clientului
					DIE(n < 0, "recv");

					buffer[strlen(buffer)-1] = '\0';

					bool alreadyConnected = false; // flag pentru clientii deja conectati
					for (auto x : umap) {
      					if (strcmp(x.second.c_str(), buffer) == 0) {
      						alreadyConnected = true;
      					}
					}

					if (!alreadyConnected) { // client nou
						printf("New client %s connected from %s:%d.\n",
					 		buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						umap[newsockfd] = buffer; // se adauga id-ul clientului nou in dictionar
					} else { // client deja existent
						printf("Client %s already connected.\n", buffer);

						complex_message cMessage;
						memset(cMessage.message, 0, BUFLEN);
						strcpy(cMessage.message, DUPLICATE_ID);

						// se atentioneaza clientul ca id-ul este deja conectat
						int s = send(newsockfd, (char*) &cMessage, sizeof(complex_message), 0);
						DIE(s == -1, "send");
					}
				} else if (i == STDIN_FILENO) { // STDIN
					memset(buffer, 0, BUFLEN);
    				fgets(buffer, BUFLEN, stdin);

					if (strcmp(buffer, "exit\n") == 0) {
						for (int i = 1; i <= fdmax; ++i) { // se inchid toate porturile (fara STDIN)
					        if (FD_ISSET(i, &tmp_fds)) {
					            close(i);
					        }
					    }
    					exit(0);
    				}
    				else { // comanda incorecta
    					;
    				}
				} else if (i == udpSock) { // socket client UDP
					char udpBuffer[BUFLENUDP]; // buffer pentru structura primita de la UDP
					memset(udpBuffer, 0, BUFLENUDP);

					int n = recvfrom(udpSock, udpBuffer, BUFLENUDP, 0, 
						(sockaddr*) &udp_addr, &udpSockLen);
					DIE(n < 0, "recv");

					complex_message cMessage; // structura folosita pentru a trimite date catre TCP
					strcpy(cMessage.ip_client_udp, inet_ntoa(udp_addr.sin_addr));
					cMessage.port_client_udp = ntohs(udp_addr.sin_port);

					// se creeaza topicul astfel:
					int t_char;
					for (t_char = 0; t_char < 50; t_char++) { // primii 50 de ibiti = topic
						cMessage.topic[t_char] = udpBuffer[t_char];
					}
					int tip = udpBuffer[50]; // bitul 50 = tipul datelor (intre 0 si 3 inclusiv)

					char udpData[BUFLEN]; // buffer pentru mesajul propriu-zis primit de la UDP
					memset(udpData, 0, BUFLEN);
					size_t cnt = 0;
					for (t_char = 51; t_char < BUFLENUDP; t_char++) { // restul bitilor = mesaj
						udpData[cnt++] = udpBuffer[t_char];
					}

					// parsarea datelor in format human-readable, conform tabelului din cerinta
					if (tip == 0) { // INT
						strcpy(cMessage.tip_date, "INT");

						// un uint32_t formatat conform netowrk byte order
						long long integerValue = ntohl(*(uint32_t*)(udpData + 1));
						if (udpData[0]) { // bitul de semn
							integerValue = -integerValue;
						}
						sprintf(cMessage.message, "%lld", integerValue);

					} else if (tip == 1){ // SHORT_REAL
						strcpy(cMessage.tip_date, "SHORT_REAL");

						// un uint16_t trimis in modul si inmultit cu 100
						double shortRealValue = ntohs(*(uint16_t*)(udpData));
						shortRealValue /= 100; // se imparte valoarea la 100
						sprintf(cMessage.message, "%.2f", shortRealValue);
					} else if (tip == 2) { // FLOAT
						strcpy(cMessage.tip_date, "FLOAT");

						// un uint32_t formatat conform network byte order
						double floatValue = ntohl(*(uint32_t*)(udpData + 1));
						// se *imparte* la puterea cu care trebuie inmultit modulul
						floatValue /= pow(10, udpData[5]);

						if (udpData[0]) { // bitul de semn
							floatValue = -floatValue;
						}
						sprintf(cMessage.message, "%lf", floatValue);
					} else if (tip == 3) { // STRING
						strcpy(cMessage.tip_date, "STRING");
						// se copiaza direct toti bitii
						strcpy(cMessage.message, udpData);
					}

					std::string topic(cMessage.topic);

					// se parcurge dictionarul de abonati si se trimite mesajul tuturor abonatilor
					for (std::string id : abonati[topic]) {
						for (auto const &pair: umap) {
					    	if (pair.second.compare(id) == 0) {
					    		int s = send(pair.first, (char*) &cMessage, sizeof(complex_message), 0);
								DIE(s == -1, "send");
					    	}
					    }
					}
				} else { // socket client
					memset(buffer, 0, BUFLEN);
					int n = recv(i, buffer, sizeof(simple_message), 0);
					DIE(n < 0, "recv");

					simple_message* sMessage = (simple_message*)buffer;
					// conexiunea s-a inchis
					if (n == 0) {
						if (strlen(umap[i].c_str())) {
							printf("Client %s disconnected.\n", umap[i].c_str());
							umap.erase(i); // se sterge clientul din dictionar
						}
						close(i); // se elibereaza socketul
						FD_CLR(i, &read_fds);
					} else {
						if (sMessage->buffer[0] == 's') { // subscribe <TOPIC> <SF>
							char copy[strlen(sMessage->buffer)];
							strcpy(copy, sMessage->buffer);
							char* p = strtok(copy, " ");
							p = strtok(NULL, " ");
							if (p != nullptr && strlen(p) <= 50) {
								std::string topic(p);
								abonati[topic].push_back(umap[i]); // se aboneaza clientul la topic
							}
						} else if (sMessage->buffer[0] == 'u') { // unsubscribe <TOPIC>
							char copy[strlen(sMessage->buffer)];
							strcpy(copy, sMessage->buffer);
							char* p = strtok(copy, " ");
							p = strtok(NULL, " ");
							if (p != nullptr && strlen(p) <= 50) {
								p[strlen(p)-1] = '\0';
								std::string topic(p);
								if (eraseVector(abonati[topic], umap[i])){
									;// se dezaboneaza clientul de la topic
								}
							}
						}
					}
				}
			}
		}
    }

    return 0;
}