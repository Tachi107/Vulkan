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
#include "new.hpp"
#include <fstream>

#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

static constexpr std::array validationLayers {"VK_LAYER_KHRONOS_validation"};
static constexpr std::array requestedPhysicalDeviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
	vk::Queue graphicsQueue;						// Automatically cleaned up when destroying device
	vk::Queue presentationQueue;
	vk::SurfaceKHR surface;							// Basically the window
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;			// Here I'll store the images in the swapchain. Automatically cleaned up when destroyng the swapchain
	vk::Format swapchainImageFormat;
	vk::Extent2D swapchainExtent;
	std::vector<vk::ImageView> swapchainImageViews;	// Necessary to visualize the vk::Images
	vk::PipelineLayout pipelineLayout;

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
		createImageViews();
		createGraphicsPipeline();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		device.destroyPipelineLayout(pipelineLayout);
		for (auto imageView : swapchainImageViews) {
			device.destroyImageView(imageView);
		}
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

			vk::InstanceCreateInfo instanceCreateInfo({},
				&applicationInfo,
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
			instance = vk::createInstance(
				vk::InstanceCreateInfo({},
					&applicationInfo,
					0, nullptr,	// validation layers
					extensions.size(), extensions.data()
				)
			);
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
			std::cout << "Unsupported extensions: ";
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
			for (const auto& layerProperties : availableLayers) {
				if (std::strcmp(layerName, layerProperties.layerName.data()) == 0) {
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
		void* const  /*pUserData*/
		) {
		std::cerr << "Validation layer: " << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": " << pCallbackData->pMessage << '\n';
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
		const std::vector physicalDevices {instance.enumeratePhysicalDevices()};
		if (physicalDevices.empty()) {
			throw std::runtime_error("Falied to find GPUs with Vulkan support");
		}
		else {
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

	[[nodiscard]] bool isPhysicalDeviceSuitable(const vk::PhysicalDevice physicalDevice) const {
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

	[[nodiscard]] QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice physicalDevice) const {
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

		queueCreateInfos.reserve(uniqueQueueFamilies.size());
		for (const uint32_t queueFamily : uniqueQueueFamilies) {
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));
		}

		vk::PhysicalDeviceFeatures physicalDeviceFeatures;

		device = physicalDevice.createDevice(
			vk::DeviceCreateInfo({},
				queueCreateInfos.size(), queueCreateInfos.data(),
				[](){
					if constexpr (enableValidationLayers) {
						return validationLayers.size(); 
					}
					else return 0;
				}(),
				[](){
					if constexpr (enableValidationLayers) {
						return validationLayers.data();
					}
					else return nullptr;
				}(),
				requestedPhysicalDeviceExtensions.size(), requestedPhysicalDeviceExtensions.data(),
				&physicalDeviceFeatures
			)
		);
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
			requiredExtensions.erase(extension.extensionName.data());
		}
		// If the set of required extension is empty, it means that all my required extensions
		// are supported by the physicalDevice
		return requiredExtensions.empty();
	}

	struct SwapchainDetails {
		std::vector<vk::SurfaceFormatKHR> formats;	// Color depth
		std::vector<vk::PresentModeKHR> presentModes;	// Conditions for swapping images to the screen
		vk::SurfaceCapabilitiesKHR capabilities;

		[[nodiscard]] bool isAdequate() const {
			return !formats.empty() && !presentModes.empty();
		}
	};

	[[nodiscard]] SwapchainDetails querySwapchainSupport(vk::PhysicalDevice physicalDevice) const {
		return SwapchainDetails {
			physicalDevice.getSurfaceFormatsKHR(surface),
			physicalDevice.getSurfacePresentModesKHR(surface),
			physicalDevice.getSurfaceCapabilitiesKHR(surface)
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
			true,												// Enable clipping for best performance, but I'm unable to see the pixels obscured by a window on top
			{/*oldSwapchain*/}
		);

		const QueueFamilyIndices indices {findQueueFamilies(physicalDevice)};
		// If I have two different queues they'll be able to access the image at the same time.
		// This way, I have to specify which families will be using the swapchain
		if (indices.graphicsFamily.value() != indices.presentationFamily.value()) {
			//const std::array queueFamilyIndices {indices.graphicsFamily.value(), indices.presentationFamily.value()};
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			// TODO: Probably the scope of the std::array must be the same as when device.createSwapchainKHR(swapchainCreateInfo) gets called
			swapchainCreateInfo.pQueueFamilyIndices = std::array{indices.graphicsFamily.value(), indices.presentationFamily.value()}.data();
		}
		
		swapchain = device.createSwapchainKHR(swapchainCreateInfo);
		swapchainImages = device.getSwapchainImagesKHR(swapchain);
		swapchainImageFormat = surfaceFormat.format;
		swapchainExtent = extent;
	}
	
	void createImageViews() {
		swapchainImageViews.resize(swapchainImages.size());

		for (size_t i {0}; i < swapchainImageViews.size(); ++i) {
			swapchainImageViews[i] = device.createImageView(
				vk::ImageViewCreateInfo({},
					swapchainImages[i],
					vk::ImageViewType::e2D,
					swapchainImageFormat,
					{/*components*/},						// These allow me to swizzle around the color channels. Here I'm leaving everything normal
					vk::ImageSubresourceRange(
						vk::ImageAspectFlagBits::eColor,	// My images will be used as color targets
						/*baseMipLevel*/ 	0,				// No mipmap
						/*levelCount*/		1,
						/*baseArrayLayer*/	0,				// Single layer, 'cause I'm not working with
						/*layerCount*/		1				// a stereostopic 3D program
					)
				)
			);
		}
	}

	void createGraphicsPipeline() {
		// Shaders are created during the creation of the pipeline, every time.
		// I can destroy them here, I don't need to make them class members
		const vk::ShaderModule vertShaderModule {createShaderModule(readFile("shaders/vert.spv"))};
		const vk::ShaderModule fragShaderModule {createShaderModule(readFile("shaders/frag.spv"))};

		// Maybe std::array?
		const std::pair shaderStageInfos {
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"),
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main")
		};

		// Empty info since I'm not sending any vertex input (vertices are already hardcoded in the shader)
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({},
			vk::PrimitiveTopology::eTriangleList, true
		);

		vk::Viewport viewport(0.0F, 0.0F, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0F, 1.0F);
		// Things outside of the scissor rectangle will not get rendered
		vk::Rect2D scissor({0, 0}, swapchainExtent);
		vk::PipelineViewportStateCreateInfo viewportState({},
			1, &viewport, 1, &scissor
		);

		vk::PipelineRasterizationStateCreateInfo resterizerInfo({},
			/*DepthClamp*/ false,				// Used to clamp instead of discard fragments beyond the planes (?)
			/*RasterizerDiscard*/ false,		// If set to true, it just discards all the output to the framebuffer lol
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,		// To avoid rendering the back face of an object ('cause it is not visible anyway)
			vk::FrontFace::eClockwise,			// Still didn't got this :/  Specifies the vertex order for faces to be considered front-facing
			/*DepthBias*/ false, {}, {}, {},	// Here I could alter the depth values, used for shadow mapping
			/*LineWidth*/ 1.0F					// For lines thicker than 1 I'd need the wideLines GPU feature
		);

		vk::PipelineMultisampleStateCreateInfo multisamplingInfo({}, vk::SampleCountFlagBits::e1, /*Enabled*/ false);

		// Here I'm just disabling color blending. For example, an polygon with alpha = 1 and another with alpha = 0 will be rendered the same way
		vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(
			false,
			vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,	// src and dest Color
			vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,	// src and dest Alpha
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA	// To pass all the colors
		);

		vk::PipelineColorBlendStateCreateInfo colorBlendInfo({},
			/*EnableLogicOp*/ false, vk::LogicOp::eCopy,
			/*AttachmentCount*/ 1, &colorBlendAttachmentState
		);

		// Used to dynamically change a few things of the (almost immutable) pipeline
		std::array dynamicStates {vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};
		vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates.size(), dynamicStates.data());

		// Used to set the uniforms in the shaders
		pipelineLayout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo());

		device.destroyShaderModule(vertShaderModule);
		device.destroyShaderModule(fragShaderModule);
	}

	static std::vector<std::byte> readFile(const std::string_view fileName) {
		std::ifstream file(fileName.data(), std::ios::ate | std::ios::binary);	// Opened at the end to know the file size
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file");
		}

		const auto fileSize {file.tellg()};
		file.seekg(0);

		std::vector<std::byte> buffer(fileSize);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		file.close();
		return buffer;
	}

	vk::ShaderModule createShaderModule(const std::vector<std::byte>& code) {
		return device.createShaderModule(
			vk::ShaderModuleCreateInfo({},
				code.size(), reinterpret_cast<const uint32_t*>(code.data())
			)
		);
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::clog << e.what() << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
// std::cerr << "———————————————————————————————————————————————————————————————————————————————————\n";
