#ifndef TEST_H
#define TEST_H

#ifdef TESTING

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "cglm/vec4.h"

int main(void)
{
	vec4 test = {1290.0f, 334.0f, 23.0f, 1.0f};
	for (int i = 0; i < 4; i++)
		printf("vec[%d]: %f\n", i, test[i]);
	return 0;
}

/*
 * int
 * main(void)
 * {
 * 	struct test {
 * 		int a;
 * 		int b;
 * 		int c;
 * 		int d;
 * 		int e;
 * 	};
 * 
 * 	struct test test = {0};
 * 	int a = 0;
 * 	char b = 0;
 * 	long c = 0;
 * 	void *d = 0;
 * 	YINFO("size int %d\n", sizeof(typeof(a)));
 * 	YINFO("size char %d\n", sizeof(typeof(b)));
 * 	YINFO("size long %d\n", sizeof(typeof(c)));
 * 	YINFO("size void* %d\n", sizeof(typeof(d)));
 * 	YINFO("size struct %d\n", sizeof(typeof(test)));
 * 	YINFO("size struct %d\n", sizeof(typeof(test)));
 * }
 */
#endif
#endif // TEST_H
