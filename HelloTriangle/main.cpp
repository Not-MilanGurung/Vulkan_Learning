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
#include <fstream>  // For reading the SPIR-V byte code

const uint32_t WIDTH = 800; // Defining the width of the GLFW window
const uint32_t HEIGHT = 600; // Defining the height of the GLFW window
const char TITLE[7] = "Vulkan"; // Defining the title of the GLFW window
const int MAX_FRAMES_IN_FLIGHT = 2; // How many frames to process concurrently

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

        GLFWwindow* window; // GLFW window instance /n Necessary to clean up

        VkInstance instance; // Vulkan instance /n Necessary to clean up
        VkDebugUtilsMessengerEXT debugMessenger; // Debug callback \n Necessary to clean up
        VkSurfaceKHR surface; // Window system integration surface \n Necessary to clean up

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // The physical graphics device (GPU) \n Automatically cleaned up with vkinstance
        VkDevice device; // Logical Device, can have multiple /n Necessary to clean up

        VkQueue graphicsQueue;  // Stores the handle of graphics queue \n Automatically cleaned up
        VkQueue presentQueue;   // Stores the handle of presentation queue \n Automatically cleaned up

        VkSwapchainKHR swapChain; // Handle of the swapchain
        std::vector<VkImage> swapChainImages; // Images stored in the swap chain
        VkFormat swapChainImageFormat; // Image format of the swapchain
        VkExtent2D swapChainExtent; // Extent, size of the image of the swapchain in pixels

        std::vector<VkImageView> swapChainImageViews; // Stores Image views. Needs to be cleaned up

        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;

        VkPipeline graphicsPipeline;

        std::vector<VkFramebuffer> swapChainFramebuffers; // Frame buffer

        VkCommandPool commandPool; // Manages the memory allocated to command buffers
        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores; // Semaphores to signal that image has been acquired from swapchain
        std::vector<VkSemaphore> renderFinishedSemaphores; // Semaphores to signal the rendering is done and ready to be presented
        std::vector<VkFence> inFlightFences;  // Making sure new frame is drawn only after the previous frame is finished

        bool framebufferResized = false;

        uint32_t currentFrame = 0;

        // This defines the parameters of glfw window
        void initWindow() {
            glfwInit(); // Initialise the glfw library

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Telling glfw to not use OpenGL
            
            
            // Create the window with the parameter width, height, title,
            // pointer to monitor for full screen or null for windowed mode,
            // pointer to share with another window or null if not sharing
            window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
            // Storing the pointer to this instance of HelloTriangleApplication in the window to call later
            glfwSetWindowUserPointer(window, this); 
            // Callback function is called when the window is resized
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback); 
        }

        // When the window is resized
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            // Recasting using the pointer of the HelloTriangleApplication instance stored in the window in initWindow() function
            auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }

        // All the code needed to initialise vulkan 
        void initVulkan() {
            createInstance(); // Creates an instance of vulkan
            setupDebugMessenger(); // Creates the debug messenger
            createSurface(); // Creates the surface to allow vulkan to render on to
            pickPhysicalDevice(); // Picks a graphics card
            createLogicalDevice(); // Creates a logical device
            createSwapChain(); // Creates the swap chain
            createImageViews();
            createRenderPass();
            createGraphicsPipeline();
            createFramebuffers();
            createCommandPool();
            createCommandBuffer(); // Creates a single command buffer
            createSyncObjects();
        }

        void mainLoop() {
            // Loops until the window is closed
            while (!glfwWindowShouldClose(window)) {

                glfwPollEvents(); // Handles all the events in the event queue

                drawFrame(); // Draws the frame
            }

            vkDeviceWaitIdle(device); // Wait until logical device has finished before cleaning up
        }

        // This function is executed after the mainloop ends
        // It cleans up the instances created 
        // The order of destruction is important
        void cleanup() {

            cleanupSwapChain();

            // Destroys the graphics pipeline
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            // Destroys the pipeline layout
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

            // Destroys the render pass
            vkDestroyRenderPass(device, renderPass, nullptr);

            // Destroys the syncronisation objects
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }

            // Destroys command pool
            vkDestroyCommandPool(device, commandPool, nullptr);
            
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

        // At a high level, rendering a frame in Vulkan consists of a common set of steps:
        // 
        // - Wait for the previous frame to finish
        // - Acquire an image from the swap chain
        // - Record a command buffer which draws the scene onto that image
        // - Submit the recorded command buffer
        // - Present the swap chain image
        void drawFrame() {
            // Wait until previous frame is finished
            vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
            
            // Acquiring the image from the swapchain
            uint32_t imageIndex; // The index of the image available for rendering
            // Defining the logical device, swapchain, timeout in nanoseconds, syncronization object, and where to store the image index
            VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
            // If swap chain is out of date
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapChain();
                return;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            // Reset the fence indicating the start of the frame
            // Only reset fence if we are subbmiting the work
            vkResetFences(device, 1, &inFlightFences[currentFrame]);

            vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
            recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Semaphores to wait on before executing the command
            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
            // Stage to wait on
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            // Command buffers to submint the execution to
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

            // Semaphores to signal to once the command buffer(s) has finished execution
            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, 
                                inFlightFences[currentFrame] /* Signal CPU to wait until the command buffers have finished execution */) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            // Semaphore to wait on before executing
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains; // Swapchain to present the images to
            presentInfo.pImageIndices = &imageIndex; // Indices of the images for each swap chain

            presentInfo.pResults = nullptr; // Optional. Return array of vkResults if all images are presented to the swap chains

            result = vkQueuePresentKHR(presentQueue, &presentInfo);
            // If swap chain is out of date or suboptimal, create a new swapchain and try drawing in the next cycle
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
                framebufferResized = false;
                recreateSwapChain();
            } else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // Advance to next frame in flight and not exceed the max frames in flight
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
        *## Selecting presentation mode
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
        // to specify how many color and depth buffers there will be, how many samples to use for 
        // each of them and how their contents should be handled throughout the rendering operations
        void createRenderPass() {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = swapChainImageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

            // The loadOp and storeOp determine what to do with the data in the attachment before 
            // rendering and after rendering.
            // 
            // We have the following choices for loadOp:
            // 
            // VK_ATTACHMENT_LOAD_OP_LOAD:      Preserve the existing contents of the attachment
            // VK_ATTACHMENT_LOAD_OP_CLEAR:     Clear the values to a constant at the start
            // VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            // There are only two possibilities for the storeOp:
            // 
            // VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
            // VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            // The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            // Some of the most common layouts are:
            // 
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:    Images used as color attachment
            // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:             Images to be presented in the swap chain
            // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:        Images to be used as destination for a memory copy operation
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Don't care what the image layout is before render pass
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // The render pass after render pass

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0; // Referencing the attachment at index 0; the only one 
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Best layout for using attachment as color buffer

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Explicitly using a graphics subpass
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit subpasses before rendering
            dependency.dstSubpass = 0;  // Index of the first subpass, our only subpass

            // Wait for the swap chain to finish reading from the image before we can access it.
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Waiting on the color attachment output stage 
            dependency.srcAccessMask = 0; // Where this can occur

            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1; // No. of attachments
            renderPassInfo.pAttachments = &colorAttachment; // Pointer to array of attachments
            renderPassInfo.subpassCount = 1;    // No. of subpasses
            renderPassInfo.pSubpasses = &subpass;   // Pointer to array of subpasses
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;


            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }

        // Creates image views for the images in the swap chain
        void createImageViews() {
            // Resizing vector to fit all the images in the swap chain
            swapChainImageViews.resize(swapChainImages.size()); 

            // Looping over the images in swap chain and creating their image views
            for (size_t i = 0; i < swapChainImages.size(); i++) {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = swapChainImages[i];

                // Specifying how the image is interperted
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // Using image as 2D texture
                createInfo.format = swapChainImageFormat;       // The format of swap chain image

                // Components allows to swizzle color channels
                // Using the default value right now
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;    // Red channel
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;    // Green channel
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;    // Blue channel
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;    // Alpha channel

                // Used to define the purpose of the image
                // Right now using image as color target without mipmapping
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

        void createGraphicsPipeline() {
            // Reading the byte code and storing them
            auto vertShaderCode = readFile("shaders/vert.spv"); // For storing vertex shader byte code
            auto fragShaderCode = readFile("shaders/frag.spv"); // For storing fragment shader byte code

            // Wrapping the bytecode
            // The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU 
            // doesn't happen until the graphics pipeline is created. That means that we're allowed to destroy 
            // the shader modules again as soon as pipeline creation is finished, which is why we'll make them 
            // local variables in the createGraphicsPipeline function instead of class members
            VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);   
            VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

            // Assigning the shaders to the graphics pipeline stage

            // For vertex shader 
            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Macro for vertex shader stage
            vertShaderStageInfo.module = vertShaderModule;  // Module containing the vertex shader bytecode
            vertShaderStageInfo.pName = "main"; // The function to invoke while running the bytecode
            // pSpecializationInfo is an optional parameter used to specify the shader constants
            // It can be used for modifying shader behaviour at pipeline creation

            // For fragment shader
            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Macro for fragment shader stage
            fragShaderStageInfo.module = fragShaderModule; // Module containing the fragment shader bytecode
            fragShaderStageInfo.pName = "main"; // The function to invoke while running the bytecode

            // Array to store the create info structs for future reference in the creation pipeline
            VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

            // Struct that describes the format of the vertex data to read
            // Since we are using hard coded data, we insert null pointers
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            // Bindings: spacing between data and whether the data is per-vertex or per-instance 
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional. nullptr since the data is hard coded
            
            // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to 
            // load them from and at which offset
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional. nullptr since the data is hard coded


            // struct describes two things: 
            //      what kind of geometry will be drawn from the vertices and
            //      if primitive restart should be enabled
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

            // topology member and can have values like:
            // 
            // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: 
            //      points from vertices
            // 
            // VK_PRIMITIVE_TOPOLOGY_LINE_LIST: 
            //      line from every 2 vertices without reuse
            // 
            // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: 
            //      the end vertex of every line is used as start vertex for the next line
            // 
            // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: 
            //      triangle from every 3 vertices without reuse
            // 
            // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: 
            //      the second and third vertex of every triangle are used as first two vertices of 
            //      the next triangle
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up 
            // lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            /* For static viewport and scissor

             A viewport basically describes the region of the framebuffer that the output will be rendered to
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) swapChainExtent.width;
                viewport.height = (float) swapChainExtent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

             scissor rectangles define in which regions pixels will actually be stored
                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = swapChainExtent;
            */
            // Using dynamic viewport and scissors
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            // Rasterization creation struct
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            /**
             * The polygonMode determines how fragments are generated for geometry. 
             * The following modes are available:
             * 
             * VK_POLYGON_MODE_FILL:    fill the area of the polygon with fragments
             * VK_POLYGON_MODE_LINE:    polygon edges are drawn as lines
             * VK_POLYGON_MODE_POINT:   polygon vertices are drawn as points
             * 
             * Using any other mode than fill requires this feature:
            */
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f; // Thicknes of lines in terms of fragmet shader
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // Optional
            rasterizer.depthBiasClamp = 0.0f; // Optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

            // Multisampling for anitaliasing
            // Requires a GPU feature
            // Disabled for now
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f; // Optional
            multisampling.pSampleMask = nullptr; // Optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
            multisampling.alphaToOneEnable = VK_FALSE; // Optional

            // How the color from rasterisation is blended
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

            // The structure references the array of structures for all of the framebuffers and allows 
            // you to set blend constants that you can use as blend factors in the aforementioned calculations.
            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f; // Optional
            colorBlending.blendConstants[1] = 0.0f; // Optional
            colorBlending.blendConstants[2] = 0.0f; // Optional
            colorBlending.blendConstants[3] = 0.0f; // Optional

            // A limited amout of state can be changed at draw time without recreating the pipeline
            // Examples are the size of the viewport, line width and blend constants
            // The struct below allows us to use these dynamic states
            std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            // Empty pipeline layout for future use
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0; // Optional
            pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            // Referencing the structs of shader stages
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            // Referencing the structs of fixed-functions created above
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = nullptr; // Optional
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            // Referencing the pipeline layout
            pipelineInfo.layout = pipelineLayout;
            // Referencing the renderpass
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0; // Index of subpass where where this graphics pipeline is used

            // Used where deriving pipelines
            // VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo
            // Right now only one pipeline is used so they are not needed
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }

            // Cleaning up the shader modules after creation of pipeline
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
        }

        // Helper function for reading the shader byte code
        static std::vector<char> readFile(const std::string& filename) {
            // ate: Start reading at the end of the file
            // binary: Read the file as binary file (avoid text transformations)
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }
            // Using read position to find the file size as we are reading from the end
            size_t fileSize = (size_t) file.tellg(); 
            std::vector<char> buffer(fileSize);

            file.seekg(0); // Seeking the beginning of the file
            file.read(buffer.data(), fileSize); // Reading the bytes to buffer

            file.close();   // Close the file

            return buffer;  // Return the contents of the file
        }

        /// For wrapping the shader code in a VkShaderModule object
        /// @param code pointer to the buffer with the bytecode and the length of it
        VkShaderModule createShaderModule(const std::vector<char>& code) {

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; // Type of create info
            createInfo.codeSize = code.size(); // Bytecode size in bytes

            // The create info accepts bytecode pointer in uint32_5 instead of char pointer we are providing
            // Hence recasting the pointer into uint32_t
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;    // For storing the shader module created
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }

        void createFramebuffers() {
            // Resizing the framebuffer to hold all the image views
            swapChainFramebuffers.resize(swapChainImageViews.size());

            // Iterating through the image views and creating framebuffers for them
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                VkImageView attachments[] = {
                    swapChainImageViews[i]
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass; // Renderpass to be compatible with
                framebufferInfo.attachmentCount = 1;    // No. of attachment same as render pass
                framebufferInfo.pAttachments = attachments; // Type of attachment same as render pass
                framebufferInfo.width = swapChainExtent.width;
                framebufferInfo.height = swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }

        // Creates the command pool that holds the comman buffers used to perform operations
        void createCommandPool() {
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            // There are two possible flags for command pools:
            // 
            // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often 
            //                                       (may change memory allocation behavior)
            // 
            // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without 
            //                                                  this flag they all have to be reset together
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            // Adding the command pool to the graphics queue as we are drawing
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(); 

            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }
        }

        void createCommandBuffer() {
            commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;    // Reference to the command pool managing the command buffer

            // The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
            // 
            // VK_COMMAND_BUFFER_LEVEL_PRIMARY:     Can be submitted to a queue for execution, but cannot be called from other command buffers.
            // VK_COMMAND_BUFFER_LEVEL_SECONDARY:   Cannot be submitted directly, but can be called from primary command buffers.
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();   // No. of command buffers

            if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        /// Writes the commands we want to execute into a command buffer
        /// @param commandBuffer The command buffer to write the commands into
        /// @param imageIndex The index of the current swapchain image we want to write to
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
            // Begin recording the command buffer
            // If already recording, resets the recording operation
            // It is not possible to append command buffers
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            // The flags parameter specifies how we're going to use the command buffer. The following values are available:
            // 
            // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT:         The command buffer will be rerecorded right after executing it once.
            // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT:    This is a secondary command buffer that will be entirely within a single 
            //                                                      render pass.
            // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT:        The command buffer can be resubmitted while it is also already pending execution.
            beginInfo.flags = 0; // Not using any flags right now

            // The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which 
            // state to inherit from the calling primary command buffers.
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            // Starting a render pass
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass; // Reference to the render pass
            renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // Binding a framebuffer for the image view

            // Size of the render area
            // Should match the attachment for best performance
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            // Clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // Black with 100% opacity
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            // Record the begin render pass command
            // The final parameter controls how the drawing commands within the render pass will be provided. 
            // It can have one of two values:
            // 
            // VK_SUBPASS_CONTENTS_INLINE: 
            //              The render pass commands will be embedded in the primary command buffer itself and 
            //              no secondary command buffers will be executed.
            // 
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: 
            //              The render pass commands will be executed from secondary command buffers.
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Record command to bind graphics pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            // Specifying the viewport as we are using dynamic viewport
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            // Specifyng the scissor as we are using dynamic scissor
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            /**
             * Recording the command to draw
             * =============================
             * It has the following parameters, aside from the command buffer:
             * 
             * vertexCount:     Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
             * instanceCount:   Used for instanced rendering, use 1 if you're not doing that.
             * firstVertex:     Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
             * firstInstance:   Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex
            */ 
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);  // End the render pass

            // Stop recording the commands
            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        // Creates objects used for syncronisation
        void createSyncObjects() {
            // Resizeing the vectors to accoring to max frames in flight
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            // Flagging the fence to start as signaled to allow the first frame to render
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

            // Creating syncronisation objects for each frame in flight
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS){
                    throw std::runtime_error("failed to create semaphores!");
                }
                if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create fence!");
                }
            }
        }

        // Clean up the incompatable swapchain
        // Also cleans up frame buffers and imageviews as they are reliant on the swap chain
        void cleanupSwapChain() {
            for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
                vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
            }

            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                vkDestroyImageView(device, swapChainImageViews[i], nullptr);
            }

            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }
        // Recreating swapchain when it is no longer compatiable with the surface
        // For example when resizing the window
        void recreateSwapChain() {
            // Ideling the recreation when the window is minimized
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height); // Gets the size of the window
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window, &width, &height); // Gets the size of the window
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);

            cleanupSwapChain();

            createSwapChain();
            createImageViews();
            createFramebuffers();
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