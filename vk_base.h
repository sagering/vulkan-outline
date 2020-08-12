#pragma once

// clang-format off
#include <vulkan\vulkan.h>
#include <GLFW\glfw3.h>
// clang-format on

#include <vector>

struct VulkanBase
{
  struct VulkanWindow
  {
    virtual VkSurfaceKHR CreateSurface(VkInstance instance) = 0;
    virtual VkExtent2D GetExtent() = 0;
  };

  VulkanWindow* window = nullptr;

  VkInstance instance = VK_NULL_HANDLE;
  std::vector<const char*> instanceLayers;
  std::vector<const char*> instanceExtensions;

  VkSurfaceKHR surface = VK_NULL_HANDLE;

  struct PhysicalDeviceProps
  {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties props = {};
    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDeviceMemoryProperties memProps = {};
    std::vector<VkQueueFamilyProperties> queueFamilyProps = {};
    std::vector<VkSurfaceFormatKHR> surfaceFormats = {};
    std::vector<VkPresentModeKHR> presentModes = {};

    PhysicalDeviceProps() = default;
    PhysicalDeviceProps(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    uint32_t GetGrahicsQueueFamiliyIdx();
    uint32_t GetPresentQueueFamiliyIdx();

    // Surface capabilities are not static, e.g. currentExtent might change.
    VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();

    bool HasGraphicsSupport();
    bool HasPresentSupport();
  };

  std::vector<const char*> deviceExtensions;
  VkDevice device;
  PhysicalDeviceProps physicalDeviceProps;
  VkQueue queue;
  VkCommandPool cmdPool;

  struct Swapchain
  {
    VkDevice device = VK_NULL_HANDLE;
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surfaceFormat = {};
    VkPresentModeKHR presentMode = {};
    VkExtent2D imageExtent = {};
    uint32_t imageCount = {};
    std::vector<VkImage> images = {};
    std::vector<VkImageView> imageViews = {};

    Swapchain(VkDevice device,
              PhysicalDeviceProps physicalDeviceProps,
              VkSurfaceKHR surface);

    Swapchain() = delete;
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain& other) = delete;

    ~Swapchain();
  };

  Swapchain* swapchain;

  VkImage depthImage = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = {};

  VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
  VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

  VkRenderPass renderPass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers = {};

  std::vector<VkCommandBuffer> commandBuffers = {};
  std::vector<VkFence> fences = {};

  // --------------------------------------------------------------------------
  // --------------------------------------------------------------------------

  VulkanBase(VulkanWindow* window);
  ~VulkanBase();
  void Update();
  virtual void OnSwapchainReinitialized() = 0;

private:
  void ReinitSwapchain();

  void CreateSwapchainIndependentResources();
  void DestroySwapchainIndependentResources();
  void CreateSwapchainDependentResources();
  void DestroySwapchainDependentResources();
};
