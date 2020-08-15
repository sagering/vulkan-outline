#include "vk_base.h"

#include <algorithm>
#include <iostream>

#include "vk_init.h"
#include "vk_utils.h"

VulkanBase::VulkanBase(VulkanWindow* window)
  : window(window)
{
  CreateSwapchainIndependentResources();
  swapchain = new Swapchain(device, physicalDeviceProps, surface);
  CreateSwapchainDependentResources();
}

VulkanBase::~VulkanBase()
{
  DestroySwapchainDependentResources();
  delete swapchain;
  DestroySwapchainIndependentResources();
}

void
VulkanBase::Update()
{
  auto windowExtent = window->GetExtent();

  if (windowExtent.width != swapchain->imageExtent.width ||
      windowExtent.height != swapchain->imageExtent.height) {
    ReinitSwapchain();
  }
}

void
VulkanBase::ReinitSwapchain()
{
  vkDeviceWaitIdle(device);

  delete swapchain;
  swapchain = new Swapchain(device, physicalDeviceProps, surface);

  DestroySwapchainDependentResources();
  CreateSwapchainDependentResources();

  OnSwapchainReinitialized();
}

void
VulkanBase::CreateSwapchainIndependentResources()
{
  // Instance
  instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
  instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  instanceExtensions.push_back("VK_KHR_win32_surface");
  instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);

  std::vector<VkLayerProperties> layerProperties;
  layerProperties.resize(count);
  ASSERT_VK_SUCCESS(
    vkEnumerateInstanceLayerProperties(&count, layerProperties.data()));

  VkApplicationInfo appInfo = vkiApplicationInfo(nullptr, 0, nullptr, 0, 1);

  VkInstanceCreateInfo instInfo =
    vkiInstanceCreateInfo(&appInfo,
                          static_cast<uint32_t>(instanceLayers.size()),
                          instanceLayers.data(),
                          static_cast<uint32_t>(instanceExtensions.size()),
                          instanceExtensions.data());

  ASSERT_VK_SUCCESS(vkCreateInstance(&instInfo, nullptr, &instance));

  // Surface
  surface = window->CreateSurface(instance);

  // Device
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  uint32_t physicalDeviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
  std::vector<VkPhysicalDevice> physicalsDevices(physicalDeviceCount);
  vkEnumeratePhysicalDevices(
    instance, &physicalDeviceCount, physicalsDevices.data());

  for (const auto& dev : physicalsDevices) {
    physicalDeviceProps = PhysicalDeviceProps(dev, surface);
    if (physicalDeviceProps.HasGraphicsSupport() &&
        physicalDeviceProps.HasPresentSupport()) {
      break;
    }
  }

  ASSERT_VK_VALID_HANDLE(physicalDeviceProps.handle);
  ASSERT_TRUE(physicalDeviceProps.GetGrahicsQueueFamiliyIdx() ==
              physicalDeviceProps.GetPresentQueueFamiliyIdx());

  float queuePriority = 1.0f;
  uint32_t queueFamiliyIdx = physicalDeviceProps.GetGrahicsQueueFamiliyIdx();

  VkDeviceQueueCreateInfo queueCreateInfo =
    vkiDeviceQueueCreateInfo(queueFamiliyIdx, 1, &queuePriority);

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.textureCompressionBC = true;
  deviceFeatures.fillModeNonSolid = true;
  deviceFeatures.multiDrawIndirect = true;

  VkDeviceCreateInfo deviceCreateInfo =
    vkiDeviceCreateInfo(1,
                        &queueCreateInfo,
                        0,
                        nullptr,
                        static_cast<uint32_t>(deviceExtensions.size()),
                        deviceExtensions.data(),
                        &deviceFeatures);

  ASSERT_VK_SUCCESS(vkCreateDevice(
    physicalDeviceProps.handle, &deviceCreateInfo, nullptr, &device));

  // Queue
  vkGetDeviceQueue(device, queueFamiliyIdx, 0, &queue);

  // CommandPool
  VkCommandPoolCreateInfo commandPoolCreateInfo =
    vkiCommandPoolCreateInfo(physicalDeviceProps.GetGrahicsQueueFamiliyIdx());

  commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  ASSERT_VK_SUCCESS(
    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &cmdPool));

  // Semaphores
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkiSemaphoreCreateInfo();

  ASSERT_VK_SUCCESS(vkCreateSemaphore(
    device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore));

  ASSERT_VK_SUCCESS(vkCreateSemaphore(
    device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore));
}

