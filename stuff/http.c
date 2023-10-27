#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http.h"

#define SEP_SP   " "
#define SEP_LF   "\n"
#define SEP_CRLF "\r\n"
#define HTTP_1_1 "HTTP/1.1"

enum {
	ST_NONE,
	ST_METHOD,
	ST_PATH,
	ST_HTTP_VER_REQ,
	ST_HTTP_VER_RES,
	ST_STATUS_CODE,
	ST_REASON_PHRASE,
	ST_HEADER,
	ST_CHUNK_LEN,
	ST_BODY,
	ST_END,
};

enum {
	ENC_NONE,
	ENC_CHUNK,
	ENC_LEN,
};

struct http_line {
	int num;
	char *buf;
	int len;
};

static char *method_name[] = {
	"OPTIONS",
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"CONNECT",
	NULL,
};

static char *find_hdr_val(char *hdr, char *lname) {
	while (*lname) {
		if (tolower(*hdr) != *lname)
			return NULL;
		hdr++;
		lname++;
	}
	if (*hdr != ':')
		return NULL;
	do {
		hdr++;
	} while (*hdr == ' ');

	return hdr;
}

static void handle_hdr(struct http *http) {
	char *val;

	if ((val = find_hdr_val(http->buf, "transfer-encoding")))
		http->enc = ENC_CHUNK;

	if ((val = find_hdr_val(http->buf, "content-length"))) {
		http->enc = ENC_LEN;
		http->body_len = atoi(val);
	}
}

static int write_str(struct http *http, char *buf, int len, char *str) {
	int bytes;

	str += http->index;
	for (bytes = 0; bytes < len && str[bytes]; bytes++)
		buf[bytes] = str[bytes];

	if (str[bytes]) {
		http->index += bytes;
	}
	else {
		switch (http->state) {
		case ST_METHOD:
			http->state = ST_PATH;
			http->sep = SEP_SP;
			break;
		case ST_PATH:
			http->state = ST_HTTP_VER_REQ;
			http->sep = SEP_SP;
			break;
		case ST_HTTP_VER_REQ:
		case ST_REASON_PHRASE:
			http->state = ST_HEADER;
			http->sep = SEP_CRLF;
			break;
		case ST_HTTP_VER_RES:
			http->state = ST_STATUS_CODE;
			http->sep = SEP_SP;
			break;
		}
		http->index = 0;
	}

	return bytes;
}

static int write_num(struct http *http, char *buf, int len, int num, int base) {
	int bytes = 0;
	int p = 1;
	int i;

	while (p < num + 1)
		p *= base;

	for (i = 0; i < http->index; i++)
		p /= base;

	while (len > 0 && p > 1) {
		p /= base;
		i = (num / p) % base;
		*buf = (i < 10 ? '0' : 'a' - 10) + i;
		buf++;
		len--;
		bytes++;
	}
	http->index += bytes;

	if (p == 1) {
		switch (http->state) {
			case ST_STATUS_CODE:
				http->state = ST_REASON_PHRASE;
				http->sep = SEP_SP;
				break;
			case ST_CHUNK_LEN:
				http->state = ST_BODY;
				http->sep = SEP_CRLF;
				break;
		}
		http->index = 0;
	}

	return bytes;
}

static int write_hdr(struct http *http, char *buf, int len) {
	int bytes;
	int res;

	if (http->index == http->data_len) {
		res = http->hdr_cb ? http->hdr_cb(http->userdata, http->buf, http->buf_len) : 0;
		if (res < 0)
			return res;

		http->index = 0;
		http->data_len = res;

		if (res == 0) {
			http->state = ST_BODY;
			http->sep = SEP_CRLF;
			return 0;
		}
	}

	bytes = http->data_len - http->index;
	bytes = bytes < len ? bytes : len;
	memcpy(buf, http->buf + http->index, bytes);
	http->index += bytes;

	if (http->index == http->data_len) {
		http->sep = SEP_CRLF;
		handle_hdr(http);
	}
	return bytes;
}

static int write_body(struct http *http, char *buf, int len) {
	int bytes;
	int res;

	if (http->index == http->data_len) {
		res = http->body_cb ? http->body_cb(http->userdata, http->buf, http->buf_len) : 0;
		if (res < 0)
			return res;

		http->index = 0;
		http->data_len = res;

		if (res == 0) {
			if (http->enc == ENC_CHUNK)
				http->sep = "0" SEP_CRLF SEP_CRLF;
			http->state = ST_END;
			return 0;
		}

		if (http->enc == ENC_CHUNK) {
			http->state = ST_CHUNK_LEN;
			return 0;
		}
	}

	bytes = http->data_len - http->index;
	bytes = bytes < len ? bytes : len;
	memcpy(buf, http->buf + http->index, bytes);
	http->index += bytes;

	if (http->index == http->data_len && http->enc == ENC_CHUNK)
		http->sep = SEP_CRLF;

	return bytes;
}

