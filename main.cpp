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
#include <set>
#include <limits>

//#ifndef NDEBUG
#if false
static uint32_t allocCount{};
static size_t totalAllocSize{};

void* operator new(std::size_t size) {
	if (size == 0) {
		size = 1;
	}
	void* p;
	while ((p = ::malloc(size)) == nullptr) {
		// If malloc fails and there is a new_handler,
		// call it to try free up memory.
		std::new_handler nh = std::get_new_handler();
		if (nh) {
			nh();
		}
		else {
			throw std::bad_alloc();
		}
	}
	++allocCount;
	totalAllocSize += size;
	std::cerr << "Allocating " << size << " bytes, " << allocCount << " total allocations, " << totalAllocSize << " bytes allocated\n";
	return p;
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
	if (size == 0) {
		size = 1;
	}
	if (static_cast<size_t>(alignment) < sizeof(void*)) {
		alignment = std::align_val_t(sizeof(void*));
	}
	void* p;
#if defined(_LIBCPP_MSVCRT_LIKE)
	while ((p = _aligned_malloc(size, static_cast<size_t>(alignment))) == nullptr)
#else
	while (::posix_memalign(&p, static_cast<size_t>(alignment), size) != 0)
#endif
	{
		// If posix_memalign fails and there is a new_handler,
		// call it to try free up memory.
		std::new_handler nh = std::get_new_handler();
		if (nh) {
			nh();
		}
		else {
#ifndef _LIBCPP_NO_EXCEPTIONS
			throw std::bad_alloc();
#else
			p = nullptr; // posix_memalign doesn't initialize 'p' on failure
			break;
#endif
		}
	}
	++allocCount;
	totalAllocSize += size;
	std::cerr << "Allocating " << size << " bytes, " << allocCount << " total allocations, " << totalAllocSize << " bytes allocated\n";
	return p;
}
#endif

#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

