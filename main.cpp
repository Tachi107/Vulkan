// -I/usr/lib/llvm-10/include/c++/v1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);


	uint32_t extensionCount;
	vk::enumerateInstanceExtensionProperties(static_cast<char*>(nullptr), &extensionCount, nullptr);

	std::cout << extensionCount << '\n';

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
}
