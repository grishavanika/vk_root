// https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets
// https://vulkan-tutorial.com/code/23_descriptor_sets.cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <optional>
#include <print>
#include <set>
#include <vector>

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <cassert>

#define KK_VERIFY(X) assert((X))
#define KK_VERIFY_VK(X) KK_VERIFY(VK_SUCCESS == (X))

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const char* const kRequiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
const char* const kRequiredDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if (0)
const bool kEnableValidationLayers = false;
#else
const bool kEnableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT f =
        PFN_vkCreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (f)
    {
        return f(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT f =
        PFN_vkDestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (f)
    {
        f(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, //
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  //
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},   //
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}   //
};

const uint16_t indices[] = {
    0, 1, 2, //
    2, 3, 0, //
};

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages{};
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    void initWindow()
    {
        KK_VERIFY(glfwInit());
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        KK_VERIFY(window);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        HelloTriangleApplication* app = static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        KK_VERIFY(app);
        app->framebufferResized = true;
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
        KK_VERIFY_VK(vkDeviceWaitIdle(device));
    }

    void cleanupSwapChain()
    {
        for (VkFramebuffer framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        for (VkImageView imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void cleanup()
    {
        cleanupSwapChain();
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        if (kEnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while ((width == 0) || (height == 0))
        {
            // in case close request is there
            if (glfwWindowShouldClose(window))
            {
                break;
            }
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        if (glfwWindowShouldClose(window))
        {
            return;
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void createInstance()
    {
        if (kEnableValidationLayers)
        {
            KK_VERIFY(checkValidationLayerSupport());
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        std::vector<const char*> extensions = getRequiredExtensions();

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (kEnableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(std::size(kRequiredValidationLayers));
            createInfo.ppEnabledLayerNames = std::data(kRequiredValidationLayers);
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        KK_VERIFY_VK(vkCreateInstance(&createInfo, nullptr, &instance));

        uint32_t extension_count = 0;
        KK_VERIFY_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
        std::vector<VkExtensionProperties> available_extensions;
        available_extensions.resize(extension_count);
        KK_VERIFY_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data()));
        std::println("Supported extensions:");
        for (const VkExtensionProperties& p : available_extensions)
        {
            std::println(" -- '{}'", p.extensionName);
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger()
    {
        if (!kEnableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        KK_VERIFY_VK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
    }

    void createSurface()
    {
        KK_VERIFY_VK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        KK_VERIFY_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
        KK_VERIFY(deviceCount > 0);
        std::vector<VkPhysicalDevice> devices;
        devices.resize(deviceCount);
        KK_VERIFY_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

        for (const VkPhysicalDevice& device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }
        KK_VERIFY(physicalDevice != VK_NULL_HANDLE);
    }

    void createLogicalDevice()
    {
        const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(kRequiredDeviceExtensions));
        createInfo.ppEnabledExtensionNames = std::data(kRequiredDeviceExtensions);

        if (kEnableValidationLayers)
        {
            // ignored by up-to-date implementations
            createInfo.enabledLayerCount = static_cast<uint32_t>(std::size(kRequiredValidationLayers));
            createInfo.ppEnabledLayerNames = std::data(kRequiredValidationLayers);
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        KK_VERIFY_VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // createInfo.oldSwapchain = VK_NULL_HANDLE;

        KK_VERIFY_VK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));
        KK_VERIFY_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
        swapChainImages.resize(imageCount);
        KK_VERIFY_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data()));

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews()
    {
        KK_VERIFY(swapChainImages.size() > 0);
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            KK_VERIFY_VK(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]));
        }
    }

    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        KK_VERIFY_VK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        KK_VERIFY_VK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
    }

    void createGraphicsPipeline()
    {
        std::vector<char> vertShaderCode = readFile("shaders/vert_22.spv");
        std::vector<char> fragShaderCode = readFile("shaders/frag_22.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(attributeDescriptions));
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = std::data(attributeDescriptions);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        KK_VERIFY_VK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        KK_VERIFY_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers()
    {
        KK_VERIFY(swapChainImageViews.size() > 0);
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {swapChainImageViews[i]};
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            KK_VERIFY_VK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
        }
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        KK_VERIFY_VK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
    }

    void createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * std::size(vertices);
        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
            stagingBufferMemory);

        void* data = nullptr;
        KK_VERIFY_VK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data));
        memcpy(data, std::data(vertices), size_t(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(uint16_t) * std::size(indices);

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
            stagingBufferMemory);

        void* data = nullptr;
        KK_VERIFY_VK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data));
        memcpy(data, std::data(indices), size_t(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],
                uniformBuffersMemory[i]);

            KK_VERIFY_VK(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]));
        }
    }

    void createDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        KK_VERIFY_VK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    }

    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts;
        layouts.resize(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        KK_VERIFY_VK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        KK_VERIFY_VK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        KK_VERIFY_VK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = nullptr;
        KK_VERIFY_VK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        KK_VERIFY_VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        KK_VERIFY_VK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        KK_VERIFY_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        KK_VERIFY_VK(vkQueueWaitIdle(graphicsQueue));

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) //
                && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        KK_VERIFY(false);
        return uint32_t(-1);
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = uint32_t(commandBuffers.size());
        KK_VERIFY_VK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        KK_VERIFY_VK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = float(swapChainExtent.width);
        viewport.height = float(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
            &descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(std::size(indices)), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        KK_VERIFY_VK(vkEndCommandBuffer(commandBuffer));
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            KK_VERIFY_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
            KK_VERIFY_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
            KK_VERIFY_VK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]));
        }
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj =
            glm::perspective(glm::radians(45.0f), swapChainExtent.width / float(swapChainExtent.height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void drawFrame()
    {
        KK_VERIFY_VK(vkWaitForFences(
            device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX)); // wait for previous vkQueueSubmit

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], // signal when presentation engine is finished using the image
            VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        KK_VERIFY((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR));

        updateUniformBuffer(currentFrame);

        KK_VERIFY_VK(vkResetFences(device, 1, &inFlightFences[currentFrame]));

        KK_VERIFY_VK(vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0));
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // wait with writing colors to the image until it's
                                                            // available (imageAvailableSemaphore)
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        KK_VERIFY_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo,
            inFlightFences[currentFrame] // what to signal when command buffers finish execution
            ));

        VkSwapchainKHR swapChains[] = {swapChain};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // wait for vkQueueSubmit
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) //
            || (result == VK_SUBOPTIMAL_KHR)     //
            || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
        }
        else
        {
            KK_VERIFY(result == VK_SUCCESS);
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        KK_VERIFY_VK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        KK_VERIFY(availableFormats.size() > 0);
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
#if (1)
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
#endif
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        // Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent
        // member. However, some window managers do allow us to differ here and this is indicated by setting the width
        // and height in currentExtent to a special value: the maximum value of uint32_t
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            // match to the resolution of the window
            return capabilities.currentExtent;
        }
        // need to do on our own
        int width_px = 0;
        int height_px = 0;
        // {WIDTH, HEIGHT} in GLFW is in screen coordinates; could be different to size in **pixels**
        glfwGetFramebufferSize(window, &width_px, &height_px);
        KK_VERIFY(width_px >= 0);
        KK_VERIFY(height_px >= 0);
        VkExtent2D actualExtent = {static_cast<uint32_t>(width_px), static_cast<uint32_t>(height_px)};
        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        KK_VERIFY_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities));
        uint32_t formatCount = 0;
        KK_VERIFY_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr));
        KK_VERIFY(formatCount > 0);
        details.formats.resize(formatCount);
        KK_VERIFY_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data()));
        uint32_t presentModeCount = 0;
        KK_VERIFY_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr));
        details.presentModes.resize(presentModeCount);
        KK_VERIFY_VK(
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data()));
        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        const QueueFamilyIndices indices = findQueueFamilies(device);
        const bool extensionsSupported = checkDeviceExtensionSupport(device);
        KK_VERIFY(extensionsSupported);
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        const bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount = 0;
        KK_VERIFY_VK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions;
        availableExtensions.resize(extensionCount);
        KK_VERIFY_VK(
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));

        for (std::string_view required_device_extension : kRequiredDeviceExtensions)
        {
            const auto it = std::ranges::find(
                availableExtensions, required_device_extension, &VkExtensionProperties::extensionName);
            const bool found = (it != std::ranges::end(availableExtensions));
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies;
        queueFamilies.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t i = 0;
        QueueFamilyIndices indices;
        for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }
            VkBool32 presentSupport = false;
            KK_VERIFY_VK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport));
            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }
            ++i;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        KK_VERIFY(glfwExtensions);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (kEnableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layer_count = 0;
        KK_VERIFY_VK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
        std::vector<VkLayerProperties> available_layers;
        available_layers.resize(layer_count);
        KK_VERIFY_VK(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

        std::println("Supported layers:");
        for (const VkLayerProperties& layer : available_layers)
        {
            std::println(" -- '{}'", layer.layerName);
        }

        for (std::string_view required_layer_name : kRequiredValidationLayers)
        {
            const auto it = std::ranges::find(available_layers, required_layer_name, &VkLayerProperties::layerName);
            const bool found = (it != std::ranges::end(available_layers));
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        KK_VERIFY(file.is_open());
        KK_VERIFY(file);
        std::ifstream::pos_type fileSize = file.tellg();
        KK_VERIFY(fileSize != std::ifstream::pos_type(-1));
        KK_VERIFY(fileSize > 0);
        std::vector<char> buffer;
        buffer.resize(std::size_t(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        KK_VERIFY(file);
        file.close();
        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        KK_VERIFY(pCallbackData);
        KK_VERIFY(pCallbackData->pMessage);
        const char* const severity = [&]() {
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            {
                return "verbose";
            }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            {
                return "info";
            }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                return "warning";
            }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                return "error";
            }
            KK_VERIFY(false);
            return "";
        }();
        std::println(stderr, "[VULKAN][{}]: {}", severity, pCallbackData->pMessage);
        return VK_FALSE; // don't abort
    }
};

int main()
{
    HelloTriangleApplication app;
    app.run();
}
