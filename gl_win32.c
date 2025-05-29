#include <windows.h>
#include <wingdi.h>

void *win32_load(const char *str)
{
	HMODULE module;
	void *res;
       
	res = wglGetProcAddress(str);		
	if (res)
		return res;

	module = LoadLibraryA("opengl32.dll");

	return GetProcAddress(module, str);
}
