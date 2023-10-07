#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>
#include <map>
#include <set>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>



struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
};


const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};



class HelloTriangleApplication {
public:


	GLFWwindow* window;
	VkInstance instance;
	VkDebugReportCallbackEXT callback; //调试 回报  回调 EXT
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const std::vector<const char*> validationLayers = { //验证 层
"VK_LAYER_LUNARG_standard_validation" //层 lunarg 标准 验证
	};
#ifdef NDEBUG	//NDEBUG 宏是C++标准的一部分，意思是“不调试”。
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true; //开启 验证 层
#endif

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();

	}
private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; //物理设备
	VkDevice device; //逻辑设备
	VkQueue graphicsQueue; //图样 队列
	VkQueue presentQueue;//目前 队列
	VkSurfaceKHR surface;//曲面
	VkSwapchainKHR swapChain;//交换 链
	std::vector<VkImage> swapChainImages;//交换链 图像7
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass; // 渲染过程
	VkPipelineLayout pipelineLayout;//管道 层 用于动态配置管道
	VkPipeline graphicsPipeline;//图形管道
	std::vector<VkFramebuffer> swapChainFramebuffers;//交换帧缓冲区
	VkCommandPool commandPool;//命令池
	std::vector<VkCommandBuffer> commandBuffers;//命令缓冲区 组
	VkSemaphore imageAvailableSemaphore;//信号量成员 以准备好可以进行渲染
	VkSemaphore renderFinishedSemaphore;//信号量成员 渲染完成



	const std::vector<const char*> deviceExtensions = {//编译器将捕获拼写错误？
		VK_KHR_SWAPCHAIN_EXTENSION_NAME //交换链 扩展 名称
	};

	struct QueueFamilyIndices { //队列 家族 指数
		int graphicsFamily = -1;//图样 家族
		int presentFamily = -1; //目前 家族
		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};
	struct SwapChainSupportDetails { //交换链 支持 细节
		VkSurfaceCapabilitiesKHR capabilities;//曲面 能力
		std::vector<VkSurfaceFormatKHR> formats;//格式
		std::vector<VkPresentModeKHR> presentModes;//目前模板
	};

	/**
	* 交换链支持详情
	*/
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		/*此函数将考虑指定的VkPhysicalDevice和VkSurfaceKHR窗口表面。
		所有支持查询函数都将这两个作为第一个参数，因为它们是交换链的核心组件。*/
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		/*查询支持的曲面格式,确保向量已调整大小，容纳所有可用格式*/
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		/*查询受支持的演示模式与vkGetPhysicalDeviceSurfacePresentModesKHR：*/
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}


		return details;
	}
	/*选择 交换 曲面 格式*/
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) { //(可获得 格式)
		/*最好的情况是，surface没有首选格式，Vulkan只返回一个VkSurfaceFormatKHR条目，
		该条目的format成员设置为VK_format_UNDEFINED*/
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		/*如果我们不能自由选择任何格式，那么我们将查看列表，看看是否有首选组合：*/
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		/*如果这也失败了，那么我们可以开始根据可用格式的“好”程度对其进行排名，但在大多数情况下，只使用指定的第一种格式是可以的。*/

		return availableFormats[0];

	}
	/*选择 交换 目前 模板*/

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				/*如果 三缓冲不行，使用单缓冲？ 我认为多数人不能接收撕裂效果*/
				bestMode = availablePresentMode;
			}
		}
		return bestMode;
	}
	/*选择 交换 范围*/
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			VkExtent2D actualExtent = { width, height };
			//VkExtent2D actualExtent = { WIDTH, HEIGHT };
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}



	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		/*检索队列族列表的过程正是您所期望的，并使用VKGetPhysicalDeviceQueueFamilyProperty*/
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		/*VkQueueFamilyProperties结构包含有关队列族的一些详细信息，
		包括支持的操作类型以及可以基于该族创建的队列数量。我们需要找到至少一个支持VK_queue_GRAPHICS_BIT的队列系列。*/

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			//然后只需检查布尔值并存储演示系列队列索引：
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.graphicsFamily = i;
				//不加这个会报错，加上后不报错没有使用，应该不影响结果
				indices.presentFamily = i;
			}
			if (indices.isComplete()) {
				break;
			}
			i++;
		}
		return indices;
	}

	void initVulkan() {
		createInstance();  //   VkInstance   Instance  
		setupDebugCallback(); //安装 debug 回调 需要 instance 输出 VkDebugReportCallbackEXT callback
		createSurface(); //创建曲面 需要 instance 输出 VkSurfaceKHR surface
		pickPhysicalDevice(); //创建物理设备 需要 instance 输出 VkPhysicalDevice physicalDevice
		createLogicalDevice(); //创建逻辑设备  需要 physicalDevice 输出 VkDevice device //逻辑设备 创建 VkQueue graphicsQueue  //图样 队列 VkQueue presentQueue;//目前 队列
		createSwapChain(); //需要physicalDevice//物理设备 surface 曲面 //device 逻辑设备 输出  swapChainExtent 交换链图像宽高 VkFormat swapChainImageFormat; //交换链图像格式 swapChain //交换链 pipelineLayout//管道层 创建 std::vector<VkImage> swapChainImages;//交换链 图像7
		createImageViews();//需要交换链图像swapChainImages.size(大小？个数？看起来像个数), swapChainImageFormat//交换链图像格式 输出交换链图像视图 swapChainImageViews
		createRenderPass();// 渲染通过 需要交换链图像swapChainImages.size(大小？个数？看起来像个数), swapChainImageFormat//交换链图像格式 输出交换链图像视图 swapChainImageViews
		createGraphicsPipeline(); //图形管道  输出 graphicsPipeline
		createFramebuffers();//帧缓冲区  需要交换图像视图 swapChainImageViews 输出 swapChainFramebuffers
		createCommandPool();//创建命令池  需要物理设备 physicalDevice 输出 commandPool
		createCommandBuffers();//为每个交换链图像分配和记录命令 需要 逻辑设备device  swapChainFramebuffers 和 commandPool 创建 commandBuffers
		createSemaphores();//初始化信号量

	}
	void createSemaphores() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}

	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;//命令缓冲区也已挂起执行，可以重新提交。
			beginInfo.pInheritanceInfo = nullptr; // Optional
			vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);//绘制三角形？
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	//创建命令池
	void createCommandPool() {
		//创造命令池只要两个参数
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	//创建帧缓冲区
	void createFramebuffers() {
		//首先调整容器大小以容纳所有帧缓冲区：
		swapChainFramebuffers.resize(swapChainImageViews.size());
		//然后，我们将遍历图像视图并从中创建帧缓冲区：
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
			swapChainImageViews[i]
			};
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;
			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

		}
	}


	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		/*通过调用vkCreateShaderModule来创建VkShaderModule*/
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shaderModule;

	}


	/**
	* 创建 图像 试图 需要交换链图像swapChainImages.size(大小？个数？看起来像个数), swapChainImageFormat//交换链图像格式 输出交换链图像视图 swapChainImageViews
	*/
	void createImageViews() {
		/*调整列表的大小*/
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			/*解释图像数据*/
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;//允许您将图像视为1D纹理、2D纹理、3D纹理和立方体贴图
			createInfo.format = swapChainImageFormat;
			/*允许您旋转颜色通道*/
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}

		}
	}

	/**
	* 渲染通道 需要交换链图像格式  swapChainImageFormat 逻辑设备 device 输出 render pass
	*/
	void createRenderPass() {
		/*附件 描述*/
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//1个样品

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;/*保留附件的现有内容*/
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;/*呈现的内容将存储在内存中，以后可以读取*/
		/*loadOp和storeOp应用于颜色和深度数据，而stencilLoadOp/stencilStoreOp应用于模具数据。我们的应用程序不会处理模具缓冲区，因此加载和存储的结果是无关的。*/
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		/*Vulkan中的纹理和帧缓冲区由具有特定像素格式的VkImage对象表示，但是内存中像素的布局可能会根据您试图对图像执行的操作而改变*/
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;/*指定在渲染过程开始之前图像将具有的布局*/ //这里代表不关心图像之前的布局
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;/*指定渲染过程完成时自动转换到的布局*/ //完成时最终布局为交换链中的图像
		/*渲染过程可以由多个子过程组成,每个子类都引用一个或多个附件*/
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;//附件索引
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//图像_布局_颜色_附件_最佳布局 为我们提供最佳性能
		/*子类使用VkSubpassDescription结构进行描述*/
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//表示这是一个图形子类

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		/*依赖项负责渲染过程开始时和渲染过程结束时的过渡
		前两个字段指定依赖项和依赖子类的索引。特殊值VK_SUBPASS_EXTERNAL是指渲染过程之前或之后的隐式子过程，具体取决于它是在srcSubpass还是dstSubpass中指定的。
		索引0引用我们的子类，它是第一个也是唯一一个。dstSubpass必须始终高于srcSubpass，以防止依赖关系图中出现循环。
		*/

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}





	}
	/**
	* 图形管道  需要 vs fs
	*/
	void createGraphicsPipeline() {//图形 管道
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		/*调用我们创建的用于加载着色器模块的辅助函数： 创建shaderModule*/
		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		/*填充顶点着色器的结构*/
		//vk 管道  shader 阶段 创建详情
		//确定类型 shader阶段 vs /fs   shader单元 名称  使用结构体形式进行 shader pipeline 进行设置
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		/*着色器将在哪个管道阶段使用。上一章中描述的每个可编程阶段都有一个枚举值*/
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // 阶段  vertex 阶段

		vertShaderStageInfo.module = vertShaderModule;//vs单元
		vertShaderStageInfo.pName = "main";
		//同上 创建fs shader 详细设置

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//fragment
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		// shader 阶段组 

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


		//管线 顶点 传入 详情 设置
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

		/*顶点绘制什么样的几何体，以及是否应启用基本体重新启动*/
		//管线 传入 装配 阶段 设置
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;  //类型
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//绘制三角形
		inputAssembly.primitiveRestartEnable = VK_FALSE; //原始重置 开启?

		/*裁剪 视口*/
		//视口设置 
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//矩形 裁剪 设置
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		//管线 视口 阶段 详情 设置
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;//类型
		viewportState.viewportCount = 1;	//视口数量？
		viewportState.pViewports = &viewport;//视口
		viewportState.scissorCount = 1;//矩形裁剪 数量？
		viewportState.pScissors = &scissor;// 矩形 裁剪

		/*VkPipelineRasterizationStateCreateInfo 可以配置线框渲染*/
		//管线  光栅化阶段创建详情
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;//VK_TRUE超出近平面和远平面的碎片将被夹住，而不是丢弃它们这在一些特殊情况下很有用，比如阴影贴图。使用此功能需要启用GPU功能
		/*如果光栅化器启用设置为VK_TRUE，则几何体永远不会通过光栅化器阶段。这基本上会禁用帧缓冲区的任何输出。*/
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		/*polygonMode确定如何为几何体生成片段。以下模式可用：*/
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;/*面剔除的类型。可以禁用消隐、消隐正面、消隐背面或同时禁用两者*/
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;/*被视为正面的面的顶点顺序，可以是顺时针或逆时针*/

		rasterizer.depthBiasEnable = VK_FALSE; //深度偏移
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		/*MSAA 多重采样抗锯齿*/
		//管线 msaa 
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;//采样着色开启
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//光栅化采样
		multisampling.minSampleShading = 1.0f; // Optional  最小采样着色
		multisampling.pSampleMask = nullptr; // Optional 采样遮罩
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional 不透明 到 覆盖范围 开启
		multisampling.alphaToOneEnable = VK_FALSE; // Optional 不透明 到 1 开启

		//管线 颜色 混合 附件 阶段
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		/* // alpha blend
		colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		*/

		/*引用所有帧缓冲区的结构数组，设置混合常数*/
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		/*动态修改配置*/
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkGraphicsPipelineCreateInfo pipelineInfo = {};//图样 管道 创建
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages; // 着色器舞台
		/*我们首先引用VkPipelineShaderStageCreateInfo结构的数组。*/
		pipelineInfo.pVertexInputState = &vertexInputInfo;//顶点输入状态
		pipelineInfo.pInputAssemblyState = &inputAssembly;//输入 集合  状态
		pipelineInfo.pViewportState = &viewportState; //视口 状态
		pipelineInfo.pRasterizationState = &rasterizer; //光栅化状态
		pipelineInfo.pMultisampleState = &multisampling;//多重采样状态
		pipelineInfo.pDepthStencilState = nullptr; // Optional//深度模板状态
		pipelineInfo.pColorBlendState = &colorBlending;//颜色混合状态
		pipelineInfo.pDynamicState = nullptr; // Optional//物理状态
		/*然后我们引用所有描述固定功能阶段的结构。*/
		pipelineInfo.layout = pipelineLayout;//管道层
		/*之后是管道布局，它是Vulkan句柄，而不是结构指针。*/
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		/*将其他渲染过程用于此管道，这里未使用*/
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		/*创建图形管道并返回createGraphicsPipeline后，应清理它们，因此请确保在函数结束时删除它们：*/
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);

	}
	/**
	* 创建交换链 需要physicalDevice//物理设备 surface 曲面 //device 逻辑设备 输出  swapChain //交换链 pipelineLayout//管道层
	*
	*/

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice); //交换链支持
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;//交换数量 
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}
		/*创建交换链对象需要填充大型结构*/
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		/*指定交换链应绑定到的曲面后，将指定交换链图像的详细信息：*/
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;//指定每个图像包含的层的数量
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //复合 alpha 不透明

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		/*创建交换链现在就像调用vkCreateSwapchainKHR一样简单：*/
		/*参数包括@逻辑设备、@交换链创建信息、@可选的自定义分配器和@指向存储句柄的变量的指针。*/
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);/*查询交换链中的图像数*/
		swapChainImages.resize(imageCount);/*调整容器的大小*/
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());/*检索句柄*/

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}


	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		/*读取位置来确定文件的大小并分配缓冲区*/
		size_t fileSize = (size_t)file.tellg();
		//std::cout << fileSize << std::endl;
		std::vector<char> buffer(fileSize);
		/*返回到文件的开头，一次读取所有字节*/
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		/*最后关闭文件并返回字节*/
		file.close();
		return buffer;
	}


	/*检查物理设备*/

	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		//return indices.isComplete() && extensionsSupported;
		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	//	/*我们将检查是否有任何物理设备符合我们将提供的要求添加到该函数中。*/
	//
	///*可以从设备中查询更多详细信息，我们将在后面讨论关于设备内存和队列系列（请参阅下一节）。
	//例如，假设我们认为我们的应用程序只能用于专用的支持几何体着色器的图形卡。那么这是可以选择的函数如下所示：*/
	//	bool isDeviceSuitable(VkPhysicalDevice device) {
	//		/*基本设备适用性检查为了评估设备的适用性，我们可以从查询一些细节开始。
	//		基本设备属性，如名称、类型和支持的Vulkan版本可以可以使用vkGetPhysicalDeviceProperties查询。*/
	//		VkPhysicalDeviceProperties deviceProperties;
	//		vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//		/*支持纹理压缩、64位浮点和可以使用vkGetPhysicalDe viceFeatures查询多视口渲染（对VR有用）：*/
	//		VkPhysicalDeviceFeatures deviceFeatures;
	//		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	//
	//		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
	//			deviceFeatures.geometryShader;
	//	}



		/*检查是否所有必需的扩展都在其中*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {//选择 设备 扩展 支持
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();

	}


	/**
	* 创建曲面
	*/

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	/**
	* 创建逻辑设备
	*/
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };
		std::cout << indices.graphicsFamily << indices.presentFamily << std::endl;
		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}




		///*此结构描述了我们希望单个队列系列的队列数量*/
		//VkDeviceQueueCreateInfo queueCreateInfo = {};
		//queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
		//queueCreateInfo.queueCount = 1;
		///*当前可用的驱动程序只允许您为每个家庭队列创建少量队列，并且您实际上不需要多个队列。
		//这是因为您可以在多个线程上创建所有命令缓冲区，然后通过一个低开销调用在主线程上一次性提交它们。
		//Vulkan允许您使用介于0.0和1.0之间的浮点数为队列分配优先级，以影响命令缓冲区执行的调度。
		//即使只有一个队列，也需要这样做：*/
		//float queuePriority = 1.0f;
		//queueCreateInfo.pQueuePriorities = &queuePriority;
		/*这些是我们在上一章中询问的vkGetPhysicalDeviceFeatures支持的功能，比如几何体着色器。
		现在我们不需要任何特别的东西，所以我们可以简单地定义它，并将一切留给VK_FALSE。
		一旦我们要开始对Vulkan做更多有趣的事情，我们将回到这个结构。*/
		VkPhysicalDeviceFeatures deviceFeatures = {};
		/*创建逻辑设备
		有了前两个结构，我们就可以开始填充VkDeviceCreateInfo主结构了。*/
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		///*首先向队列创建信息和设备功能结构添加指针：*/
		//createInfo.pQueueCreateInfos = &queueCreateInfo;
		//createInfo.queueCreateInfoCount = 1;

		/*修改VkDeviceCreateInfo以指向向量：*/
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		/*我们将为设备启用与实例相同的验证层。我们现在不需要任何特定于设备的扩展。*/
		//createInfo.enabledExtensionCount = 0;

		/*启用扩展验证*/
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();



		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}
		/*就这样，我们现在可以通过调用适当命名的vkCreateDevice函数来实例化逻辑设备。*/
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		/*我们可以使用vkGetDeviceQueue函数来检索每个队列系列的队列句柄。
		参数包括@逻辑设备、@队列系列、@队列索引和@指向存储队列句柄的变量的指针。
		因为我们只从这个系列中创建一个队列，所以我们只使用索引0。*/
		//不太确定是否应该使用这个，不使用前面的铺垫都是无效的，所以暂时认为应该进行使用
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);

		/*如果队列族相同，那么我们只需要传递其索引一次。最后，添加一个调用以检索队列句柄：*/

		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);


	}


	/**
	* 选择物理设备
	*/
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		/*如果有0台设备支持Vulkan，那么就没有必要再进一步了。*/
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		/*否则，我们现在可以分配一个数组来保存所有VkPhysicalDevice把手。*/

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		/*现在我们需要评估每一个，并检查它们是否适合我们想要执行的操作，因为并非所有图形卡都是平等创建的。
			为此，我们将引入一个新函数：*/
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		// Use an ordered map to automatically sort candidates by increasing score
		//使用有序映射通过增加分数自动对候选人进行排序
		std::multimap<int, VkPhysicalDevice> candidates;
		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			std::cout << score << std::endl;
			candidates.insert(std::make_pair(score, device));
		}
		// Check if the best candidate is suitable at all
		//检查最佳候选人是否合适
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

	}

	int rateDeviceSuitability(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		int score = 0;
		// Discrete GPUs have a significant performance advantage
		//离散GPU具有显著的性能优势
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}
		// Maximum possible size of textures affects graphics quality
		//纹理的最大可能大小会影响图形质量

		score += deviceProperties.limits.maxImageDimension2D;

		std::cout << deviceProperties.limits.maxImageDimension2D << std::endl;

		// Application can't function without geometry shaders
		//没有几何体着色器，应用程序无法运行
		if (!deviceFeatures.geometryShader) {
			return 0;
		}
		return score;
	}




	/*现在我们有了这个奇特的队列族查找功能，我们可以使用它作为ISDeviceSuite功能中的检查，
以确保设备能够处理我们想要使用的命令：
	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}*/


	void mainLoop() {
		//循环事件  
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	/**
	* 绘制帧
	* 需要commandBudfder命令缓冲区
	* imageAvailableSemaphore  用表示渲染准备情况，可以获取渲染准备完成
	* 交换链 swapChain
	* renderFinishedSemaphore 用来表示渲染完成情况
	* graphicsQueue 图样队列
	*
	*/
	void drawFrame() {

		uint32_t imageIndex;

		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);//获得图像index
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		/*前三个参数指定在执行开始之前等待哪些信号量，以及在管道的哪个阶段等待。我们希望等待将颜色写入图像，直到它可用，
		因此我们指定了写入颜色附件的图形管道的阶段。这意味着理论上，实现可以开始执行我们的顶点着色器，而图像还不可用。
		waitStages数组中的每个条目对应于pWaitSemaphores中具有相同索引的信号量。*/
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		/*接下来的两个参数指定实际提交执行的命令缓冲区。如前所述，我们应该提交命令缓冲区，
		该缓冲区绑定我们刚刚作为颜色附件获取的交换链图像。*/
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		/*signalSemaphoreCount和pSignalSemaphores参数指定在命令缓冲区完成执行后发送信号的信号量。在我们的例子中，我们使用renderFinishedSemaphore来实现这个目的。*/
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		/*提交渲染队列*/
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;/**/

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo); //渲染演示队列
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		vkQueueWaitIdle(presentQueue);//等待渲染队列完成  这个在启用验证层的情况下会避免验证层带来的内存泄漏

	}

	void cleanupSwapChain() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void cleanup() {
		cleanupSwapChain();

		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);


		vkDestroyDevice(device, nullptr);

		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		/*VkinInstance只应该在程序退出之前销毁。它可以使用vkDestroyInstance函数在清理过程中被销毁：*/
		vkDestroyInstance(instance, nullptr);
		/*vkDestroyInstance函数的参数非常简单。像如前一章所述，
		中的分配和解除分配函数Vulkan有一个可选的分配器回调，我们将通过传递nullptr忽略它去吧。
		我们将在以下章节中创建的所有其他Vulkan资源应该在实例被销毁之前进行清理。
		在创建实例之后继续执行更复杂的步骤之前，是时候了通过检查验证层来评估我们的调试选项。*/


		//终止glfw本身
		glfwDestroyWindow(window);
		glfwTerminate();

	}

	//调整窗口大小
	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);

	}



	//void initWindow() {

	//	glfwInit();

	//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//创建窗口 不需要要后续API

	//	//因为处理调整大小的窗口需要特别小心，我们将在后面进行研究，
	//	//通过另一个窗口提示调用暂时禁用它：
	//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//	//现在剩下的就是创建实际的窗口。添加一个GLFWwindow*窗口；
	//	//private类成员存储对它的引用，并使用以下命令初始化窗口：
	//	//window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);//@宽度@高度@标题@指定监视器打开@OpenGL功能


	//	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	//}


	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;
		HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain();

	}


	/**
	* 创建 VkInstance 实例
	*/

	void createInstance() {

		//检测图层是否可用
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}


		//现在，要创建一个实例，我们首先必须用一些信息填充一个结构
		//关于我们的申请。该数据在技术上是可选的，但它可能提供
		//	为驾驶员提供一些有用的信息，以便针对我们的特定应用进行优化，
		//	例如，因为它使用了一个著名的图形引擎，具有某些特殊的功能
		//	行为此结构称为VkApplicationInfo：

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;//如前所述，Vulkan中的许多结构都要求您显式指定sType成员中的类型。
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		/*Vulkan中的很多信息都是通过结构而不是函数参数传递的，我们必须再填充一个结构来提供足够的信息
		用于创建实例。下一个结构不是可选的，它告诉Vulkan驱动我们想要使用的全局扩展和验证层。这里是全球
		这意味着它们适用于整个程序，而不是特定的设备	将在接下来的几章中变得清晰*/
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		/*前两个参数很简单。接下来的两层指定期望的全局扩展。如概述一章所述，Vulkan是一个
		平台无关API，这意味着您需要对接口进行扩展使用窗口系统。GLFW有一个方便的内置函数glfwGetRequiredInstanceExtensions，
		可以返回扩展它需要执行我们可以传递给结构的操作：*/
		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//createInfo.enabledExtensionCount = glfwExtensionCount;

		//createInfo.ppEnabledExtensionNames = glfwExtensions;
		/*结构的最后两个成员确定要执行的全局验证层使可能我们将在下一章更深入地讨论这些，所以请离开
		这些暂时是空的。*/

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}


		/*我们现在已经指定了Vulkan创建实例所需的一切，我们可以最后发出vkCreateInstance调用：*/

		//VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		/*正如您将看到的，对象创建函数参数所在的一般模式以下是：
			- 指向包含创建信息的结构的指针
			- 指向自定义分配器回调的指针，在本教程中始终为nullptr
			- 指向存储新对象句柄的变量的指针
		如果一切顺利，那么实例的句柄存储在VkInstance类成员。
		几乎所有Vulkan函数都返回类型为的值VkResult，表示VK_成功或错误代码。
		检查实例已成功创建，我们不需要存储结果，只需使用改为检查成功值：*/
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

		showSupport();


	}

	/*提供有关Vulkan支持的一些详细信息。(可选)*/

	void showSupport() {

		/*如果您查看vkCreateInstance文档，就会看到可能的错误代码是“VK_ERROR_EXTENSION_NOT_PRESENT”。
		我们可以简单地	指定我们需要的扩展，并在错误代码返回时终止。
		这对于Windows系统界面这样的基本扩展来说是有意义的，但是如果我们想检查可选功能呢？

		要在创建实例之前检索受支持扩展的列表，有一个vkEnumerateInstanceExtensionProperties函数。
		这需要一段时间	指向一个变量的指针，该变量存储扩展名数和扩展名数组VkExtensionProperties来存储扩展的详细信息。
		这也需要一段时间可选的第一个参数，允许我们通过特定的验证筛选扩展图层，我们暂时忽略它。

		要分配一个数组来保存扩展细节，我们首先需要知道如何分配有很多。您可以通过离开后一个参数为空：*/

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		/*最后我们可以查询扩展细节：*/
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		/*每个VkExtensionProperties结构都包含扩展的名称和版本。我们可以用一个简单的for循环（\t是缩进选项卡）列出它们：*/
		std::cout << "available extensions:" << std::endl;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}

	}


	//验证
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
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
	/*根据是否启用验证层返回所需的扩展列表：*/
	std::vector<const char*> getRequiredExtensions() {
		std::vector<const char*> extensions;
		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			extensions.push_back(glfwExtensions[i]);
		}
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		return extensions;
	}
	/*添加一个新的静态成员使用PFN_vkDebugReportCallbackEXT pro totype调用debugCallback函数。
	VKAPI_ATTR和VKAPI_调用确保函数具有正确的签名让Vulkan称之为*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {
		std::cerr << "validation layer: " << msg << std::endl;
		return VK_FALSE;
	}

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	void recreateSwapChain() {
		vkDeviceWaitIdle(device);

		cleanupSwapChain();//确保创建之前清理之前的旧版本

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}




	/**
	* 安装debug回调
	* 需要instance 输出callback
	*/
	void setupDebugCallback() {
		/*我们需要填写一个包含回调细节的结构*/
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
		if (!enableValidationLayers) return;
	}

};

int main() {
	HelloTriangleApplication app;
	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

