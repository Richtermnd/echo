#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define BUFFER_SIZE 1024
#define NUM_CONNECTIONS (10+1)
#define ADDR_fmt "%zu.%zu.%zu.%zu:%zu"
#define ADDR_arg(addr) ((addr).sin_addr.s_addr >> 0) & 0xff, ((addr).sin_addr.s_addr >> 8) & 0xff, ((addr).sin_addr.s_addr >> 16) & 0xff, ((addr).sin_addr.s_addr >> 24) & 0xff, (addr).sin_port

int fds_count = 0;
struct pollfd fds[NUM_CONNECTIONS];
int timeout_msecs = 500;

// first buffer and out will be wasted cuz fds[0] - server socket, but not a big deal tbh
char buffers[NUM_CONNECTIONS][BUFFER_SIZE] = {0};
char outs[NUM_CONNECTIONS][BUFFER_SIZE] = {0};
size_t n;

int main() {
	// create socket
    int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
		return 1;
	}
	fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
	fds[0].fd = s;
	fds[0].events = POLLIN;
	fds_count++;

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
		return 1;
	}

	printf("[INFO] echo server started\n");
	struct sockaddr_in peer_addr = {0};
	socklen_t peer_addr_size = sizeof(peer_addr);
	int peer_sock;
	for (;;) {
		int ret = poll(fds, fds_count, -1);
		if (ret < 0) {
			fprintf(stderr, "failed to poll: %s\n", strerror(errno));
			return 1;
		}
		// printf("[DEBUG] succesfull poll\n");
		// printf("[DEBUG] fds_count = %d\n", fds_count);

		for (int i = 0; i < fds_count; i++) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == s) { // `server` socket ready to accept connection
					// printf("[DEBUG] expecting new connection\n");
					peer_sock = accept(s, (struct sockaddr *)(&peer_addr), &peer_addr_size); 
					fds[fds_count].fd = peer_sock;
					fds[fds_count].events = POLLIN | POLLOUT;
					fds_count++;
					printf("[INFO] connection accepted "ADDR_fmt"\n", ADDR_arg(peer_addr));
					// printf("[DEBUG] fds_count = %d\n", fds_count);
				} else { // client socket send data
					// printf("[DEBUG] receiving data from %d\n", i);
					n = recv(fds[i].fd, buffers[i], BUFFER_SIZE, 0);
					if (n == 0) {
						printf("[INFO] connection %d closed\n", i);
						close(fds[i].fd);
						fds[i] = fds[fds_count - 1];
						fds_count--;
						i--;
						continue;
					}
					buffers[i][n] = '\0';
					printf("msg: %zu:%s\n", n, buffers[i]);
					// printf("[DEBUG] sending data to %d\n", i);
					strcpy(outs[i], "[echo] ");
					strcat(outs[i], buffers[i]);
					send(fds[i].fd, outs[i], strlen(outs[i]), 0);
				}
			} 
			if (fds[i].revents & POLLOUT) {
			}
		}

	}
	close(s);
	return 0;
}
