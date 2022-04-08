#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLENUDP		1553		// dimensiunea primita de la UDP
#define BUFLEN			1501		// dimensiunea maxima a calupului de date
#define MAX_CLIENTS		2147483647	// numarul maxim de clienti in asteptare = INT_MAX
#define DUPLICATE_ID 	"42"		// cod client deja conectat
#define DEBUG 0						// flag pentru afisari aditionale

#define loop			while(1)

// functie folosita la debugging
void debug(const char& code, const char* s) {
	if (DEBUG) {
		printf("%c. %s\n", code, s);
	}
}

// structura trimisa de tcp la server
struct __attribute__((packed)) simple_message {
	char buffer[BUFLEN];
	bool sf;
};

// structura trimisa de server la tcp
struct __attribute__((packed)) complex_message {
	char ip_client_udp[16];
    uint16_t port_client_udp;
    char topic[51];
    char tip_date[16];
    char message[BUFLEN];
}; 

#endif
