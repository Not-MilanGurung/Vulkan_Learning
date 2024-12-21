#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// For catching and reporting errors
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
// For EXIT_SUCCESS and EXIT_FAILURE macros
#include <cstdlib>

const uint32_t WIDTH = 800; // Defining the width of the GLFW window
const uint32_t HEIGHT = 600; // Defining the height of the GLFW window
const char TITLE[7] = "Vulkan"; // Defining the title of the GLFW window

// Name of the validation layer
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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

        GLFWwindow* window; // GLFW window instance
        VkInstance instance; // Vulkan instance
        VkDebugUtilsMessengerEXT debugMessenger; // Debug callback

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

        void mainLoop() {
            // Loops until the window is closed
            while (!glfwWindowShouldClose(window)) {

                glfwPollEvents(); // Handles all the events in the event queue
            }
        }

        // This function is executed after the mainloop ends
        // It cleans up the instances created 
        void cleanup() {
            // Destroys the debug messenger 
            if (enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }
            // Destroys the vulkan instance
            // Can take a callback pointer else nullptr
            vkDestroyInstance(instance, nullptr);

            glfwDestroyWindow(window); // Destroies the glfw window instance

            glfwTerminate(); // Terminates the glfw library
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

        // Creates the debug messenger for when the vulkan instance is active
        void setupDebugMessenger() {
            if (!enableValidationLayers) return;

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            populateDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug messenger!");
            }
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