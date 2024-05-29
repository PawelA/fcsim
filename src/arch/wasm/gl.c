typedef unsigned int GLuint;
typedef int GLsizei;

GLuint glCreateBuffer(void);

void glGenBuffers(GLsizei n, GLuint *buffers)
{
	GLsizei i;

	for (i = 0; i < n; i++)
		buffers[i] = glCreateBuffer();
}
