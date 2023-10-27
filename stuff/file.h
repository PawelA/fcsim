struct file_buf {
	char *ptr;
	int len;
};

int read_file(struct file_buf *buf, const char *filename);