static int write_state(struct http *http, struct http_line *line,
		char *buf, int len) {
	switch (http->state) {
		case ST_METHOD:
			return write_str(http, buf, len, method_name[line->num]);
		case ST_PATH:
		case ST_REASON_PHRASE:
			return write_str(http, buf, len, line->buf);
		case ST_HTTP_VER_REQ:
		case ST_HTTP_VER_RES:
			return write_str(http, buf, len, HTTP_1_1);
		case ST_STATUS_CODE:
			return write_num(http, buf, len, line->num, 10);
		case ST_CHUNK_LEN:
			return write_num(http, buf, len, http->data_len, 16);
		case ST_HEADER:
			return write_hdr(http, buf, len);
		case ST_BODY:
			return write_body(http, buf, len);
	}
	return -1;
}

static int http_write(struct http *http, struct http_line *line,
		char *buf, int len) {
	char *buf_base;
	int res;

	buf_base = buf;
	while (len > 0) {
		if (http->sep) {
			while (*http->sep && len > 0) {
				*buf = *http->sep;
				buf++;
				http->sep++;
				len--;
			}
			if (!*http->sep) http->sep = NULL;
		}
		else if (http->state != ST_END) {
			res = write_state(http, line, buf, len);
			if (res < 0)
				return res;
			buf += res;
			len -= res;
		}
		else break;
	}

	return buf - buf_base;
}

int http_write_req(struct http *http, struct http_req_line *line,
		char *buf, int len) {
	if (http->state == ST_NONE)
		http->state = ST_METHOD;
	return http_write(http, (struct http_line *) line, buf, len);
}

int http_write_res(struct http *http, struct http_res_line *line,
		char *buf, int len) {
	if (http->state == ST_NONE)
		http->state = ST_HTTP_VER_RES;
	return http_write(http, (struct http_line *) line, buf, len);
}

static int read_str(struct http *http, char *buf, int buf_len,
		char *str, int str_len) {
	int bytes;
	char end;
	int found_end = 0;
	
	if (http->state == ST_REASON_PHRASE || http->state == ST_HTTP_VER_REQ)
		end = '\r';
	else
		end = ' ';

	str += http->index;
	str_len -= http->index;
	for (bytes = 0; bytes < buf_len; bytes++) {
		if (bytes >= str_len)
			return -1;
		if (buf[bytes] == end) {
			found_end = 1;
			break;
		}
		str[bytes] = buf[bytes];
	}
	http->index += bytes;

	if (found_end) {
		str[bytes] = 0;
		http->index = 0;
		switch (http->state) {
			case ST_METHOD:
				http->state = ST_PATH;
				break;
			case ST_PATH:
				http->state = ST_HTTP_VER_REQ;
				break;
			case ST_HTTP_VER_REQ:
				http->state = ST_HEADER;
				http->sep = SEP_LF;
				break;
			case ST_HTTP_VER_RES:
				http->state = ST_STATUS_CODE;
				break;
			case ST_REASON_PHRASE:
				http->state = ST_HEADER;
				http->sep = SEP_LF;
		}
		bytes++;
	}

	return bytes;
}

static int read_method(struct http *http, char *buf, int len, int *method) {
	int res;
	int i;

	res = read_str(http, buf, len, http->buf, http->buf_len);
	if (res > 0 && http->state != ST_METHOD) {
		*method = -1;
		for (i = 0; method_name[i]; i++) {
			if (strcmp(http->buf, method_name[i]) == 0)
				*method = i;
		}
	}
	return res;
}

static int read_num(struct http *http, char *buf, int len, int *num, int base) {
	int bytes;
	int ended = 0;

	if (http->index == 0)
		*num = 0;

	for (bytes = 0; bytes < len; bytes++) {
		char c = buf[bytes];
		int val = base;
		if ('0' <= c && c <= '9') val = c - '0';
		else if ('a' <= c && c <= 'z') val = c - 'a' + 10;
		else if ('A' <= c && c <= 'Z') val = c - 'A' + 10;
		if (val >= base) {
			ended = 1;
			break;
		}
		*num = *num * base + val;
		http->index++;
	}

	if (ended) {
		if (http->index == 0)
			return -1;
		switch (http->state) {
			case ST_STATUS_CODE:
				http->state = ST_REASON_PHRASE;
				http->sep = SEP_SP;
				break;
			case ST_CHUNK_LEN:
				if (*num == 0) {
					http->state = ST_END;
					http->sep = SEP_CRLF SEP_CRLF;
				} else {
					http->state = ST_BODY;
					http->sep = SEP_CRLF;
				}
		}
		http->index = 0;
	}
	
	if (bytes == 0)
		http_reset(http);

	return bytes;
}
			
