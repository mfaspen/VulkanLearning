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
	VkDebugReportCallbackEXT callback; //���� �ر�  �ص� EXT
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const std::vector<const char*> validationLayers = { //��֤ ��
"VK_LAYER_LUNARG_standard_validation" //�� lunarg ��׼ ��֤
	};
#ifdef NDEBUG	//NDEBUG ����C++��׼��һ���֣���˼�ǡ������ԡ���
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true; //���� ��֤ ��
#endif

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();

	}
private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; //�����豸
	VkDevice device; //�߼��豸
	VkQueue graphicsQueue; //ͼ�� ����
	VkQueue presentQueue;//Ŀǰ ����
	VkSurfaceKHR surface;//����
	VkSwapchainKHR swapChain;//���� ��
	std::vector<VkImage> swapChainImages;//������ ͼ��7
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass; // ��Ⱦ����
	VkPipelineLayout pipelineLayout;//�ܵ� �� ���ڶ�̬���ùܵ�
	VkPipeline graphicsPipeline;//ͼ�ιܵ�
	std::vector<VkFramebuffer> swapChainFramebuffers;//����֡������
	VkCommandPool commandPool;//�����
	std::vector<VkCommandBuffer> commandBuffers;//������� ��
	VkSemaphore imageAvailableSemaphore;//�ź�����Ա ��׼���ÿ��Խ�����Ⱦ
	VkSemaphore renderFinishedSemaphore;//�ź�����Ա ��Ⱦ���



	const std::vector<const char*> deviceExtensions = {//������������ƴд����
		VK_KHR_SWAPCHAIN_EXTENSION_NAME //������ ��չ ����
	};

	struct QueueFamilyIndices { //���� ���� ָ��
		int graphicsFamily = -1;//ͼ�� ����
		int presentFamily = -1; //Ŀǰ ����
		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};
	struct SwapChainSupportDetails { //������ ֧�� ϸ��
		VkSurfaceCapabilitiesKHR capabilities;//���� ����
		std::vector<VkSurfaceFormatKHR> formats;//��ʽ
		std::vector<VkPresentModeKHR> presentModes;//Ŀǰģ��
	};

	/**
	* ������֧������
	*/
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		/*�˺���������ָ����VkPhysicalDevice��VkSurfaceKHR���ڱ��档
		����֧�ֲ�ѯ����������������Ϊ��һ����������Ϊ�����ǽ������ĺ��������*/
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		/*��ѯ֧�ֵ������ʽ,ȷ�������ѵ�����С���������п��ø�ʽ*/
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		/*��ѯ��֧�ֵ���ʾģʽ��vkGetPhysicalDeviceSurfacePresentModesKHR��*/
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}


		return details;
	}
	/*ѡ�� ���� ���� ��ʽ*/
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) { //(�ɻ�� ��ʽ)
		/*��õ�����ǣ�surfaceû����ѡ��ʽ��Vulkanֻ����һ��VkSurfaceFormatKHR��Ŀ��
		����Ŀ��format��Ա����ΪVK_format_UNDEFINED*/
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		/*������ǲ�������ѡ���κθ�ʽ����ô���ǽ��鿴�б������Ƿ�����ѡ��ϣ�*/
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		/*�����Ҳʧ���ˣ���ô���ǿ��Կ�ʼ���ݿ��ø�ʽ�ġ��á��̶ȶ���������������ڴ��������£�ֻʹ��ָ���ĵ�һ�ָ�ʽ�ǿ��Եġ�*/

		return availableFormats[0];

	}
	/*ѡ�� ���� Ŀǰ ģ��*/

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				/*��� �����岻�У�ʹ�õ����壿 ����Ϊ�����˲��ܽ���˺��Ч��*/
				bestMode = availablePresentMode;
			}
		}
		return bestMode;
	}
	/*ѡ�� ���� ��Χ*/
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
		/*�����������б�Ĺ����������������ģ���ʹ��VKGetPhysicalDeviceQueueFamilyProperty*/
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		/*VkQueueFamilyProperties�ṹ�����йض������һЩ��ϸ��Ϣ��
		����֧�ֵĲ��������Լ����Ի��ڸ��崴���Ķ���������������Ҫ�ҵ�����һ��֧��VK_queue_GRAPHICS_BIT�Ķ���ϵ�С�*/

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			//Ȼ��ֻ���鲼��ֵ���洢��ʾϵ�ж���������
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.graphicsFamily = i;
				//��������ᱨ�����Ϻ󲻱���û��ʹ�ã�Ӧ�ò�Ӱ����
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
		setupDebugCallback(); //��װ debug �ص� ��Ҫ instance ��� VkDebugReportCallbackEXT callback
		createSurface(); //�������� ��Ҫ instance ��� VkSurfaceKHR surface
		pickPhysicalDevice(); //���������豸 ��Ҫ instance ��� VkPhysicalDevice physicalDevice
		createLogicalDevice(); //�����߼��豸  ��Ҫ physicalDevice ��� VkDevice device //�߼��豸 ���� VkQueue graphicsQueue  //ͼ�� ���� VkQueue presentQueue;//Ŀǰ ����
		createSwapChain(); //��ҪphysicalDevice//�����豸 surface ���� //device �߼��豸 ���  swapChainExtent ������ͼ���� VkFormat swapChainImageFormat; //������ͼ���ʽ swapChain //������ pipelineLayout//�ܵ��� ���� std::vector<VkImage> swapChainImages;//������ ͼ��7
		createImageViews();//��Ҫ������ͼ��swapChainImages.size(��С�������������������), swapChainImageFormat//������ͼ���ʽ ���������ͼ����ͼ swapChainImageViews
		createRenderPass();// ��Ⱦͨ�� ��Ҫ������ͼ��swapChainImages.size(��С�������������������), swapChainImageFormat//������ͼ���ʽ ���������ͼ����ͼ swapChainImageViews
		createGraphicsPipeline(); //ͼ�ιܵ�  ��� graphicsPipeline
		createFramebuffers();//֡������  ��Ҫ����ͼ����ͼ swapChainImageViews ��� swapChainFramebuffers
		createCommandPool();//���������  ��Ҫ�����豸 physicalDevice ��� commandPool
		createCommandBuffers();//Ϊÿ��������ͼ�����ͼ�¼���� ��Ҫ �߼��豸device  swapChainFramebuffers �� commandPool ���� commandBuffers
		createSemaphores();//��ʼ���ź���

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
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;//�������Ҳ�ѹ���ִ�У����������ύ��
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

			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);//���������Σ�
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	//���������
	void createCommandPool() {
		//���������ֻҪ��������
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	//����֡������
	void createFramebuffers() {
		//���ȵ���������С����������֡��������
		swapChainFramebuffers.resize(swapChainImageViews.size());
		//Ȼ�����ǽ�����ͼ����ͼ�����д���֡��������
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
		/*ͨ������vkCreateShaderModule������VkShaderModule*/
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shaderModule;

	}


	/**
	* ���� ͼ�� ��ͼ ��Ҫ������ͼ��swapChainImages.size(��С�������������������), swapChainImageFormat//������ͼ���ʽ ���������ͼ����ͼ swapChainImageViews
	*/
	void createImageViews() {
		/*�����б�Ĵ�С*/
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			/*����ͼ������*/
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;//��������ͼ����Ϊ1D����2D����3D�������������ͼ
			createInfo.format = swapChainImageFormat;
			/*��������ת��ɫͨ��*/
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
	* ��Ⱦͨ�� ��Ҫ������ͼ���ʽ  swapChainImageFormat �߼��豸 device ��� render pass
	*/
	void createRenderPass() {
		/*���� ����*/
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//1����Ʒ

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;/*������������������*/
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;/*���ֵ����ݽ��洢���ڴ��У��Ժ���Զ�ȡ*/
		/*loadOp��storeOpӦ������ɫ��������ݣ���stencilLoadOp/stencilStoreOpӦ����ģ�����ݡ����ǵ�Ӧ�ó��򲻻ᴦ��ģ�߻���������˼��غʹ洢�Ľ�����޹صġ�*/
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		/*Vulkan�е������֡�������ɾ����ض����ظ�ʽ��VkImage�����ʾ�������ڴ������صĲ��ֿ��ܻ��������ͼ��ͼ��ִ�еĲ������ı�*/
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;/*ָ������Ⱦ���̿�ʼ֮ǰͼ�񽫾��еĲ���*/ //�����������ͼ��֮ǰ�Ĳ���
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;/*ָ����Ⱦ�������ʱ�Զ�ת�����Ĳ���*/ //���ʱ���ղ���Ϊ�������е�ͼ��
		/*��Ⱦ���̿����ɶ���ӹ������,ÿ�����඼����һ����������*/
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;//��������
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//ͼ��_����_��ɫ_����_��Ѳ��� Ϊ�����ṩ�������
		/*����ʹ��VkSubpassDescription�ṹ��������*/
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//��ʾ����һ��ͼ������

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		/*���������Ⱦ���̿�ʼʱ����Ⱦ���̽���ʱ�Ĺ���
		ǰ�����ֶ�ָ����������������������������ֵVK_SUBPASS_EXTERNAL��ָ��Ⱦ����֮ǰ��֮�����ʽ�ӹ��̣�����ȡ����������srcSubpass����dstSubpass��ָ���ġ�
		����0�������ǵ����࣬���ǵ�һ��Ҳ��Ψһһ����dstSubpass����ʼ�ո���srcSubpass���Է�ֹ������ϵͼ�г���ѭ����
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
	* ͼ�ιܵ�  ��Ҫ vs fs
	*/
	void createGraphicsPipeline() {//ͼ�� �ܵ�
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		/*�������Ǵ��������ڼ�����ɫ��ģ��ĸ��������� ����shaderModule*/
		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		/*��䶥����ɫ���Ľṹ*/
		//vk �ܵ�  shader �׶� ��������
		//ȷ������ shader�׶� vs /fs   shader��Ԫ ����  ʹ�ýṹ����ʽ���� shader pipeline ��������
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		/*��ɫ�������ĸ��ܵ��׶�ʹ�á���һ����������ÿ���ɱ�̽׶ζ���һ��ö��ֵ*/
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // �׶�  vertex �׶�

		vertShaderStageInfo.module = vertShaderModule;//vs��Ԫ
		vertShaderStageInfo.pName = "main";
		//ͬ�� ����fs shader ��ϸ����

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//fragment
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		// shader �׶��� 

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


		//���� ���� ���� ���� ����
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

		/*�������ʲô���ļ����壬�Լ��Ƿ�Ӧ���û�������������*/
		//���� ���� װ�� �׶� ����
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;  //����
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//����������
		inputAssembly.primitiveRestartEnable = VK_FALSE; //ԭʼ���� ����?

		/*�ü� �ӿ�*/
		//�ӿ����� 
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//���� �ü� ����
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		//���� �ӿ� �׶� ���� ����
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;//����
		viewportState.viewportCount = 1;	//�ӿ�������
		viewportState.pViewports = &viewport;//�ӿ�
		viewportState.scissorCount = 1;//���βü� ������
		viewportState.pScissors = &scissor;// ���� �ü�

		/*VkPipelineRasterizationStateCreateInfo ���������߿���Ⱦ*/
		//����  ��դ���׶δ�������
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;//VK_TRUE������ƽ���Զƽ�����Ƭ������ס�������Ƕ�����������һЩ��������º����ã�������Ӱ��ͼ��ʹ�ô˹�����Ҫ����GPU����
		/*�����դ������������ΪVK_TRUE���򼸺�����Զ����ͨ����դ�����׶Ρ�������ϻ����֡���������κ������*/
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		/*polygonModeȷ�����Ϊ����������Ƭ�Ρ�����ģʽ���ã�*/
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;/*���޳������͡����Խ����������������桢���������ͬʱ��������*/
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;/*����Ϊ�������Ķ���˳�򣬿�����˳ʱ�����ʱ��*/

		rasterizer.depthBiasEnable = VK_FALSE; //���ƫ��
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		/*MSAA ���ز��������*/
		//���� msaa 
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;//������ɫ����
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//��դ������
		multisampling.minSampleShading = 1.0f; // Optional  ��С������ɫ
		multisampling.pSampleMask = nullptr; // Optional ��������
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional ��͸�� �� ���Ƿ�Χ ����
		multisampling.alphaToOneEnable = VK_FALSE; // Optional ��͸�� �� 1 ����

		//���� ��ɫ ��� ���� �׶�
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

		/*��������֡�������Ľṹ���飬���û�ϳ���*/
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

		/*��̬�޸�����*/
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkGraphicsPipelineCreateInfo pipelineInfo = {};//ͼ�� �ܵ� ����
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages; // ��ɫ����̨
		/*������������VkPipelineShaderStageCreateInfo�ṹ�����顣*/
		pipelineInfo.pVertexInputState = &vertexInputInfo;//��������״̬
		pipelineInfo.pInputAssemblyState = &inputAssembly;//���� ����  ״̬
		pipelineInfo.pViewportState = &viewportState; //�ӿ� ״̬
		pipelineInfo.pRasterizationState = &rasterizer; //��դ��״̬
		pipelineInfo.pMultisampleState = &multisampling;//���ز���״̬
		pipelineInfo.pDepthStencilState = nullptr; // Optional//���ģ��״̬
		pipelineInfo.pColorBlendState = &colorBlending;//��ɫ���״̬
		pipelineInfo.pDynamicState = nullptr; // Optional//����״̬
		/*Ȼ�������������������̶����ܽ׶εĽṹ��*/
		pipelineInfo.layout = pipelineLayout;//�ܵ���
		/*֮���ǹܵ����֣�����Vulkan����������ǽṹָ�롣*/
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		/*��������Ⱦ�������ڴ˹ܵ�������δʹ��*/
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		/*����ͼ�ιܵ�������createGraphicsPipeline��Ӧ�������ǣ������ȷ���ں�������ʱɾ�����ǣ�*/
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);

	}
	/**
	* ���������� ��ҪphysicalDevice//�����豸 surface ���� //device �߼��豸 ���  swapChain //������ pipelineLayout//�ܵ���
	*
	*/

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice); //������֧��
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;//�������� 
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}
		/*����������������Ҫ�����ͽṹ*/
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		/*ָ��������Ӧ�󶨵�������󣬽�ָ��������ͼ�����ϸ��Ϣ��*/
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;//ָ��ÿ��ͼ������Ĳ������
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

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //���� alpha ��͸��

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		/*�������������ھ������vkCreateSwapchainKHRһ���򵥣�*/
		/*��������@�߼��豸��@������������Ϣ��@��ѡ���Զ����������@ָ��洢����ı�����ָ�롣*/
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);/*��ѯ�������е�ͼ����*/
		swapChainImages.resize(imageCount);/*���������Ĵ�С*/
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());/*�������*/

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
		/*��ȡλ����ȷ���ļ��Ĵ�С�����仺����*/
		size_t fileSize = (size_t)file.tellg();
		//std::cout << fileSize << std::endl;
		std::vector<char> buffer(fileSize);
		/*���ص��ļ��Ŀ�ͷ��һ�ζ�ȡ�����ֽ�*/
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		/*���ر��ļ��������ֽ�*/
		file.close();
		return buffer;
	}


	/*��������豸*/

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

	//	/*���ǽ�����Ƿ����κ������豸�������ǽ��ṩ��Ҫ����ӵ��ú����С�*/
	//
	///*���Դ��豸�в�ѯ������ϸ��Ϣ�����ǽ��ں������۹����豸�ڴ�Ͷ���ϵ�У��������һ�ڣ���
	//���磬����������Ϊ���ǵ�Ӧ�ó���ֻ������ר�õ�֧�ּ�������ɫ����ͼ�ο�����ô���ǿ���ѡ��ĺ���������ʾ��*/
	//	bool isDeviceSuitable(VkPhysicalDevice device) {
	//		/*�����豸�����Լ��Ϊ�������豸�������ԣ����ǿ��ԴӲ�ѯһЩϸ�ڿ�ʼ��
	//		�����豸���ԣ������ơ����ͺ�֧�ֵ�Vulkan�汾���Կ���ʹ��vkGetPhysicalDeviceProperties��ѯ��*/
	//		VkPhysicalDeviceProperties deviceProperties;
	//		vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//		/*֧������ѹ����64λ����Ϳ���ʹ��vkGetPhysicalDe viceFeatures��ѯ���ӿ���Ⱦ����VR���ã���*/
	//		VkPhysicalDeviceFeatures deviceFeatures;
	//		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	//
	//		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
	//			deviceFeatures.geometryShader;
	//	}



		/*����Ƿ����б������չ��������*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {//ѡ�� �豸 ��չ ֧��
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
	* ��������
	*/

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	/**
	* �����߼��豸
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




		///*�˽ṹ����������ϣ����������ϵ�еĶ�������*/
		//VkDeviceQueueCreateInfo queueCreateInfo = {};
		//queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
		//queueCreateInfo.queueCount = 1;
		///*��ǰ���õ���������ֻ������Ϊÿ����ͥ���д����������У�������ʵ���ϲ���Ҫ������С�
		//������Ϊ�������ڶ���߳��ϴ����������������Ȼ��ͨ��һ���Ϳ������������߳���һ�����ύ���ǡ�
		//Vulkan������ʹ�ý���0.0��1.0֮��ĸ�����Ϊ���з������ȼ�����Ӱ���������ִ�еĵ��ȡ�
		//��ʹֻ��һ�����У�Ҳ��Ҫ��������*/
		//float queuePriority = 1.0f;
		//queueCreateInfo.pQueuePriorities = &queuePriority;
		/*��Щ����������һ����ѯ�ʵ�vkGetPhysicalDeviceFeatures֧�ֵĹ��ܣ����缸������ɫ����
		�������ǲ���Ҫ�κ��ر�Ķ������������ǿ��Լ򵥵ض�����������һ������VK_FALSE��
		һ������Ҫ��ʼ��Vulkan��������Ȥ�����飬���ǽ��ص�����ṹ��*/
		VkPhysicalDeviceFeatures deviceFeatures = {};
		/*�����߼��豸
		����ǰ�����ṹ�����ǾͿ��Կ�ʼ���VkDeviceCreateInfo���ṹ�ˡ�*/
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		///*��������д�����Ϣ���豸���ܽṹ���ָ�룺*/
		//createInfo.pQueueCreateInfos = &queueCreateInfo;
		//createInfo.queueCreateInfoCount = 1;

		/*�޸�VkDeviceCreateInfo��ָ��������*/
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		/*���ǽ�Ϊ�豸������ʵ����ͬ����֤�㡣�������ڲ���Ҫ�κ��ض����豸����չ��*/
		//createInfo.enabledExtensionCount = 0;

		/*������չ��֤*/
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();



		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}
		/*���������������ڿ���ͨ�������ʵ�������vkCreateDevice������ʵ�����߼��豸��*/
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}
		/*���ǿ���ʹ��vkGetDeviceQueue����������ÿ������ϵ�еĶ��о����
		��������@�߼��豸��@����ϵ�С�@����������@ָ��洢���о���ı�����ָ�롣
		��Ϊ����ֻ�����ϵ���д���һ�����У���������ֻʹ������0��*/
		//��̫ȷ���Ƿ�Ӧ��ʹ���������ʹ��ǰ����̵涼����Ч�ģ�������ʱ��ΪӦ�ý���ʹ��
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);

		/*�����������ͬ����ô����ֻ��Ҫ����������һ�Ρ�������һ�������Լ������о����*/

		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);


	}


	/**
	* ѡ�������豸
	*/
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		/*�����0̨�豸֧��Vulkan����ô��û�б�Ҫ�ٽ�һ���ˡ�*/
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		/*�����������ڿ��Է���һ����������������VkPhysicalDevice���֡�*/

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		/*����������Ҫ����ÿһ��������������Ƿ��ʺ�������Ҫִ�еĲ�������Ϊ��������ͼ�ο�����ƽ�ȴ����ġ�
			Ϊ�ˣ����ǽ�����һ���º�����*/
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
		//ʹ������ӳ��ͨ�����ӷ����Զ��Ժ�ѡ�˽�������
		std::multimap<int, VkPhysicalDevice> candidates;
		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			std::cout << score << std::endl;
			candidates.insert(std::make_pair(score, device));
		}
		// Check if the best candidate is suitable at all
		//�����Ѻ�ѡ���Ƿ����
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
		//��ɢGPU������������������
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}
		// Maximum possible size of textures affects graphics quality
		//����������ܴ�С��Ӱ��ͼ������

		score += deviceProperties.limits.maxImageDimension2D;

		std::cout << deviceProperties.limits.maxImageDimension2D << std::endl;

		// Application can't function without geometry shaders
		//û�м�������ɫ����Ӧ�ó����޷�����
		if (!deviceFeatures.geometryShader) {
			return 0;
		}
		return score;
	}




	/*������������������صĶ�������ҹ��ܣ����ǿ���ʹ������ΪISDeviceSuite�����еļ�飬
��ȷ���豸�ܹ�����������Ҫʹ�õ����
	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}*/


	void mainLoop() {
		//ѭ���¼�  
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	/**
	* ����֡
	* ��ҪcommandBudfder�������
	* imageAvailableSemaphore  �ñ�ʾ��Ⱦ׼����������Ի�ȡ��Ⱦ׼�����
	* ������ swapChain
	* renderFinishedSemaphore ������ʾ��Ⱦ������
	* graphicsQueue ͼ������
	*
	*/
	void drawFrame() {

		uint32_t imageIndex;

		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);//���ͼ��index
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		/*ǰ��������ָ����ִ�п�ʼ֮ǰ�ȴ���Щ�ź������Լ��ڹܵ����ĸ��׶εȴ�������ϣ���ȴ�����ɫд��ͼ��ֱ�������ã�
		�������ָ����д����ɫ������ͼ�ιܵ��Ľ׶Ρ�����ζ�������ϣ�ʵ�ֿ��Կ�ʼִ�����ǵĶ�����ɫ������ͼ�񻹲����á�
		waitStages�����е�ÿ����Ŀ��Ӧ��pWaitSemaphores�о�����ͬ�������ź�����*/
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		/*����������������ָ��ʵ���ύִ�е������������ǰ����������Ӧ���ύ���������
		�û����������Ǹո���Ϊ��ɫ������ȡ�Ľ�����ͼ��*/
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		/*signalSemaphoreCount��pSignalSemaphores����ָ��������������ִ�к����źŵ��ź����������ǵ������У�����ʹ��renderFinishedSemaphore��ʵ�����Ŀ�ġ�*/
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		/*�ύ��Ⱦ����*/
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

		result = vkQueuePresentKHR(presentQueue, &presentInfo); //��Ⱦ��ʾ����
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		vkQueueWaitIdle(presentQueue);//�ȴ���Ⱦ�������  �����������֤�������»������֤��������ڴ�й©

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
		/*VkinInstanceֻӦ���ڳ����˳�֮ǰ���١�������ʹ��vkDestroyInstance��������������б����٣�*/
		vkDestroyInstance(instance, nullptr);
		/*vkDestroyInstance�����Ĳ����ǳ��򵥡�����ǰһ��������
		�еķ���ͽ�����亯��Vulkan��һ����ѡ�ķ������ص������ǽ�ͨ������nullptr������ȥ�ɡ�
		���ǽ��������½��д�������������Vulkan��ԴӦ����ʵ��������֮ǰ��������
		�ڴ���ʵ��֮�����ִ�и����ӵĲ���֮ǰ����ʱ����ͨ�������֤�����������ǵĵ���ѡ�*/


		//��ֹglfw����
		glfwDestroyWindow(window);
		glfwTerminate();

	}

	//�������ڴ�С
	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);

	}



	//void initWindow() {

	//	glfwInit();

	//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//�������� ����ҪҪ����API

	//	//��Ϊ���������С�Ĵ�����Ҫ�ر�С�ģ����ǽ��ں�������о���
	//	//ͨ����һ��������ʾ������ʱ��������
	//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//	//����ʣ�µľ��Ǵ���ʵ�ʵĴ��ڡ����һ��GLFWwindow*���ڣ�
	//	//private���Ա�洢���������ã���ʹ�����������ʼ�����ڣ�
	//	//window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);//@���@�߶�@����@ָ����������@OpenGL����


	//	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	//}


	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;
		HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain();

	}


	/**
	* ���� VkInstance ʵ��
	*/

	void createInstance() {

		//���ͼ���Ƿ����
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}


		//���ڣ�Ҫ����һ��ʵ�����������ȱ�����һЩ��Ϣ���һ���ṹ
		//�������ǵ����롣�������ڼ������ǿ�ѡ�ģ����������ṩ
		//	Ϊ��ʻԱ�ṩһЩ���õ���Ϣ���Ա�������ǵ��ض�Ӧ�ý����Ż���
		//	���磬��Ϊ��ʹ����һ��������ͼ�����棬����ĳЩ����Ĺ���
		//	��Ϊ�˽ṹ��ΪVkApplicationInfo��

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;//��ǰ������Vulkan�е����ṹ��Ҫ������ʽָ��sType��Ա�е����͡�
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		/*Vulkan�еĺܶ���Ϣ����ͨ���ṹ�����Ǻ����������ݵģ����Ǳ��������һ���ṹ���ṩ�㹻����Ϣ
		���ڴ���ʵ������һ���ṹ���ǿ�ѡ�ģ�������Vulkan����������Ҫʹ�õ�ȫ����չ����֤�㡣������ȫ��
		����ζ�������������������򣬶������ض����豸	���ڽ������ļ����б������*/
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		/*ǰ���������ܼ򵥡�������������ָ��������ȫ����չ�������һ��������Vulkan��һ��
		ƽ̨�޹�API������ζ������Ҫ�Խӿڽ�����չʹ�ô���ϵͳ��GLFW��һ����������ú���glfwGetRequiredInstanceExtensions��
		���Է�����չ����Ҫִ�����ǿ��Դ��ݸ��ṹ�Ĳ�����*/
		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//createInfo.enabledExtensionCount = glfwExtensionCount;

		//createInfo.ppEnabledExtensionNames = glfwExtensions;
		/*�ṹ�����������Աȷ��Ҫִ�е�ȫ����֤��ʹ�������ǽ�����һ�¸������������Щ���������뿪
		��Щ��ʱ�ǿյġ�*/

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


		/*���������Ѿ�ָ����Vulkan����ʵ�������һ�У����ǿ�����󷢳�vkCreateInstance���ã�*/

		//VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		/*�������������ģ����󴴽������������ڵ�һ��ģʽ�����ǣ�
			- ָ�����������Ϣ�Ľṹ��ָ��
			- ָ���Զ���������ص���ָ�룬�ڱ��̳���ʼ��Ϊnullptr
			- ָ��洢�¶������ı�����ָ��
		���һ��˳������ôʵ���ľ���洢��VkInstance���Ա��
		��������Vulkan��������������Ϊ��ֵVkResult����ʾVK_�ɹ��������롣
		���ʵ���ѳɹ����������ǲ���Ҫ�洢�����ֻ��ʹ�ø�Ϊ���ɹ�ֵ��*/
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

		showSupport();


	}

	/*�ṩ�й�Vulkan֧�ֵ�һЩ��ϸ��Ϣ��(��ѡ)*/

	void showSupport() {

		/*������鿴vkCreateInstance�ĵ����ͻῴ�����ܵĴ�������ǡ�VK_ERROR_EXTENSION_NOT_PRESENT����
		���ǿ��Լ򵥵�	ָ��������Ҫ����չ�����ڴ�����뷵��ʱ��ֹ��
		�����Windowsϵͳ���������Ļ�����չ��˵��������ģ�����������������ѡ�����أ�

		Ҫ�ڴ���ʵ��֮ǰ������֧����չ���б���һ��vkEnumerateInstanceExtensionProperties������
		����Ҫһ��ʱ��	ָ��һ��������ָ�룬�ñ����洢��չ��������չ������VkExtensionProperties���洢��չ����ϸ��Ϣ��
		��Ҳ��Ҫһ��ʱ���ѡ�ĵ�һ����������������ͨ���ض�����֤ɸѡ��չͼ�㣬������ʱ��������

		Ҫ����һ��������������չϸ�ڣ�����������Ҫ֪����η����кܶࡣ������ͨ���뿪��һ������Ϊ�գ�*/

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		/*������ǿ��Բ�ѯ��չϸ�ڣ�*/
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		/*ÿ��VkExtensionProperties�ṹ��������չ�����ƺͰ汾�����ǿ�����һ���򵥵�forѭ����\t������ѡ����г����ǣ�*/
		std::cout << "available extensions:" << std::endl;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}

	}


	//��֤
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
	/*�����Ƿ�������֤�㷵���������չ�б�*/
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
	/*���һ���µľ�̬��Աʹ��PFN_vkDebugReportCallbackEXT pro totype����debugCallback������
	VKAPI_ATTR��VKAPI_����ȷ������������ȷ��ǩ����Vulkan��֮Ϊ*/
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

		cleanupSwapChain();//ȷ������֮ǰ����֮ǰ�ľɰ汾

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}




	/**
	* ��װdebug�ص�
	* ��Ҫinstance ���callback
	*/
	void setupDebugCallback() {
		/*������Ҫ��дһ�������ص�ϸ�ڵĽṹ*/
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

