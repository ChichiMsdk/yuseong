#ifndef TEST_H
#define TEST_H

#define TESTING

#ifdef TESTING

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

int main(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create a windowed mode window and its context
    GLFWwindow* window = glfwCreateWindow(800, 600, "PUTEPUTEPUTEPUTPEUTPEUTE", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Vulkan instance creation
    VkInstance instance;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions for GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    // Create Vulkan instance
    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();
    }

    // Cleanup Vulkan and GLFW
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

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