void
VulkanBase::DestroySwapchainIndependentResources()
{
  vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
  vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
  vkDestroyCommandPool(device, cmdPool, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void
VulkanBase::CreateSwapchainDependentResources()
{
  // color image / view
  VkImageCreateInfo cImageInfo = vkiImageCreateInfo(
    VK_IMAGE_TYPE_2D,
    swapchain->surfaceFormat.format,
    { swapchain->imageExtent.width, swapchain->imageExtent.height, 1 },
    1,
    1,
    VK_SAMPLE_COUNT_1_BIT,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_SHARING_MODE_EXCLUSIVE,
    VK_QUEUE_FAMILY_IGNORED,
    nullptr,
    VK_IMAGE_LAYOUT_UNDEFINED);

  ASSERT_VK_SUCCESS(vkCreateImage(device, &cImageInfo, nullptr, &colorImage));

  colorImageMemory = vkuAllocateImageMemory(
    device, physicalDeviceProps.memProps, colorImage, true);
  VkImageSubresourceRange cRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  VkImageViewCreateInfo cImageViewInfo =
    vkiImageViewCreateInfo(colorImage,
                           VK_IMAGE_VIEW_TYPE_2D,
                           cImageInfo.format,
                           { VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY },
                           cRange);

  ASSERT_VK_SUCCESS(
    vkCreateImageView(device, &cImageViewInfo, nullptr, &colorImageView));

  auto samplerInfo = vkiSamplerCreateInfo(VK_FILTER_NEAREST,
                                          VK_FILTER_NEAREST,
                                          VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          0.f,
                                          VK_FALSE,
                                          0.f,
                                          VK_FALSE,
                                          VK_COMPARE_OP_NEVER,
                                          0.f,
                                          0.f,
                                          VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                                          VK_FALSE);

  ASSERT_VK_SUCCESS(
    vkCreateSampler(device, &samplerInfo, nullptr, &colorImageSampler));

  // depth image / view
  VkImageCreateInfo dImageInfo = vkiImageCreateInfo(
    VK_IMAGE_TYPE_2D,
    VK_FORMAT_D32_SFLOAT,
    { swapchain->imageExtent.width, swapchain->imageExtent.height, 1 },
    1,
    1,
    VK_SAMPLE_COUNT_1_BIT,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_SHARING_MODE_EXCLUSIVE,
    VK_QUEUE_FAMILY_IGNORED,
    nullptr,
    VK_IMAGE_LAYOUT_UNDEFINED);

  ASSERT_VK_SUCCESS(vkCreateImage(device, &dImageInfo, nullptr, &depthImage));

  depthImageMemory = vkuAllocateImageMemory(
    device, physicalDeviceProps.memProps, depthImage, true);
  VkImageSubresourceRange dRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
  VkImageViewCreateInfo dImageViewInfo =
    vkiImageViewCreateInfo(depthImage,
                           VK_IMAGE_VIEW_TYPE_2D,
                           dImageInfo.format,
                           { VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY },
                           dRange);

  ASSERT_VK_SUCCESS(
    vkCreateImageView(device, &dImageViewInfo, nullptr, &depthImageView));
  // Renderpass 1: main
  // Attachments: 0 color, 1 depth

  std::vector<VkAttachmentDescription> attachmentDescriptions;

  attachmentDescriptions.push_back(
    vkiAttachmentDescription(swapchain->surfaceFormat.format,
                             VK_SAMPLE_COUNT_1_BIT,
                             VK_ATTACHMENT_LOAD_OP_CLEAR,      // clear
                             VK_ATTACHMENT_STORE_OP_STORE,     // store
                             VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // not a stencil
                             VK_ATTACHMENT_STORE_OP_DONT_CARE, // not a stencil
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

  attachmentDescriptions.push_back(vkiAttachmentDescription(
    VK_FORMAT_D32_SFLOAT,
    VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_CLEAR,      // clear
    VK_ATTACHMENT_STORE_OP_DONT_CARE, // store not necessary
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // not a stencil
    VK_ATTACHMENT_STORE_OP_DONT_CARE, // not a stencil
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

  VkAttachmentReference colorAttachmentRef =
    vkiAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkAttachmentReference depthAttachmentRef =
    vkiAttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkSubpassDescription subpassDesc =
    vkiSubpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
                          0,
                          nullptr,
                          1,
                          &colorAttachmentRef,
                          nullptr,
                          &depthAttachmentRef,
                          0,
                          nullptr);

  std::vector<VkSubpassDependency> dependencies;

  dependencies.push_back(vkiSubpassDependency(
    VK_SUBPASS_EXTERNAL,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    {}));

  dependencies.push_back(
    vkiSubpassDependency(VK_SUBPASS_EXTERNAL,
                         0,
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                         {}));

  VkRenderPassCreateInfo renderPassCreateInfo = vkiRenderPassCreateInfo(
    static_cast<uint32_t>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    1,
    &subpassDesc,
    dependencies.size(),
    dependencies.data());

  ASSERT_VK_SUCCESS(
    vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPassPre));

  {
    VkImageView attachments[2] = { colorImageView, depthImageView };
    VkFramebufferCreateInfo createInfo =
      vkiFramebufferCreateInfo(renderPassPre,
                               2,
                               attachments,
                               swapchain->imageExtent.width,
                               swapchain->imageExtent.height,
                               1);
    ASSERT_VK_SUCCESS(
      vkCreateFramebuffer(device, &createInfo, nullptr, &framebufferPre));
  }

  attachmentDescriptions.clear();
  dependencies.clear();

  // Renderpass 2: post process
  // Attachments: 0 swapchain image

  attachmentDescriptions.push_back(
    vkiAttachmentDescription(swapchain->surfaceFormat.format,
                             VK_SAMPLE_COUNT_1_BIT,
                             VK_ATTACHMENT_LOAD_OP_CLEAR,      // clear
                             VK_ATTACHMENT_STORE_OP_STORE,     // store
                             VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // not a stencil
                             VK_ATTACHMENT_STORE_OP_DONT_CARE, // not a stencil
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));

  colorAttachmentRef =
    vkiAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  subpassDesc = vkiSubpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      0,
                                      nullptr,
                                      1,
                                      &colorAttachmentRef,
                                      nullptr,
                                      nullptr,
                                      0,
                                      nullptr);

  dependencies.push_back(vkiSubpassDependency(
    VK_SUBPASS_EXTERNAL,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    {}));

  renderPassCreateInfo = vkiRenderPassCreateInfo(
    static_cast<uint32_t>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    1,
    &subpassDesc,
    dependencies.size(),
    dependencies.data());

  ASSERT_VK_SUCCESS(vkCreateRenderPass(
    device, &renderPassCreateInfo, nullptr, &renderPassPost));

  framebuffersPost.resize(swapchain->imageCount);
  for (uint32_t i = 0; i < swapchain->imageCount; ++i) {
    VkImageView attachments[] = { swapchain->imageViews[i] };
    VkFramebufferCreateInfo createInfo =
      vkiFramebufferCreateInfo(renderPassPost,
                               1,
                               attachments,
                               swapchain->imageExtent.width,
                               swapchain->imageExtent.height,
                               1);

    ASSERT_VK_SUCCESS(
      vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffersPost[i]));
  }

  // CommandBuffers
  commandBuffers.resize(swapchain->imageCount);
  VkCommandBufferAllocateInfo allocateInfo = vkiCommandBufferAllocateInfo(
    cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, swapchain->imageCount);

  ASSERT_VK_SUCCESS(
    vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()));

  // Fences
  VkFenceCreateInfo fenceInfo = vkiFenceCreateInfo();
  fences.resize(swapchain->imageCount);
  for (size_t i = 0; i < fences.size(); ++i) {
    ASSERT_VK_SUCCESS(vkCreateFence(device, &fenceInfo, nullptr, &fences[i]));

    // Put fences in a signalled state.
    VkSubmitInfo submitInfo =
      vkiSubmitInfo(0, nullptr, 0, 0, nullptr, 0, nullptr);

    ASSERT_VK_SUCCESS(vkQueueSubmit(queue, 1, &submitInfo, fences[i]));
  }
}

void
VulkanBase::DestroySwapchainDependentResources()
{
  for (auto fence : fences) {
    vkDestroyFence(device, fence, nullptr);
  }
  vkFreeCommandBuffers(device,
                       cmdPool,
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  for (auto fb : framebuffersPost) {
    vkDestroyFramebuffer(device, fb, nullptr);
  }
  vkDestroyRenderPass(device, renderPassPost, nullptr);
  vkDestroyFramebuffer(device, framebufferPre, nullptr);
  vkDestroyRenderPass(device, renderPassPre, nullptr);
  vkDestroyImageView(device, depthImageView, nullptr);
  vkDestroyImage(device, depthImage, nullptr);
  vkFreeMemory(device, depthImageMemory, nullptr);
  vkDestroyImage(device, colorImage, nullptr);
  vkDestroyImageView(device, colorImageView, nullptr);
  vkFreeMemory(device, colorImageMemory, nullptr);
  vkDestroySampler(device, colorImageSampler, nullptr);
}

VulkanBase::Swapchain::Swapchain(VkDevice device,
                                 PhysicalDeviceProps physicalDeviceProps,
                                 VkSurfaceKHR surface)
  : device(device)
{
  auto surfaceCapabilities = physicalDeviceProps.GetSurfaceCapabilities();

  imageCount = surfaceCapabilities.minImageCount + 1;
  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  imageExtent = surfaceCapabilities.currentExtent;

  auto formatIter = std::find_if(
    physicalDeviceProps.surfaceFormats.begin(),
    physicalDeviceProps.surfaceFormats.end(),
    [](const VkSurfaceFormatKHR& format) {
      return format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
             format.format == VK_FORMAT_B8G8R8A8_UNORM;
    });

  ASSERT_TRUE(formatIter != physicalDeviceProps.surfaceFormats.end());
  surfaceFormat = *formatIter;

  std::vector<VkPresentModeKHR> presentModes = { VK_PRESENT_MODE_MAILBOX_KHR,
                                                 VK_PRESENT_MODE_FIFO_KHR };

  auto presentModeIter =
    std::find_first_of(physicalDeviceProps.presentModes.begin(),
                       physicalDeviceProps.presentModes.end(),
                       presentModes.begin(),
                       presentModes.end());

  ASSERT_TRUE(presentModeIter != physicalDeviceProps.presentModes.end());
  presentMode = *presentModeIter;

  auto swapchainCreateInfo =
    vkiSwapchainCreateInfoKHR(surface,
                              imageCount,
                              surfaceFormat.format,
                              surfaceFormat.colorSpace,
                              imageExtent,
                              1,
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                              VK_SHARING_MODE_EXCLUSIVE,
                              VK_QUEUE_FAMILY_IGNORED,
                              nullptr,
                              surfaceCapabilities.currentTransform,
                              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                              presentMode,
                              VK_TRUE,
                              VK_NULL_HANDLE);

  ASSERT_VK_SUCCESS(
    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &handle));

  ASSERT_VK_SUCCESS(
    vkGetSwapchainImagesKHR(device, handle, &imageCount, nullptr));
  images.resize(imageCount);
  ASSERT_VK_SUCCESS(
    vkGetSwapchainImagesKHR(device, handle, &imageCount, images.data()));

  imageViews.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; ++i) {
    auto imageViewCreateInfo =
      vkiImageViewCreateInfo(images[i],
                             VK_IMAGE_VIEW_TYPE_2D,
                             surfaceFormat.format,
                             { VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY },
                             { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    ASSERT_VK_SUCCESS(
      vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageViews[i]));
  }
}

VulkanBase::Swapchain::~Swapchain()
{
  for (auto imageView : imageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, handle, nullptr);
}

VulkanBase::PhysicalDeviceProps::PhysicalDeviceProps(
  VkPhysicalDevice physicalDevice,
  VkSurfaceKHR surface)
  : handle(physicalDevice)
  , surface(surface)
{
  vkGetPhysicalDeviceProperties(handle, &props);
  vkGetPhysicalDeviceFeatures(handle, &features);
  vkGetPhysicalDeviceMemoryProperties(handle, &memProps);

  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, nullptr);
  queueFamilyProps.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    handle, &count, queueFamilyProps.data());

  count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(handle, surface, &count, nullptr);
  surfaceFormats.resize(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    handle, surface, &count, surfaceFormats.data());

  count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(handle, surface, &count, nullptr);
  presentModes.resize(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    handle, surface, &count, presentModes.data());
}