static int read_hdr(struct http *http, char *buf, int len) {
	int bytes;
	int res;

	for (bytes = 0; bytes < len; bytes++) {
		char c = buf[bytes];

		if (c == '\r') {
			if (http->data_len == 0) {
				http->sep = SEP_CRLF;
				if (http->enc == ENC_LEN)
					http->state = http->body_len ? ST_BODY : ST_END;
				else
					http->state = ST_CHUNK_LEN;
				return 0;
			}

			http->buf[http->data_len] = 0;
			res = http->hdr_cb ? http->hdr_cb(http->userdata, http->buf, http->data_len) : 0;
			if (res < 0)
				return res;
			handle_hdr(http);

			http->sep = SEP_CRLF;
			http->data_len = 0;
			return bytes;
		}

		http->buf[http->data_len++] = c;
		if (http->data_len == http->buf_len)
			return -1;
	}

	return bytes;
}

static int read_body(struct http *http, char *buf, int len) {
	int bytes;

	if (http->enc == ENC_NONE)
		return http->body_cb ? http->body_cb(http->userdata, buf, len) : len;

	if (http->body_len == 0) {
		if (http->enc == ENC_CHUNK)
			http->sep = SEP_CRLF;
		http->state = ST_END;
		return 0;
	}

	bytes = http->body_len - http->index;
	bytes = bytes < len ? bytes : len;
	bytes = http->body_cb ? http->body_cb(http->userdata, buf, bytes) : bytes;

	if (bytes > 0) {
		http->index += bytes;
		if (http->index == http->body_len) {
			http->index = http->body_len = 0;
			if (http->enc == ENC_CHUNK) {
				http->sep = SEP_CRLF;
				http->state = ST_CHUNK_LEN;
			}
			else http->state = ST_END;
		}
	}

	return bytes;
}

static int read_state(struct http *http, struct http_line *line,
		char *buf, int len) {
	switch (http->state) {
		case ST_METHOD:
			return read_method(http, buf, len, &line->num);
		case ST_HTTP_VER_REQ: 
		case ST_HTTP_VER_RES: 
			return read_str(http, buf, len, http->buf, http->buf_len);
		case ST_PATH: 
		case ST_REASON_PHRASE: 
			return read_str(http, buf, len, line->buf, line->len);
		case ST_STATUS_CODE: 
			return read_num(http, buf, len, &line->num, 10);
		case ST_CHUNK_LEN:
			return read_num(http, buf, len, &http->body_len, 16);
		case ST_HEADER: 
			return read_hdr(http, buf, len);
		case ST_BODY: 
			return read_body(http, buf, len);
	}
	return -1;
}

static int http_read(struct http *http, struct http_line *line,
		char *buf, int len) {
	char *buf_base = buf;
	int res;

	if (len == 0) {
		if (http->state == ST_BODY && http->enc == ENC_NONE)
			http->state = ST_END;
		return http->state == ST_END ? 0 : -1;
	}

	while (len > 0) {
		if (http->sep) {
			while (*http->sep && len > 0) {
				if (*buf != *http->sep) return -1;
				buf++;
				http->sep++;
				len--;
			}
			if (!*http->sep) http->sep = NULL;
		}
		else if (http->state != ST_END) {
			res = read_state(http, line, buf, len);
			if (res < 0)
				return res;
			buf += res;
			len -= res;
		}
		else break;
	}

	return buf - buf_base;
}

int http_read_req(struct http *http, struct http_req_line *line,
		char *buf, int len) {
	if (http->state == ST_NONE)
		http->state = ST_METHOD;
	return http_read(http, (struct http_line *) line, buf, len);
}

int http_read_res(struct http *http, struct http_res_line *line,
		char *buf, int len) {
	if (http->state == ST_NONE)
		http->state = ST_HTTP_VER_RES;
	return http_read(http, (struct http_line *) line, buf, len);
}

int http_finished(struct http *http) {
	return http->state == ST_END && (!http->sep || !*http->sep);
}

void http_reset(struct http *http) {
	http->state = 0;
	http->enc = 0;
	http->index = 0;
	http->data_len = 0;
	http->body_len = 0;
	http->sep = 0;
}
