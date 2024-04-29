#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// 这里存储了当前vulkan程序要启用的validation层
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// validation层仅在Debug模式下作用
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

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
    //类成员
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    // 将显卡存储到VkPhysicalDevice句柄中，且随着VkInstance的销毁而销毁
    VkSurfaceKHR surface;//创建Window surface
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;//LogicDevice
    VkQueue graphicsQueue;//Queue(Graphics)
    VkQueue presentQueue;//Queue(Presentation)
    //Store the VkSwapchainKHR;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    struct QueueFamilyIndices
    {
        // std::optional为C++17标准引入的，可以通过.has_value()来判定是否赋值
        // 因为graphicsFamily为任何值都可能有效，甚至是0，如果使用uint32_t来做类型的话，无法判断是否有效
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        //检查SwapChain是否可用是不够的，还需要检查一下三种属性
        /*
        1.Basic surface capabilities(min/max number of images in swap chain, min/max width and height of images)
        2.Surface formats(pixel format, color space)
        3.Available presentation modes
        */
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void createInstance()
    {
        // 检查Layer层的支持情况
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("=====Validation layers requested, but not available!=====");
        }

        // 创建实例

        // 填充struct，用于描述信息
        // vk中很多struct会被用于当作函数的参数进行传递
        // VkApplicationInfo结构体是可选的
        VkApplicationInfo appInfo{};
        // vk有很多结构体都需要显式地指明sType
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Kutory Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // 该部分不是可选的
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Specify the desired global extensions
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // 如果开启Validation调试，将validation层的信息（开启层数、开启扩展的名称）记录到VkInstanceCreateInfo中
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // instance设置完毕
        // 三个参数：创建信息的指针；指向自定义分配器回调的指针；指向存储新对象句柄的变量的指针
        // 几乎所有的Vulkan函数都返回一个类型VkResult，要么是VK_SUCCESS要么是错误代码的值
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("=====Faild to create instance!=====");
        }
    }

    bool checkValidationLayerSupport()
    {
        // 列出所有可用的Layers
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::cout << '\n'
                  << "Here are all avalilable layers:" << '\n';
        for (const auto &layerProperties : availableLayers)
        {
            // 输出所有的availableLayers的layerName
            std::cout << '\t' << layerProperties.layerName << '\n';
        }
        // 检查availableLayers列表中是否存在所有validationLayers图层
        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    // 根据是否启用验证层返回所需的扩展列表
    std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            // set up a debug messenger
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // To retrieve（检索） a list of supported extensions before creating an instance
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        // Allocate an array to hold the extension details
        std::vector<VkExtensionProperties> vkextensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkextensions.data());

        // Output extensions
        std::cout << "Here are all available instance extensions:\n";
        for (const auto &extension : vkextensions)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        return extensions;
    }

    // Message callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            // Message is important enough to show
        }
        return VK_FALSE;
    }

    void initVulkan()
    {
        // 第一步，创建Instance
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("=====Failed to set up debug messenger!=====");
        }
    }

    void createSurface()
    {
        //Use platform specific extension
        /*include information:
        #define VK_USE_PLATFORM_WIN32_KHR
        #define GLFW_INCLUDE_VULKAN
        #include <GLFW/glfw3.h>
        #define GLFW_EXPOSE_NATIVE_WIN32
        #include <GLFW/glfw3native.h>

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        */


        //使用GLFW
        if(glfwCreateWindowSurface(instance, window,nullptr,&surface)!= VK_SUCCESS){
            throw std::runtime_error("=====Failed to create window surface!=====");
        }
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("=====No GPUs with Vulkan support!=====");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // 调用设备检查函数来检查GPUs的可用性
        for (const auto &device : devices)
        {
            if (isDeviceSuitable(device))
            {
                // 找第一个可以用的设备
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("=====No suitable GPU!=====");
        }

        // 使用ordered map来对候选设备进行自动排序
        //  std::multimap<int, VkPhysicalDevice> candidates;
        //  for(const auto& device : devices) {
        //      int score = rateDeviceSuitability(device);
        //      candidates.insert(std::make_pair(score, device));
        //  }
        //  if(candidates.rbegin()->first > 0){
        //      physicalDevice = candidates.rbegin()->second;
        //  }else {
        //      throw std::runtime_error("No suitable GPU!");
        //  }
    }

    // 检查当前的设备能否支持Vulkan的功能
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        // 查询基本设备属性（名称、类型、Vulkan支持版本等）
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // 查询设备特性（纹理压缩、64位浮点数、Multiviewport渲染等可选功能）
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // 以下代码示例为选择仅适用与支持几何着色器显卡
        // return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        // deviceFeatures.geometryShader;

        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionSupported = checkDeviceExtensionSupport(device);

        //验证SwapChain支持是否足够
        bool swapChainAdequate = false;
        //先验证extension可用
        if(extensionSupported){
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            //本例只需要至少一种支持的Format和一种支持的PresentMode即可
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        
        return indices.isComplete() && extensionSupported && swapChainAdequate ;
    }

    //Additional check 检查设备的Swap chain extensions
    bool checkDeviceExtensionSupport(VkPhysicalDevice device){
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,availableExtensions.data());
        
        std::set<std::string> requiredExtensions(deviceExtensions.begin(),deviceExtensions.end());

        std::cout << '\n'
                  << "Here are all avalilable device extensions:" << '\n';
        for(const auto& extension : availableExtensions){
            // 输出所有的availableExtensionsextensionName
            std::cout << '\t' << extension.extensionName << '\n';
            requiredExtensions.erase(extension.extensionName);
        }
        
        return requiredExtensions.empty();
    }

    // 给物理设备打分以选取最适合的设备
    int rateDeviceSuitability(VkPhysicalDevice device)
    {
        int score = 0;

        // 查询基本设备属性（名称、类型、Vulkan支持版本等）
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        // 查询设备特性（纹理压缩、64位浮点数、Multiviewport渲染等可选功能）
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Discrete GPUs（独立显卡）拥有更高的性能，+1000分！
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        // Textures size作为打分重要依据
        score += deviceProperties.limits.maxImageDimension2D;

        // 一票否决项，不支持几何着色器的GPU直接0分！
        if (!deviceFeatures.geometryShader)
        {
            return 0;
        }

        return score;
    }

    // 检查设备支持哪些Queue families，以及其中哪一个支持我们需要使用的命令
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        // Logic to find graphics queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i ,surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = i;
            }
            if (indices.isComplete())
            {
                break;
            }
            i++;
        }
        return indices;
    }

    //检查SwapChain的各项属性
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        //All of the support querying functions have these two as first parameters
        //VkPhysicalDevice and VkSurfaceKHR
        //Get Capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
        
        //Get supported surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,surface,&formatCount,nullptr);
        if(formatCount!=0){
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device,surface,&formatCount,details.formats.data());
        }

        //Get present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,nullptr);
        if(presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    //Find the right settings for the best possible swapchain
    /*3 settings:
    1.Surface format(color depth)
    2.Presentation mode(conditions for "swapping" images to the screen)
    3.Swap extent(resolution of images in swap chain)
    */

   //SurfaceFormat
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        //指定颜色位深格式、SRGB模式
        for(const auto& availableFormat : availableFormats){
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    //Prsentation Mode
    /*
    VK_PRESENT_MODE_IMMEDIATE_KHR 立即传输
    VK_PRESENT_MODE_FIFO_KHR 队列先进先出，队满等待，垂直同步
    VK_PRESENT_MODE_FIFO_RELAXED_KHR 程序延迟且队列在最后一个vertical blank时不同，不等待立即传输
    VK_PRESENT_MODE_MAILBOX_KHR 队满直接替换新图像，三重缓冲，避免撕裂同时低延迟
    */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for(const auto& availablePresentMode : availablePresentModes){
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
    {
        //通过currentExtent来确认当前分辨率
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
        //一些窗口管理器在currentExtent值为uint32_t的最大值时会选择minImageExtent与maxImageExtent之间最匹配的窗口分辨率
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }


    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),indices.presentFamily.value()};
        
        // 0.0-1.0分配队列优先级
        float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies){
            // 描述了单个Queue family中我们需要的队列数
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        
        // 指定将使用的Device features
        VkPhysicalDeviceFeatures deviceFeatures{};

        // 使用前两个结构以及其他信息来填充VkDeviceCreateInfo主体结构以创建逻辑设备
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //Queue
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        //Features
        createInfo.pEnabledFeatures = &deviceFeatures;
        //Extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //Layers(.enabledLayerCount & .ppEnabledLayerNames are Out of date in new Vulkan)
        if (enableValidationLayers){
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else{
            createInfo.enabledLayerCount = 0;
        }


        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("=====Failed to create logical device!=====");
        }

        //创建Queue handles。参数为逻辑设备、QueueFamily、队列索引、存储句柄的指针
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(),0,&presentQueue);
    }

    //Behind create logic device
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
        //决定在SwapChain中拥有多少个图像
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        //不要超过最大值，0表示没有最大值
        if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount){
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        //Create swap chain,fill in a struct
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        //Unless 3D,always 1
        createInfo.imageArrayLayers = 1;
        //使用swap chain中的图像用作什么操作，下为直接渲染，用作color attachment
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        //指定如何处理在多个queue families中使用swap chain image
        if(indices.graphicsFamily != indices.presentFamily){
            //image可以跨queue families使用，无需显式转移所有权
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }else{
            //image一次由一个queue family所有，需要显示转移所有权才能在另外一个queue family中使用
            //大多数硬件graphics queue family和presentation queue family相同
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        //对swap chain中的image做何种变换。（顺时针旋转90度或者水平翻转）
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        //compositeAlpha字段指定了是否使用A通道与窗口系统中的其他窗口混合，以下为忽略
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        //Create swap chain
        if(vkCreateSwapchainKHR(device, &createInfo,nullptr,&swapChain)!=VK_SUCCESS){
            throw std::runtime_error("=====Failed to create swap chain!=====");
        }

        //Retrieve the handles
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device,swapChain,&imageCount,swapChainImages.data());
    
        //Store as member variables
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
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
        //在销毁设备之前清理Swap chain
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        //销毁设备，销毁时设备队列也被隐式清理
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        // Destroyed
        //先销毁Surface
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //再销毁Innstance
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}