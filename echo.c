#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void handle_connection(int peer_sock) {
	char buffer[BUFFER_SIZE] = {0};
	char out[BUFFER_SIZE] = {0};
	size_t n;
	for (;;) {
		n = recv(peer_sock, buffer, BUFFER_SIZE, 0);
		if (n == 0) {
			printf("[INFO] connection closed\n");
			return;
		}
		buffer[n] = '\0';
		printf("size: %zu\n", n);
		printf("msg: %s\n", buffer);
		strcpy(out, "[echo] ");
		strcat(out, buffer);
		send(peer_sock, out, strlen(out), 0);
	}
}

int main() {
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


	printf("[INFO] echo server started\n");
	struct sockaddr_in peer_addr = {0};
	socklen_t peer_addr_size = sizeof(peer_addr);
	int peer_sock;
	for (;;) {
		peer_sock = accept(s, (struct sockaddr *)(&peer_addr), &peer_addr_size); 
		printf("[INFO] connection accepted\n");
		handle_connection(peer_sock);
	}
	close(s);
	return 0;
}
