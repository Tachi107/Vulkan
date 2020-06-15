// -I/usr/lib/llvm-10/include/c++/v1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <optional>

static constexpr int32_t windowWidth = 1280;
static constexpr int32_t windowHeight = 720;

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers {"VK_LAYER_KHRONOS_validation"};

VkResult createDebugUtilsMessengerEXT(
	vk::Instance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto createDebugMessengerFunc {reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};
	if (createDebugMessengerFunc) {
		return createDebugMessengerFunc(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroydebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto destroyDebugMessengerFunc {reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"))};
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
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;	// Automatically cleaned up when destroying device

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
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void createInstance() {
		constexpr vk::ApplicationInfo applicationInfo(
			"Vulkan", VK_MAKE_VERSION(0, 1, 0),
			"No engine", VK_MAKE_VERSION(0, 0, 0),
			VK_API_VERSION_1_1
		);

		const std::vector extensions {getRequiredExtensions()};

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
	}

	static bool areGlfwExtensionsSupported(const char** const glfwExtensions, const uint32_t glfwExtensionCount, const std::vector<vk::ExtensionProperties>& vulkanExtensions) {
		bool glfwExtensionFound {true};
		std::vector<uint32_t> unsupportedExtensionsIndices;
		for (uint32_t i {0}; (i < glfwExtensionCount) && glfwExtensionFound; ++i) {
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
			for (size_t i {0}; i < unsupportedExtensionsIndices.size() - 1; ++i) {
				std::cout << glfwExtensions[unsupportedExtensionsIndices[i]] << ", ";
			}
			std::cout << glfwExtensions[unsupportedExtensionsIndices[unsupportedExtensionsIndices.size() - 1]] << '\n';
		
		}
		return glfwExtensionFound;
	}

	[[nodiscard]] static bool checkValidationLayerSupport() {
		std::vector availableLayers {vk::enumerateInstanceLayerProperties()};
		for (const char* const layerName : validationLayers) {
			bool layerFound {false};
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
		device.destroy();
		instance.destroy();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	[[nodiscard]] static std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount;
		const char** const glfwExtensions {glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};
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
		const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		const VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* const pCallbackData,
		void* const pUserData
		) {
		std::cerr << "Validation layer: " << pCallbackData->pMessage << '\n';
		return VK_FALSE;
	}

	void setupDebugMessenger() {
		if constexpr (!enableValidationLayers) {
			return;
		}
		else {
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};	// to set flags to 0
			populateDebugMessengerCreateInfo(debugCreateInfo);

			if (createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("Failed to set up debug messenger");
			}
		}
	}

	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) {
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr;
	}

	void pickPhysicalDevice() {
		if (instance.enumeratePhysicalDevices().empty()) {
			throw std::runtime_error("Falied to find GPUs with Vulkan support");
		}
		else {
			const std::vector physicalDevices {instance.enumeratePhysicalDevices()};
			for (const auto& physicalDevice : physicalDevices) {
				if (isPhysicalDeviceSuitable(physicalDevice)) {
					this->physicalDevice = physicalDevice;
					std::cout << "Suitable device\n";
				}
			}
			if (!physicalDevice) {
				throw std::runtime_error("Failed to find a suitable GPU");
			}
			//vk::PhysicalDeviceProperties physicalDeviceProperties {physicalDevice.getProperties()};
			//vk::PhysicalDeviceFeatures physicalDeviceFeatures {physicalDevice.getFeatures()};
		}
	}

	static bool isPhysicalDeviceSuitable(const vk::PhysicalDevice physicalDevice) {
		return findQueueFamilies(physicalDevice).graphicsFamily.has_value();
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
	};

	static QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice physicalDevice) {
		QueueFamilyIndices indices;
		const std::vector queueFamilyProperties {physicalDevice.getQueueFamilyProperties()};
		for (size_t i {0}; i < queueFamilyProperties.size(); ++i) {
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}
		}
		return indices;
	}

	void createLogicalDevice() {
		const QueueFamilyIndices indices {findQueueFamilies(physicalDevice)};
		float queuePriority {1.0F};
		const vk::DeviceQueueCreateInfo queueCreateInfo({}, indices.graphicsFamily.value(), 1, &queuePriority);
		constexpr vk::PhysicalDeviceFeatures physicalDeviceFeatures;
		vk::DeviceCreateInfo deviceCreateInfo({}, 1, &queueCreateInfo);
		if constexpr (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = validationLayers.size();
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		device = physicalDevice.createDevice(deviceCreateInfo);
		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
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
