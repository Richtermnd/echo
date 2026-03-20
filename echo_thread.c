#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define STACK_SIZE (1024*8)
#define CONNECTIONS_NUM 10
#define BUFFER_SIZE 1024

typedef struct {
	pthread_t thread_id;
	int       socket;
	int       is_free;
} connection_info;

void *handle_connection(void *arg) {
	connection_info *conn = arg;
	char buffer[BUFFER_SIZE] = {0};
	char out[BUFFER_SIZE] = {0};
	size_t n;
	for (;;) {
		n = recv(conn->socket, buffer, BUFFER_SIZE, 0);
		if (n == 0) {
			printf("[INFO] connection closed\n");
			break;
		}
		buffer[n] = '\0';
		printf("[%zu]: %zu:%s\n", conn->thread_id, n, buffer);
		strcpy(out, "[echo] ");
		strcat(out, buffer);
		send(conn->socket, out, strlen(out), 0);
	}
	close(conn->socket);
	conn->is_free = 1;
}

int main() {
	connection_info connections[CONNECTIONS_NUM] = {0};
	for (int i = 0; i < CONNECTIONS_NUM; i++) {
		connections[i].is_free = 1;
		connections[i].socket = -1;
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, STACK_SIZE);

	// create socket
    int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
		return 1;
	}

	// bind socket
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8080);
	if (bind(s, (struct sockaddr *)(&addr), sizeof(addr))) {
		fprintf(stderr, "failed to bind socket: %s\n", strerror(errno));
		return 1;
	}

	// start listening
	if(listen(s, 0) < 0) {
		fprintf(stderr, "failed to listen: %s\n", strerror(errno));
	}

	printf("[INFO] echo server started on 0.0.0.0:8080\n");
	struct sockaddr_in peer_addr = {0};
	socklen_t peer_addr_size = sizeof(peer_addr);
	int peer_sock;
	for (;;) {
		peer_sock = accept(s, (struct sockaddr *)(&peer_addr), &peer_addr_size); 
		printf("[INFO] connection accepted\n");

		int found = 0;
		for	(int i = 0; i < CONNECTIONS_NUM; i++) {
			if (connections[i].is_free) {
				found = 1;
				connections[i].is_free = 0;
				connections[i].socket = peer_sock;
				pthread_create(&connections[i].thread_id, &attr, &handle_connection, (void *)&connections[i]);
				break;
			}
		}
		if (!found) {
			printf("[INFO] max connections count reached\n");
			close(peer_sock);
		}
	}
	close(s);
	pthread_attr_destroy(&attr);
	return 0;
}
