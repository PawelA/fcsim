#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <byteswap.h>

#define PORT 8088
 
static int sock;

int minit(void)
{
	struct sockaddr_in addr;
	int res;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return sock;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	res = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (res < 0) {
		sock = 0;
		return res;
	}

	write(sock, "AAAAAAAAAAAAAAAAAAAAAAA", 23);

	return 0;
}

void mw(const char *name, int num_cnt, ...)
{
	va_list va;
	char who = 'c';
	int name_len = strlen(name);
	int name_len_n = htonl(name_len);
	int num_cnt_n  = htonl(num_cnt);
	int i;

	if (sock <= 0)
		return;

	write(sock, &who, 1);
	write(sock, &name_len_n, 4);
	write(sock, &num_cnt_n, 4);
	write(sock, name, name_len);

	va_start(va, num_cnt);
	for (i = 0; i < num_cnt; i++) {
		union {
			double d;
			uint64_t u;
		} num;
		num.d = va_arg(va, double);
		num.u = bswap_64(num.u);
		write(sock, &num, 8);
	}
}
