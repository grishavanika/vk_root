// https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Instance
// https://vulkan-tutorial.com/code/01_instance_creation.cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <print>
#include <vector>

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <cassert>

#define KK_VERIFY(X) assert((X))
#define KK_VERIFY_VK(X) KK_VERIFY(VK_SUCCESS == (X))

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
    GLFWwindow* _window = nullptr;
    VkInstance _instance = nullptr;

    void initWindow()
    {
        KK_VERIFY(glfwInit());

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        KK_VERIFY(_window);
    }

    void initVulkan()
    {
        createInstance();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(_window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyInstance(_instance, nullptr);

        glfwDestroyWindow(_window);

        glfwTerminate();
    }

    void createInstance()
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = ::glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        KK_VERIFY(glfw_extensions);

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;
        create_info.enabledLayerCount = 0;

        KK_VERIFY_VK(::vkCreateInstance(&create_info, nullptr, &_instance));

        uint32_t extension_count = 0;
        KK_VERIFY_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
        std::vector<VkExtensionProperties> extensions;
        extensions.resize(extension_count);
        KK_VERIFY_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()));
        std::println("Supported extensions:");
        for (const VkExtensionProperties& p : extensions)
        {
            std::println(" -- '{}'", p.extensionName);
        }
    }
};

int main()
{
    HelloTriangleApplication app;
    app.run();
}
