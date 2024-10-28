#ifndef GLFW_CALLBACK_H
#define GLFW_CALLBACK_H

#include "os.h"

#define GLFW_CHECK(expr)\
	do { \
		const char *pDes;\
		int errcode = (expr); \
		if (errcode != GLFW_NO_ERROR) \
		{ \
			glfwGetError(&pDes);\
			YERROR("Error[%d] %s:%d: %s", errcode, __FILE__, __LINE__,  pDes); \
			return errcode; \
		} \
	} while (0);

#include <GLFW/glfw3.h>

YMB static void _KeyCallback(
		GLFWwindow*							pWindow,
		int									key,
		YMB int								scancode,
		int									action,
		YMB int								mods);

YMB static void _MouseCallback(
		YMB GLFWwindow*						pWindow,
		YMB int								button,
		YMB int								action,
		YMB int								mods);

void _ErrorCallback(
		int									error,
		const char*							pDescription);

#endif // GLFW_CALLBACK_H
