#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <pthread.h>

#include <fcsim.h>
#include "timer.h"
#include "http.h"

#define BUF_LEN 3000
#define REASON_LEN 100

char http_buf[BUF_LEN];
char sock_buf[BUF_LEN];
char reason_buf[REASON_LEN];

int fc_connect(void) {
	struct addrinfo *info;
	int family;
	int addrlen;
	struct sockaddr addr;
	int fd;
	int res;

	res = getaddrinfo("fantasticcontraption.com", "80", NULL, &info);
	if (res != 0)
		return -1;

	family = info->ai_family;
	addrlen = info->ai_addrlen;
	addr = *info->ai_addr;
	freeaddrinfo(info);

	fd = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return fd;

	res = connect(fd, &addr, addrlen);
	if (res < 0)
		return res;

	return fd;
}



struct retrieve_level_writedata {
	int header_cnt;
	int body_len;
	int body_pos;
	char body[32];
};

void retrieve_level_write_init(struct retrieve_level_writedata *data, const char *design_id)
{
	int len;

	data->header_cnt = 0;
	data->body_pos = 0;
	len = snprintf(data->body, 32, "id=%s&loadDesign=1", design_id);
	data->body_len = len;
}

int retrieve_level_write_header(void *_data, char *buf, int len)
{
	struct retrieve_level_writedata *data = _data;

	switch (data->header_cnt++) {
	case 0:
		return sprintf(buf, "Host: fantasticcontraption.com");
	case 1:
		return sprintf(buf, "Content-Length: %d", data->body_len);
	case 2:
		return sprintf(buf, "Content-Type: application/x-www-form-urlencoded");
	}

	return 0;
}

int retrieve_level_write_body(void *_data, char *buf, int len)
{
	struct retrieve_level_writedata *data = _data;
	int left;

	if (data->body_pos >= data->body_len)
		return 0;

	left = data->body_len - data->body_pos;
	if (len > left)
		len = left;
	memcpy(buf, data->body + data->body_pos, len);
	data->body_pos += len;

	return len;
}



struct alloc_readdata {
	char *buf;
	int len;
};

void alloc_read_init(struct alloc_readdata *data)
{
	data->buf = NULL;
	data->len = 0;
}

int alloc_read_body(void *_data, char *buf, int len)
{
	struct alloc_readdata *data = _data;
	char *newbuf;
	int newlen;

	newlen = data->len + len;
	newbuf = realloc(data->buf, newlen);
	if (!newbuf)
		return 0;
	memcpy(newbuf + data->len, buf, len);

	data->buf = newbuf;
	data->len = newlen;

	return len;
}



char path[] = "/retrieveLevel.php";

int write_req(int fd, const char *design_id)
{
	struct retrieve_level_writedata data;
	struct http_req_line req_line;
	struct http http;
	int res;

	retrieve_level_write_init(&data, design_id);
	
	req_line.method = HTTP_POST;
	req_line.path = path;
	req_line.len = sizeof(path);

	memset(&http, 0, sizeof(http));
	http.hdr_cb  = retrieve_level_write_header;
	http.body_cb = retrieve_level_write_body;
	http.userdata = &data;
	http.buf = http_buf;
	http.buf_len = BUF_LEN;

	while (!http_finished(&http)) {
		res = http_write_req(&http, &req_line, sock_buf, BUF_LEN);
		if (res < 0)
			return res;
		res = send(fd, sock_buf, res, 0);
		if (res < 0)
			return res;
	}

	return 0;
}

int read_res(int fd, struct alloc_readdata *data)
{
	struct http_res_line res_line;
	struct http http;
	int res;

	alloc_read_init(data);

	res_line.reason = reason_buf;
	res_line.len = sizeof(reason_buf);

	memset(&http, 0, sizeof(http));
	http.body_cb = alloc_read_body;
	http.userdata = data;
	http.buf = http_buf;
	http.buf_len = BUF_LEN;

	while (!http_finished(&http)) {
		res = recv(fd, sock_buf, BUF_LEN, 0);
		if (res < 0)
			return res;
		res = http_read_res(&http, &res_line, sock_buf, res);
		if (res < 0)
			return res;
	}

	return 0;
}

struct loader {
	pthread_t thread;
	char *design_id;
	struct fcsim_level level;
	int done;
};
#include "file.h"

static void *thread_func(void *arg)
{
	struct loader *loader = arg;
	struct alloc_readdata data;
	int fd;

	/*
	fd = fc_connect();
	if (fd < 0) {
		fprintf(stderr, "couldn't connect\n");
		return NULL;
	}

	write_req(fd, loader->design_id);
	read_res(fd, &data);
	fcsim_parse_xml(data.buf, data.len, &loader->level);
	*/

	struct file_buf fb;
	read_file(&fb, "pringle.xml");
	fcsim_parse_xml(fb.ptr, fb.len, &loader->level);

	loader->done = 1;

	return NULL;
}

struct loader *loader_load(char *design_id) {
	struct loader *loader;

	loader = malloc(sizeof(*loader));
	if (!loader)
		return loader;

	loader->done = 0;
	loader->design_id = design_id;

	pthread_create(&loader->thread, NULL, thread_func, loader);

	return loader;
}

int loader_is_done(struct loader *loader) {
	return loader->done;
}

void loader_get(struct loader *loader, struct fcsim_level *level) {
	memcpy(level, &loader->level, sizeof(*level));
}

void loader_init(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}