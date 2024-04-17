#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
    VkInstance instance;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void createInstance(){
        //创建实例

        //填充struct，用于描述信息
        //vk中很多struct会被用于当作函数的参数进行传递
        //VkApplicationInfo结构体是可选的
        VkApplicationInfo appInfo{};
        //vk有很多结构体都需要显式地指明sType
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "Kutory Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        //该部分不是可选的
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        //To retrieve（检索） a list of supported extensions before creating an instance
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,nullptr);
        //Allocate an array to hold the extension details
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,extensions.data());

        //Output extensions
        std::cout<<"Pay Attention!!! Here are avaulable extensions:\n";
        for(const auto& extension : extensions){
            std::cout << '\t' << extension.extensionName << '\n';
        }

        //Specify the desired global extensions
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        //determine the global validation layers to enable.
        createInfo.enabledLayerCount = 0;

        //instance设置完毕
        //三个参数：创建信息的指针；指向自定义分配器回调的指针；指向存储新对象句柄的变量的指针
        //几乎所有的Vulkan函数都返回一个类型VkResult，要么是VK_SUCCESS要么是错误代码的值
        if(vkCreateInstance(&createInfo, nullptr, &instance)!=VK_SUCCESS){
            throw std::runtime_error("Holy Shit!!! Faild to create instance!");
        }
    }
    void initVulkan() {
        //第一步，创建Instance
        createInstance();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }        
    }

    void cleanup() {
        //Destroyed
        vkDestroyInstance(instance,nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
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