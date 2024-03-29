/* Mahesh V. Tripunitara
 * University of Waterloo
 * rcsapp-client.c -- takes as cmd line args a server ip & port.
 * After establishing a connection, reads from stdin till eof
 * and sends everything it reads to the server. sleep()'s
 * occasionally in the midst of reading & sending.
 */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rcs.h"

unsigned int getrand() {
	int f = open("/dev/urandom", O_RDONLY);
	if(f < 0) {
		perror("open(/dev/urandom)"); return 0;
	}

	unsigned int ret;
	read(f, &ret, sizeof(unsigned int));
	close(f);
	return ret;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("usage: %s <server-ip> <server-port>\n", argv[0]);
		exit(0);
	}
	int s = rcsSocket();
	struct sockaddr_in a;

	memset(&a, 0, sizeof(struct sockaddr_in));
	a.sin_family = AF_INET;
	a.sin_port = 0;
	a.sin_addr.s_addr = INADDR_ANY;

	if(rcsBind(s, (struct sockaddr_in *)(&a)) < 0) {
		perror("bind"); exit(1);
	}

	if(rcsGetSockName(s, &a) < 0) {
		fprintf(stderr, "rcsGetSockName() failed. Exiting...\n");
		exit(0);
	}

	int nread = -1;

	a.sin_family = AF_INET;
	a.sin_port = htons((uint16_t)(atoi(argv[2])));
	if(inet_aton(argv[1], &(a.sin_addr)) < 0) {
		fprintf(stderr, "inet_aton(%s) failed.\n", argv[1]);
		exit(1);
	}

	if(rcsConnect(s, (struct sockaddr_in *)(&a)) < 0) {
		perror("connect"); exit(1);
	}

    size_t len = 0;
    char *buf = NULL;
    while((nread = getline(&buf, &len, stdin)) > 0) {
        printf("%s", buf);
		if(rcsSend(s, buf, nread) < 0) {
			perror("send"); exit(1);
		}
        printf("sent!\n\n");

		/* sleep(getrand()%7); */
    }
    free(buf);

	rcsClose(s);

	return 0;
}
