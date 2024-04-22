#define HTTP_OPTIONS 0
#define HTTP_GET     1
#define HTTP_HEAD    2
#define HTTP_POST    3
#define HTTP_PUT     4
#define HTTP_DELETE  5
#define HTTP_TRACE   6
#define HTTP_CONNECT 7

typedef int (*http_fn)(void *, char *, int);

struct http {
	http_fn hdr_cb;
	http_fn body_cb;
	void *userdata;
	char *buf;
	int buf_len;

	int state;
	int enc;
	int index;
	int data_len;
	int body_len;
	char *sep;
};

struct http_req_line {
	int method;
	char *path;
	int len;
};

struct http_res_line {
	int status;
	char *reason;
	int len;
};

int http_write_req(struct http *http, struct http_req_line *line,
		char *buf, int len);

int http_write_res(struct http *http, struct http_res_line *line,
		char *buf, int len);

int http_read_req(struct http *http, struct http_req_line *line,
		char *buf, int len);

int http_read_res(struct http *http, struct http_res_line *line,
		char *buf, int len);

int http_finished(struct http *http);
void http_reset(struct http *http);
