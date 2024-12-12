// https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Window_surface
// https://vulkan-tutorial.com/code/05_window_surface.cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
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

const char* const kRequiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};

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

    void initWindow()
    {
        KK_VERIFY(glfwInit());
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        KK_VERIFY(window);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
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
        createInfo.enabledExtensionCount = 0;

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

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        const QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
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
            auto it = std::ranges::find(available_layers, required_layer_name, &VkLayerProperties::layerName);
            if (it == std::ranges::end(available_layers))
            {
                return false;
            }
        }

        return true;
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
