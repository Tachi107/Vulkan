// -I/usr/lib/llvm-10/include/c++/v1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <stdexcept>

static constexpr int32_t windowWidth = 1280;
static constexpr int32_t windowHeight = 720;

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

VkResult createDebugUtilsMessengerEXT(
	vk::Instance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto createDebugMessengerFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (createDebugMessengerFunc) {
		return createDebugMessengerFunc(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroydebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto destroyDebugMessengerFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (destroyDebugMessengerFunc) {
		destroyDebugMessengerFunc(instance, debugMessenger, pAllocator);
	}
}

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;
	vk::Instance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

private:
	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
	}

	void createInstance() {
		constexpr vk::ApplicationInfo applicationInfo(
			"Vulkan", VK_MAKE_VERSION(0, 1, 0),
			"No engine", VK_MAKE_VERSION(0, 0, 0),
			VK_API_VERSION_1_2
		);

		const std::vector extensions = getRequiredExtensions();

		if constexpr (enableValidationLayers) {
			if (!checkValidationLayerSupport()) {
				throw std::runtime_error("Validation layers requested, but not available");
			}

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
			populateDebugMessengerCreateInfo(debugCreateInfo);

			vk::InstanceCreateInfo instanceCreateInfo(
				{}, &applicationInfo,
				validationLayers.size(), validationLayers.data(),
				extensions.size(), extensions.data()
			);
			instanceCreateInfo.pNext = &debugCreateInfo;
			
			if (vk::createInstance(&instanceCreateInfo, nullptr, &instance) != vk::Result::eSuccess) {
				throw std::runtime_error("Failed to create instance!");
			}
		}
		else {
			const vk::InstanceCreateInfo instanceCreateInfo(
				{}, &applicationInfo,
				0, nullptr,	// validation layers
				extensions.size(), extensions.data()
			);
			if (vk::createInstance(&instanceCreateInfo, nullptr, &instance) != vk::Result::eSuccess) {
				throw std::runtime_error("Failed to create instance!");
			}
		}
		//instance = vk::createInstance(instanceCreateInfo);

		//std::vector<vk::ExtensionProperties> extensions(vk::enumerateInstanceExtensionProperties());
		//std::cout << "Supported extensions\n";
		//for (const vk::ExtensionProperties& extension : extensions) {
		//	std::cout << '\t' << extension.extensionName << '\n';
		//}
		//
		//std::cout << "Are all GLFW extensions supported? "
		//	<< areGlfwExtensionsSupported(glfwExtensions, glfwExtensionCount, extensions) << '\n';
	}

	static bool areGlfwExtensionsSupported(const char** const glfwExtensions, const uint32_t glfwExtensionCount, const std::vector<vk::ExtensionProperties>& vulkanExtensions) {
		bool glfwExtensionFound = true;
		std::vector<uint32_t> unsupportedExtensionsIndices;
		for (uint32_t i = 0; (i < glfwExtensionCount) && glfwExtensionFound; ++i) {
			for (const vk::ExtensionProperties& vulkanExtension : vulkanExtensions) {
				if (std::strcmp(glfwExtensions[i], vulkanExtension.extensionName) == 0) {
					glfwExtensionFound = true;
					break;
				}
				glfwExtensionFound = false;
			}
			if (!glfwExtensionFound) {
				unsupportedExtensionsIndices.push_back(i);
			}
		}
		if (!glfwExtensionFound) {
			std::cout << "Usupported extensions: ";
			for (size_t i = 0; i < unsupportedExtensionsIndices.size() - 1; ++i) {
				std::cout << glfwExtensions[unsupportedExtensionsIndices[i]] << ", ";
			}
			std::cout << glfwExtensions[unsupportedExtensionsIndices[unsupportedExtensionsIndices.size() - 1]] << '\n';
		
		}
		return glfwExtensionFound;
	}

	[[nodiscard]] static bool checkValidationLayerSupport() {
		std::vector availableLayers = vk::enumerateInstanceLayerProperties();
		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const vk::LayerProperties& layerProperties : availableLayers) {
				if (std::strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		if constexpr (enableValidationLayers) {
			destroydebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		instance.destroy();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	[[nodiscard]] std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if constexpr (enableValidationLayers) {
			//if (!checkValidationLayerSupport()) {
			//	throw std::runtime_error("Validation layers requested, but not available");
			//}
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
		) {
		std::cerr << "Validation layer: " << pCallbackData->pMessage << '\n';
		return VK_FALSE;
	}

	void setupDebugMessenger() {
		if constexpr (!enableValidationLayers) {
			return;
		}
		else {
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
			populateDebugMessengerCreateInfo(debugCreateInfo);
			

			if (createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("Failed to set up debug messenger");
			}
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) {
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr;
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
