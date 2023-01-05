#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <byteswap.h>

#define SRV_PORT 2137
#define BUF_SIZE 1024
#define CONTEXT  50

const char *dont_add[] = {
	"parse_check",
	"body",
	"player_block",
	"level_block",
	"step_block",
	"rod_ends_in",
	"rod_ends_out",
	"jj",
	"atan2_in",
	"atan2_out",
	"lel",
	"replace",
	"replace0",
	"replace1",
	"replace2",
	"joint",
	NULL,
};

const char *dont_check[] = {
	"parse_check",
	NULL,
};

int find_word(const char **words, const char *word)
{
	for (; *words; words++) {
		if (!strcmp(*words, word))
			return 1;
	}
	return 0;
}

enum {
	ST_NEW,
	ST_POLICY,
	ST_POLICY_DONE,
	ST_IDENT,
	ST_READY,
	ST_READING,
};

struct client {
	char ident;
	int state;
	int bytes;
	int bytes_left;
	char buf[256];
};

struct packet {
	struct packet *next;
	int size;
	char packet[];
};

struct packet_queue {
	struct packet *head;
	struct packet *tail;
	int cnt;
};

struct packet_queue the_packets;
char the_ident;
struct packet_queue the_back_packets;

char the_buf[BUF_SIZE];

char policy[] =
	"<cross-domain-policy>"
		"<allow-access-from domain=\"*\" to-ports=\"*\" />"
	"</cross-domain-policy>";

void error(char *msg)
{
	fprintf(stderr, "error: ");
	if (errno)
		perror(msg);
	else
		fprintf(stderr, "%s\n", msg);
	exit(1);
}

int start_server(void)
{
	int fd;
	int val;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error("socket");

	val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
		error("setsockopt");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(SRV_PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		error("bind");
	if (listen(fd, 5) == -1)
		error("listen");

	return fd;
}

void accept_client(struct pollfd *fds, struct client *cli)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	int fd;
	int i;

	addr_len = sizeof(addr);
	fd = accept(fds[2].fd, (struct sockaddr *)&addr, &addr_len);
	if (fd == -1)
		error("accept");

	for (i = 0; i < 2; i++) {
		if (fds[i].fd < 0) {
			fds[i].fd = fd;
			memset(&cli[i], 0, sizeof(struct client));
			return;
		}
	}
}

void push_packet(struct packet_queue *queue, struct packet *packet)
{
	if (queue->tail) {
		queue->tail->next = packet;
		queue->tail = packet;
	} else {
		queue->head = packet;
		queue->tail = packet;
	}
	queue->cnt++;
}

struct packet *pop_packet(struct packet_queue *queue)
{
	struct packet *head;

	head = queue->head;
	queue->head = head->next;
	if (!queue->head)
		queue->tail = NULL;
	head->next = NULL;
	queue->cnt--;

	return head;
}

struct packet *alloc_packet(char *buf, int len)
{
	struct packet *packet;

	packet = malloc(sizeof(struct packet) + len);
	if (!packet)
		error("malloc");

	packet->next = NULL;
	packet->size = len;
	memcpy(packet->packet, buf, len);

	return packet;
}

void alloc_and_push_packet(struct packet_queue *queue, char *buf, int len)
{
	struct packet *packet = alloc_packet(buf, len);
	push_packet(queue, packet);
}

void free_packet(struct packet *packet)
{
	free(packet);
}

void print_packet_nums(char *buf, int len)
{
	int name_size = strlen(buf) + 1;
	char *it;
	union {
		double d;
		uint64_t u;
	} num;

	for (it = buf + name_size; it + 7 < buf + len; it += 8) {
		memcpy(&num, it, 8);
		num.u = bswap_64(num.u);
		printf("%.20lg(%.016lx) ", num.d, num.u);
	}
}

void print_packet_ok(struct packet *packet)
{
	printf("\x1b[32mOK %s ", packet->packet);
	print_packet_nums(packet->packet, packet->size);
	printf("\x1b[0m\n");
}

void print_packet_wr(struct packet *packet, char id, char *buf, int len, char id2)
{
	printf("\x1b[31mWR\n");
	printf("%c: %s ", id, packet->packet);
	print_packet_nums(packet->packet, packet->size);
	printf("\n");
	printf("%c: %s ", id2, buf);
	print_packet_nums(buf, len);
	printf("\x1b[0m\n");
}

static void print_packets_ok(struct packet_queue *queue)
{
	struct packet *it;

	for (it = queue->head; it; it = it->next)
		print_packet_ok(it);
}