static const std::vector<const char*> validationLayers {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char*> requestedPhysicalDeviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkResult createDebugUtilsMessengerEXT(
	const vk::Instance instance,
	vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	vk::AllocationCallbacks* pAllocator,
	vk::DebugUtilsMessengerEXT* pDebugMessenger
) {
	auto createDebugMessengerFunc {reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"))};
	if (createDebugMessengerFunc) {
		return createDebugMessengerFunc(instance, reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(pCreateInfo), reinterpret_cast<VkAllocationCallbacks*>(pAllocator), reinterpret_cast<VkDebugUtilsMessengerEXT*>(pDebugMessenger));
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroydebugUtilsMessengerEXT(const vk::Instance instance, vk::DebugUtilsMessengerEXT debugMessenger, vk::AllocationCallbacks* pAllocator) {
	auto destroyDebugMessengerFunc {reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"))};
	if (destroyDebugMessengerFunc) {
		destroyDebugMessengerFunc(instance, debugMessenger, reinterpret_cast<VkAllocationCallbacks*>(pAllocator));
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
	static constexpr uint32_t windowWidth = 1280;
	static constexpr uint32_t windowHeight = 720;
	GLFWwindow* window;
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;				// Automatically cleaned up when destroying device
	vk::Queue presentationQueue;
	vk::SurfaceKHR surface;					// Basically the window
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;	// Here I'll store the images in the swapchain. Automatically cleaned up when destroyng the swapchain
	vk::Format swapchainImageFormat;
	vk::Extent2D swapchainExtent;

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
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		device.destroySwapchainKHR(swapchain);
		device.destroy();
		if constexpr (enableValidationLayers) {
			destroydebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		instance.destroySurfaceKHR(surface);
		instance.destroy();
		glfwDestroyWindow(window);
		glfwTerminate();
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

			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo({},
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				debugCallback
			);

			vk::InstanceCreateInfo instanceCreateInfo(
				{}, &applicationInfo,
				validationLayers.size(), validationLayers.data(),
				extensions.size(), extensions.data()
			);
			instanceCreateInfo.pNext = &debugCreateInfo;
			
			//if (vk::createInstance(&instanceCreateInfo, nullptr, &instance) != vk::Result::eSuccess) {
			//	throw std::runtime_error("Failed to create instance!");
			//}
			instance = vk::createInstance(instanceCreateInfo);
		}
		else {
			const vk::InstanceCreateInfo instanceCreateInfo(
				{}, &applicationInfo,
				0, nullptr,	// validation layers
				extensions.size(), extensions.data()
			);
			instance = vk::createInstance(instanceCreateInfo);
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

	[[nodiscard]] static std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount;
		const char** const glfwExtensions {glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};
		// TODO: Test vector on Windows
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);	// (I guess) I'm giving the vector the start and the end of the memory that it has to use to initialize itself... But how does it know how many elements it has to create and how long they should be?
		if constexpr (enableValidationLayers) {
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
		if constexpr (enableValidationLayers) {
			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo({},
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				debugCallback
			);

			if (createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("Failed to set up debug messenger");
			}
		}
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
					break;	// To pick the first suitable device
				}
			}
			if (!physicalDevice) {
				throw std::runtime_error("Failed to find a suitable GPU");
			}
			//vk::PhysicalDeviceProperties physicalDeviceProperties {physicalDevice.getProperties()};
			//vk::PhysicalDeviceFeatures physicalDeviceFeatures {physicalDevice.getFeatures()};
		}
	}

	[[nodiscard]] bool isPhysicalDeviceSuitable(const vk::PhysicalDevice physicalDevice) {
		return findQueueFamilies(physicalDevice).isComplete() &&
			   checkPhysicalDeviceExtensionSupport(physicalDevice) &&
			   querySwapchainSupport(physicalDevice).isAdequate();
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentationFamily;

		[[nodiscard]] bool isComplete() const {
			return graphicsFamily.has_value() && presentationFamily.has_value();
		}
	};

	[[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice physicalDevice) {
		const std::vector queueFamilyProperties {physicalDevice.getQueueFamilyProperties()};
		// i is the index of the queue
		for (size_t i {0}; i < queueFamilyProperties.size(); ++i) {
			QueueFamilyIndices indices;
			// If the queue at that index supports graphics, I insert that index in the graphicsFamily part of the QueueFamilyIndices
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}
			// If the queue at that index supports presentation, I insert its index in the presentationFamily part of the QueueFamilyIndices
			const vk::Bool32 presentationSupport {physicalDevice.getSurfaceSupportKHR(i, surface)};
			if (presentationSupport) {
				indices.presentationFamily = i;
			}
			// The two indices don't have to always be the same
			if (indices.isComplete()) {
				return indices;
			}
		}
		throw std::runtime_error("Failed to find the required queue families");
	}

	void createLogicalDevice() {
		const QueueFamilyIndices indices {findQueueFamilies(physicalDevice)};
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		const std::set<uint32_t> uniqueQueueFamilies {indices.graphicsFamily.value(), indices.presentationFamily.value()};
		constexpr float queuePriority {1.0F};

		for (const uint32_t queueFamily : uniqueQueueFamilies) {
			const vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures physicalDeviceFeatures;
		vk::DeviceCreateInfo deviceCreateInfo(
			{}, queueCreateInfos.size(), queueCreateInfos.data(),
			{/*EnabledLayerCount*/}, {/*EnabledLayerNames*/},
			requestedPhysicalDeviceExtensions.size(), requestedPhysicalDeviceExtensions.data(),
			&physicalDeviceFeatures
		);
		if constexpr (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = validationLayers.size();
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}

		device = physicalDevice.createDevice(deviceCreateInfo);
		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
		presentationQueue = device.getQueue(indices.presentationFamily.value(), 0);
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS) {	// I think I can safely cast, the C++ wrapper and the original C struct should have the same size
			//vk::SurfaceKHR member_surface(temp_VkSurfaceKHR); To test if the above doesn't work
			throw std::runtime_error("Failed to create window surface");
		}
	}
	
	[[nodiscard]] static bool checkPhysicalDeviceExtensionSupport(const vk::PhysicalDevice physicalDevice) {
		// I get all extensions supported by my physicalDevice
		const std::vector availableExtensions {physicalDevice.enumerateDeviceExtensionProperties()};
		// I create a set of extensions that I decide to require (the swapchain in this case).
		// I've used a set so that I can easly remove one of them by just specifying the name
		std::set<std::string_view> requiredExtensions(requestedPhysicalDeviceExtensions.begin(), requestedPhysicalDeviceExtensions.end());
		// I iterate over all my available extensions, and, one iteration over the other,
		// i remove an extension from the set of required ones if that's already supported,
		// and it is if the extension is present in the vector of available ones.
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		// If the set of required extension is empty, it means that all my required extensions
		// are supported by the physicalDevice
		return requiredExtensions.empty();
	}

	struct SwapchainDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;	// Color depth
		std::vector<vk::PresentModeKHR> presentModes;	// Conditions for swapping images to the screen

		[[nodiscard]] bool isAdequate() const {
			return !formats.empty() && !presentModes.empty();
		}
	};

	SwapchainDetails querySwapchainSupport(vk::PhysicalDevice physicalDevice) {
		return SwapchainDetails {
			physicalDevice.getSurfaceCapabilitiesKHR(surface),
			physicalDevice.getSurfaceFormatsKHR(surface),
			physicalDevice.getSurfacePresentModesKHR(surface)
		};
	}

	[[nodiscard]] static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}
		std::clog << "Unable to use sRGB format, errors may occur\n";
		return availableFormats.front();
	}

	[[nodiscard]] static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {	// Mailbox is fastsync (triple buffering, and is the best option available)
				return availablePresentMode;
			}
		}
		return vk::PresentModeKHR::eFifo;	// Guaranteed to be available, is v-sync
	}

	[[nodiscard]] static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			//vk::Extent2D actualExtent {windowWidth, windowHeight};
			//std::cerr << actualExtent.width << ", " << actualExtent.height << '\n';
			//actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			//actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			// TODO: Check if the line below works
			return vk::Extent2D(std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, windowWidth)), std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, windowHeight)));
		}
	}

	void createSwapchain() {
		const SwapchainDetails swapchainDetails {querySwapchainSupport(physicalDevice)};
		const vk::SurfaceFormatKHR surfaceFormat {chooseSwapSurfaceFormat(swapchainDetails.formats)};
		const vk::Extent2D extent {chooseSwapExtent(swapchainDetails.capabilities)};
		uint32_t imageCount {swapchainDetails.capabilities.minImageCount + 1};
		// 0 means that's no limit to the image count in the swap chain
		if (swapchainDetails.capabilities.maxImageExtent != 0 && imageCount > swapchainDetails.capabilities.maxImageCount) {
			imageCount = swapchainDetails.capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR swapchainCreateInfo({},
			surface,
			imageCount,
			surfaceFormat.format, surfaceFormat.colorSpace,
			extent,
			1,													// Number of layers per image. 1 unless 3D
			vk::ImageUsageFlagBits::eColorAttachment,			// Using as an attachment 'cause I'm going to directly render from them
			vk::SharingMode::eExclusive,						// Better performance, harder to manage
			{/*queueFamilyIndexCount*/}, {/*pQueueFamilyIndices*/},
			swapchainDetails.capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,				// Specifies if the alpha channel should be used for blending with other windows. This way I just ignore it
			chooseSwapPresentMode(swapchainDetails.presentModes),
			vk::Bool32(VK_TRUE),								// Enable clipping for best performance, but I'm unable to see the pixels obscured by a window on top
			{/*oldSwapchain*/}
		);

		QueueFamilyIndices indices {findQueueFamilies(physicalDevice)};
		// If I have two different queues they'll be able to access the image at the same time.
		// This way, I have to specify which families will be using the swapchain
		if (indices.graphicsFamily.value() != indices.presentationFamily.value()) {
			//const std::array queueFamilyIndices {indices.graphicsFamily.value(), indices.presentationFamily.value()};
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = std::array{indices.graphicsFamily.value(), indices.presentationFamily.value()}.data();
		}
		
		swapchain = device.createSwapchainKHR(swapchainCreateInfo);
		swapchainImages = device.getSwapchainImagesKHR(swapchain);
		swapchainImageFormat = surfaceFormat.format;
		swapchainExtent = extent;
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