uint32_t
VulkanBase::PhysicalDeviceProps::GetGrahicsQueueFamiliyIdx()
{
  for (uint32_t idx = 0; idx < queueFamilyProps.size(); ++idx) {
    if (queueFamilyProps[idx].queueCount == 0)
      continue;
    if (queueFamilyProps[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      return idx;
  }
  return -1;
}

uint32_t
VulkanBase::PhysicalDeviceProps::GetPresentQueueFamiliyIdx()
{
  for (uint32_t idx = 0; idx < queueFamilyProps.size(); ++idx) {
    if (queueFamilyProps[idx].queueCount == 0)
      continue;
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(handle, idx, surface, &presentSupport);
    if (presentSupport)
      return idx;
  }
  return -1;
}

VkSurfaceCapabilitiesKHR
VulkanBase::PhysicalDeviceProps::GetSurfaceCapabilities()
{
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    handle, surface, &surfaceCapabilities);
  return surfaceCapabilities;
}

bool
VulkanBase::PhysicalDeviceProps::HasGraphicsSupport()
{
  return GetGrahicsQueueFamiliyIdx() != (uint32_t)-1;
}

bool
VulkanBase::PhysicalDeviceProps::HasPresentSupport()
{
  return GetPresentQueueFamiliyIdx() != (uint32_t)-1;
}