int cmp_packet(struct packet *packet, char *buf, int len)
{
	if (find_word(dont_check, buf))
		return strcmp(packet->packet, buf);
	else
		return packet->size != len || memcmp(packet->packet, buf, len);
}

static int the_countdown = -1;

void handle_packet(char ident, char *buf, int len)
{
	if (find_word(dont_add, buf))
		return;

	if (the_packets.head) {
		if (the_ident == ident) {
			alloc_and_push_packet(&the_packets, buf, len);
		} else {
			struct packet *p = pop_packet(&the_packets);

			if (cmp_packet(p, buf, len)) {
				if (the_countdown == -1) {
					print_packets_ok(&the_back_packets);
					the_countdown = CONTEXT;
				}
				print_packet_wr(p, the_ident, buf, len, ident);
			} else {
				if (the_countdown != -1)
					print_packet_ok(p);
			}

			push_packet(&the_back_packets, p);
			if (the_back_packets.cnt > CONTEXT) {
				struct packet *old = pop_packet(&the_back_packets);
				free_packet(old);
			}

			if (the_countdown != -1) {
				the_countdown--;
				if (the_countdown == 0)
					exit(0);
			}
		}
	} else {
		the_ident = ident;
		alloc_and_push_packet(&the_packets, buf, len);
	}
}

void handle_new(struct pollfd *fd, struct client *cli, char c)
{
	if (c == '<')
		cli->state = ST_POLICY;
	else
		cli->state = ST_IDENT;
}

void handle_policy(struct pollfd *fd, struct client *cli, char c)
{
	if (c == '>') {
		cli->bytes = 0;
		cli->bytes_left = sizeof(policy);
		cli->state = ST_POLICY_DONE;
	}
}

void handle_ident(struct pollfd *fd, struct client *cli, char c)
{
	cli->ident = c;
	cli->state = ST_READY;
}

void handle_ready(struct pollfd *fd, struct client *cli, char c)
{
	cli->bytes = 0;
	cli->bytes_left = c;
	cli->state = ST_READING;
}

void handle_reading(struct pollfd *fd, struct client *cli, char c)
{
	cli->buf[cli->bytes] = c;
	cli->bytes++;
	cli->bytes_left--;

	if (cli->bytes_left == 0) {
		handle_packet(cli->ident, cli->buf, cli->bytes);
		cli->state = ST_READY;
	}
}

void handle_policy_done(struct pollfd *fd, struct client *cli)
{
	int res;

	res = write(fd->fd, policy + cli->bytes, cli->bytes_left);
	if (res < 0)
		error("write");

	cli->bytes += res;
	cli->bytes_left -= res;

	if (cli->bytes_left == 0) {
		close(fd->fd);
		fd->fd = -1;
	}
}

void handle_client(struct pollfd *fd, struct client *cli)
{
	int nbytes = 0;
	int i;

	if (fd->revents & POLLIN) {
		nbytes = read(fd->fd, the_buf, sizeof(the_buf));
		if (nbytes <= 0) {
			fd->fd = -1;
			return;
		}
	}

	for (i = 0; i < nbytes; i++) {
		switch (cli->state) {
		case ST_NEW:
			handle_new(fd, cli, the_buf[i]);
			break;
		case ST_POLICY:
			handle_policy(fd, cli, the_buf[i]);
			break;
		case ST_POLICY_DONE:
			i = nbytes;
			break;
		case ST_IDENT:
			handle_ident(fd, cli, the_buf[i]);
			break;
		case ST_READY:
			handle_ready(fd, cli, the_buf[i]);
			break;
		case ST_READING:
			handle_reading(fd, cli, the_buf[i]);
			break;
		}
	}
	
	if (cli->state == ST_POLICY_DONE && (fd->revents & POLLOUT))
		handle_policy_done(fd, cli);
}

int main(void)
{
	struct pollfd fds[3];
	struct client cli[2];
	int res;
	int i;

	fds[0].fd = -1;
	fds[0].events = POLLIN | POLLOUT;
	fds[1].fd = -1;
	fds[1].events = POLLIN | POLLOUT;
	fds[2].fd = start_server();
	fds[2].events = POLLIN | POLLOUT;

	while (1) {
		res = poll(fds, 3, -1);
		if (res < 0)
			error("poll");
		if (fds[2].revents & POLLIN)
			accept_client(fds, cli);
		for (i = 0; i < 2; i++) {
			if (fds[i].revents)
				handle_client(&fds[i], &cli[i]);
		}
	}

	return 0;
}
