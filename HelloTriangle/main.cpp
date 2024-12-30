#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// For catching and reporting errors
#include <iostream>
#include <stdexcept>
#include <vector>   // For creating arrays
#include <map>      // For creating maps
#include <cstring>
#include <optional> // For checking queue family
#include <set> // For creating sets
// For EXIT_SUCCESS and EXIT_FAILURE macros
#include <cstdlib>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp


const uint32_t WIDTH = 800; // Defining the width of the GLFW window
const uint32_t HEIGHT = 600; // Defining the height of the GLFW window
const char TITLE[7] = "Vulkan"; // Defining the title of the GLFW window

// Name of the validation layer
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// Name of extensions required
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME // Swap chain extension 
};

// Disabling validation layers in non-debug mode
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// The two functions below are explicitly called as otherwise they are treated as additional and are not loaded

// Proxy function that loads the debug extenstion
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
// Proxy function that destroys the debug messenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication {
    public:
        // This fuction is used to start the application
        void run() {
            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        }

    private:

        GLFWwindow* window; // GLFW window instance \n Necessary to clean up

        VkInstance instance; // Vulkan instance \n Necessary to clean up
        VkDebugUtilsMessengerEXT debugMessenger; // Debug callback \n Necessary to clean up
        VkSurfaceKHR surface; // Window system integration surface \n Necessary to clean up

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // The physical graphics device (GPU) \n Automatically cleaned up with vkinstance
        VkDevice device; // Logical Device, can have multiple \n Necessary to clean up

        VkQueue graphicsQueue;  // Stores the handle of graphics queue \n Automatically cleaned up
        VkQueue presentQueue;   // Stores the handle of presentation queue \n Automatically cleaned up

        VkSwapchainKHR swapChain; // Handle of the swapchain
        std::vector<VkImage> swapChainImages; // Images stored in the swap chain
        VkFormat swapChainImageFormat; // Image format of the swapchain
        VkExtent2D swapChainExtent; // Extent, size of the image of the swapchain in pixels

        // This defines the parameters of glfw window
        void initWindow() {
            glfwInit(); // Initialise the glfw library

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Telling glfw to not use OpenGL
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Preventing it from resizing
            
            // Create the window with the parameter width, height, title,
            // pointer to monitor for full screen or null for windowed mode,
            // pointer to share with another window or null if not sharing
            window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
        }

        // All the code needed to initialise vulkan 
        void initVulkan() {
            createInstance(); // Creates an instance of vulkan
            setupDebugMessenger(); // Creates the debug messenger
            createSurface(); // Creates the surface to allow vulkan to render on to
            pickPhysicalDevice(); // Picks a graphics card
            createLogicalDevice(); // Creates a logical device
            createSwapChain(); // Creates the swap chain
        }

        void mainLoop() {
            // Loops until the window is closed
            while (!glfwWindowShouldClose(window)) {

                glfwPollEvents(); // Handles all the events in the event queue
            }
        }

        // This function is executed after the mainloop ends
        // It cleans up the instances created 
        // The order of destruction is important
        void cleanup() {
            // Destroys the swap chain
            vkDestroySwapchainKHR(device, swapChain, nullptr);
            // Detroys the logical device
            vkDestroyDevice(device, nullptr);

            // Destroys the debug messenger 
            if (enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }

            // Destroys the surface where vulkan is rendered
            vkDestroySurfaceKHR(instance, surface, nullptr);
            // Destroys the vulkan instance
            // Can take a callback pointer else nullptr
            vkDestroyInstance(instance, nullptr);            

            glfwDestroyWindow(window); // Destroys the glfw window instance

            glfwTerminate(); // Terminates the glfw library
        }

        // Function to create a vulkan instance
        void createInstance() {
            // For validation layers
            if (enableValidationLayers && !checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            // Defining the specifications of the vulkan application instance
            // Optional but can help in optimisation
            VkApplicationInfo appInfo{}; // Instance of vulkan application info used to store specifications
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Hello Triangle"; // Name of the vulkan application
            appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); // Application version
            appInfo.pEngineName = "No Engine"; // Engine name. Not using a engine
            appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); // Engine version
            appInfo.apiVersion = VK_API_VERSION_1_0; // Vulkan api version

            // Defining the specifications of the vulkan instance 
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo; // Giving the application info definet above

            
            auto extensions = getRequiredExtensions();

            // Giving the count and names of the extensions to the instance creation specification
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            // Handles debug messeges for vk instance creation
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            // Enabling validation layers when in  debug mode
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();

                // Creates a debug messenger for vkInstance creation
                populateDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
            } else {
                createInfo.enabledLayerCount = 0;

                createInfo.pNext = nullptr;
            }

            // Creates a vulkan instance with the specification defined above
            // The nullptr represents allocation callback which can be used if created
            // The instance ptr is used to point where to store the instance created
            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
        }

        
        // Gets the required extensions
        // Also get debugging extension when validaion layers are active
        std::vector<const char*> getRequiredExtensions() {

            uint32_t glfwExtensionCount = 0; // Holds to cout of glfw extensions
            const char** glfwExtensions; // Array of the glfw extensions
            // This function returns an array of strings of the name of the extensions used 
            // And also stores the no of extension used to the count variable
            // The array is Null and the count is zero if an error occurs
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            // Adds vk debug when validation layers are enabled
            if (enableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        bool checkValidationLayerSupport() {
            uint32_t layerCount; // Stores the count of layers
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr); // Getting the count of layers

            std::vector<VkLayerProperties> availableLayers(layerCount); // Used to store the properties of the available layers
            // Storing the properties of all the available layers
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); 

            // Looping through the names of all the validation layers
            for (const char* layerName : validationLayers) {
                bool layerFound = false;

                // Looping through the properties of all the available layers
                for (const auto& layerProperties : availableLayers) {
                    // Checks if the name of the validation layers matches the available layer
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }
                // If one of the validation layers was not in the available layers
                if (!layerFound) {
                    return false;
                }
            }

            return true;
        }

        // Creates the debug messenger for when the vulkan instance is active
        void setupDebugMessenger() {
            if (!enableValidationLayers) return;

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            populateDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug messenger!");
            }
        }

        // Defines the specifications of debug messenger 
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // Type of vulkan structure
            // Types of message severity the debugger catches and callbacks
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            // Types of message the debugger catches and callback
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            // Handles the message callback
            // In this case prints the messages defined above
            createInfo.pfnUserCallback = debugCallback; 
        }


        // Defines what to do with the debug messages
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            // The values can be compared with denomination incresing with severity i.e. 
            // message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            //
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 
            //      Diagnostic message
            //
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
            //      Informational message like the creation of a resource
            //
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 
            //      Message about behavior that is not necessarily an error, but very likely a bug in your application
            //
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: 
            //      Message about behavior that is invalid and may cause crashes
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            
            // Can have following values:
            // 
            // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: 
            //      Some event has happened that is unrelated to the specification or performance
            // 
            // VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: 
            //      Something has happened that violates the specification or indicates a possible mistake
            // 
            // VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: 
            //      Potential non-optimal use of Vulkan
            VkDebugUtilsMessageTypeFlagsEXT messageType,    
            
            // Contains the callback struct with the most imporntant members being:
            // 
            // Message: The debug message as a null-terminated string
            // pObjects: Array of Vulkan object handles related to the message
            // objectCount: Number of objects in array
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

        // Creates a surface so vulkan can interact with the window
        // Surface creation is platform dependent and different procedure is required for different os
        // 
        // Here glfw's surface creation is used which is platform agnostic
        void createSurface(){
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface!");
            }
        }

        void pickPhysicalDevice(){
            uint32_t deviceCount = 0; // Variable to store device that support vulkan
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); // Assign the count of graphics cards

            // If no graphics cards that support vulkan are found
            if (deviceCount == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount); // Array to store graphics device handles
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); // Store the GPU devices handles
            
            // Use an ordered map to automatically sort candidates by increasing score
            std::multimap<int, VkPhysicalDevice> candidates;
            // Loop over the devices and store the device and its rating in candidates arrary
            for (const auto& device : devices) {
                int score = rateDeviceSuitability(device);
                candidates.insert(std::make_pair(score, device));
            }

            // Check if the best candidate is suitable at all
            if (candidates.rbegin()->first > 0) {
                physicalDevice = candidates.rbegin()->second;
            } else {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }

        // Checks if the device is suitable for application
        int rateDeviceSuitability(VkPhysicalDevice device) {
            VkPhysicalDeviceProperties deviceProperties; // For storing the device's property
            vkGetPhysicalDeviceProperties(device, &deviceProperties); // Gets the device's property

            // For storing the features(texture compression, 64 bit floats and multi viewport rendering (useful for VR)) 
            // supported by the device
            VkPhysicalDeviceFeatures deviceFeatures; 
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures); // Getting the features supported

            int score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;

            // Application can't function without geometry shaders
            if (!deviceFeatures.geometryShader) {
                return 0;
            }

            // Checking if the device supports the required extensions
            bool extensionsSupported = checkDeviceExtensionSupport(device);
            if (!extensionsSupported) {
                return 0;
            }

            // If the swapchain is compatiable with the surface
            bool swapChainAdequate = false;
            if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }
            // Mark device as unsuitable if swapchain is not compatiable
            if (!swapChainAdequate) {
                return 0;
            }
            // Finding queue family
            QueueFamilyIndices indices = findQueueFamilies(device);
            if (!indices.isComplete()) {
                return 0;
            }

            return score;
        }

        // Stores the index of the queue family
        // the optional uint32_t allows the functionality to check if
        // the variable has a value assigned
        struct QueueFamilyIndices {

            std::optional<uint32_t> graphicsFamily; // Index of the queue family that draws

            std::optional<uint32_t> presentFamily;  // Index of the queue family that presents


            bool isComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        // Takes a graphics device and returns a struct with 
        // indexes of the queue family needed
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
            QueueFamilyIndices indices;
            // Logic to find queue family indices to populate struct with
            uint32_t queueFamilyCount = 0;  // Stores the count of available queue family supported by device
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); // Array to store Queue families
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            
            // Finding a queue families with the required capabilities
            int i = 0; // Index of the current queue family of the physical device
            for (const auto& queueFamily : queueFamilies) {
                // Checks for a queue family for drawing
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }

                // Checks for a queue family for presenting to the surface
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                // Assignes the index when a suitable queue family is found
                if (presentSupport) {
                    indices.presentFamily = i;
                }
                // If the queue families with the requirements is already assigned
                if (indices.isComplete()) {
                    break;
                }

                i++;
            }
            return indices;
        } 

        // Loops through the required extensions's list and checks if the device supports them
        bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount; // Stores the count of extensions supported by the device
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);    // Gets the count from the device

            // Vectors to store the extensions supported by the device
            std::vector<VkExtensionProperties> availableExtensions(extensionCount); 
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()); // Gets the array of extensions

            // Set of the extensions required
            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

            // Erasing the extensions from the requiredExtensions set which are in the availableExtensions vector
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }
            // Returns true if all the required extensions are supported by the device
            // Returns false if the set is not empty i.e. the device doesn't support every require extension 
            return requiredExtensions.empty();
        }

        
        // Creates a logical device using the physical device
        // It also utilises the queue family to be used
        void createLogicalDevice(){
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
            // A vector that stores structs of queue creation info
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{}; // A struct to define queue creation
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // Type of struct
                queueCreateInfo.queueFamilyIndex = queueFamily; // Index of queue family to be created
                queueCreateInfo.queueCount = 1; // No. of queues assigned to the family
                queueCreateInfo.pQueuePriorities = &queuePriority; // Priority of the queue family

                queueCreateInfos.push_back(queueCreateInfo); // Adding the struct to the vector
            }

            VkPhysicalDeviceFeatures deviceFeatures{}; // Features required, empty for now

            // Creating the structure of the logical device to be created
            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // No of queue families
            createInfo.pQueueCreateInfos = queueCreateInfos.data(); // Creation structs of the queue families

            createInfo.pEnabledFeatures = &deviceFeatures;  // Pointing to the features used

            // Setting the extension count and status of validation layers
            // It is ignored by up-to-date implementaions
            // Here for backward compatability

            // Setting the count of extensions and giving the pointer to the array
            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();
            // Checking if in debug mode or not
            if (enableValidationLayers) {
                // Setting the count of layers and giving the pointer to the array
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            // Creating the logical device with the struct above
            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            // Storing the handle of the graphics/drawing queue to index 0
            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
            // Storing the handle of the presentation queue to index 0
            vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        }

        // Struct to check if the device swap chain is suitable with the window surface
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;  // Store the capabilities of the device surface
            std::vector<VkSurfaceFormatKHR> formats; // Store the surface formats supported by the device
            std::vector<VkPresentModeKHR> presentModes;
        };

        void createSwapChain() {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

            // Minimum no. of images in the swap chain for it to function. Plus one for overhead
            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

            // Making sure the imagecount does not exceed the maximum supported by the implementaion
            // Here 0 means there is no limit
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
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

            // There are two ways to handle images that are accessed from multiple queues:
            //
            // VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership must 
            // be explicitly transferred before using it in another queue family. This option offers the best 
            // performance.
            //
            // VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue families without explicit 
            // ownership transfers.

            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }
            // Specifing not to transform image like rotaion or flip
            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            // Ignoring alpha channel
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE; // Enabling clipping of pixel , Better performance  

            createInfo.oldSwapchain = VK_NULL_HANDLE; // Incase the we are replacing an existing swap chain 

            if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }        

            // Assiginging the images to the swap chain
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
            swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

            // Storing format and size for future use
            swapChainImageFormat = surfaceFormat.format;
            swapChainExtent = extent;

        }

        // Checks if swapchian supports the surface
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
            SwapChainSupportDetails details;

            // Getting the capabilities of the device surface
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            uint32_t formatCount; // Store no. of surface format supported 
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr); // Getting the count

            if (formatCount != 0) {
                details.formats.resize(formatCount); // Resizing the vector according to the no. of formats
                // Getting the array of surface format supported
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;  // Store no. of presentation modes
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount); // Resizing the vector to match no. of presentation modes
                // Getting the array of presentation modes and storing in the presentModes vector of details struct
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }
            return details;
        }


        // Seleting the best format supported by the swap chain
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB // Checking for 32-bit per pixel format
                    && 
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR // Checking for sRGB color space support
                    ) { 
                    return availableFormat;
                }
            }
            // Return the first availabe format if no formats match the requirements
            // Can also implement a ranking setup like that of physical device selection
            return availableFormats[0];
        }

        
        /**  
        * Selecting presentation mode
        * 
        * This function selects the best presentation mode available for the swap chain
        * There are four possible presentation modes:
        * 
        *#### VK_PRESENT_MODE_IMMEDIATE_KHR :
        * 
        * Images submitted by your application are transferred to the screen 
        * right away, which may result in tearing.
        *
        *#### VK_PRESENT_MODE_FIFO_KHR :
        * 
        * The swap chain is a queue where the display takes an image from the front 
        * of the queue when the display is refreshed and the program inserts rendered images at the back of 
        * the queue. If the queue is full then the program has to wait. This is most similar to vertical sync 
        * as found in modern games. The moment that the display is refreshed is known as "vertical blank".
        * 
        *#### VK_PRESENT_MODE_FIFO_RELAXED_KHR: 
        * 
        * This mode only differs from the previous one if the application 
        * is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical 
        * blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
        * 
        *#### VK_PRESENT_MODE_MAILBOX_KHR: 
        * 
        * This is another variation of the second mode. Instead of blocking the 
        * application when the queue is full, the images that are already queued are simply replaced with the 
        * newer ones. This mode can be used to render frames as fast as possible while still avoiding tearing, 
        * resulting in fewer latency issues than standard vertical sync. This is commonly known as "triple buffering", 
        * although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.
        */
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // Good if enery usage is not a concern else use VK_PRESENT_MODE_FIFO_RELAXED_KHR
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR; // Garunteed to be always available
        }

        // Chooses the size of frames
        // Uses the capabilities default if it doesn't exceed the limit of implementaion
        // else gets the size of glfw window in pixels and limit it 
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            // If the window size doesn't exceed the maximum allowed by the implementaion
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            } else {
                // Getting the size of the glfw window in pixels
                // The glfw is being initilised with screen coordinates while 
                // vulkan works with pixel
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };
                // Limit the frame size to the capabilities of the implementaion
                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }
    };

int main() {
    HelloTriangleApplication app; // Creates the application instance

    try {
        app.run(); // Running the application
    } catch (const std::exception& e) {
        // Catching errors
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}