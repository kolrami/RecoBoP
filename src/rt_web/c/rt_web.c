#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define HEADER_COUNT_MAX 2048
#define INDEX_COUNT_MAX  20480
#define BUF_COUNT_MAX    2048

THREAD_ENTRY() {
	int server_sock, client_sock;
	struct sockaddr_in server_addr;

	char header[HEADER_COUNT_MAX];
	ssize_t header_count;

	char buf[BUF_COUNT_MAX];
	ssize_t buf_count;

	char index[INDEX_COUNT_MAX];
	ssize_t index_count;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(12345);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0) {
		panic("Failed to open socket");
	}

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in))) {
		panic("Failed to bind socket");

		close(server_sock);
	}

	listen(server_sock, 1);


	while (1) {
		FILE *fp = fopen("d3.html", "r");
		if (!fp) {
			panic("Failed to open source");
		}
		index_count = fread(index, sizeof(char), INDEX_COUNT_MAX, fp);
		index[index_count] = '\0';
		fclose(fp);

		client_sock = accept(server_sock, 0, 0);
		if (client_sock < 0) {
			panic("Failed to accept socket");

			close(server_sock);
		}

		header_count = recv(client_sock, header, HEADER_COUNT_MAX, 0);
		header[header_count] = '\0';

		if (!strncmp(header, "GET / ", 6)) {
			send(client_sock, "HTTP/1.1 200 OK\n\n", 17, 0);
			send(client_sock, index, index_count, 0);
		} else if (!strncmp(header, "GET /saw_vsense ", 10)) {
			send(client_sock, "HTTP/1.1 200 OK\n\n", 17, 0);
			buf_count = snprintf(buf, 1024, "%d", rand() % 256);
			send(client_sock, buf, buf_count, 0);
		}

		close(client_sock);
	}

	close(server_sock);
}