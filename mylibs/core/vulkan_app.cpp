#include "vulkan_app.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "myvulkan/myvulkan.h"
//#include "myvulkan/vulkandevice.h"
//#include "myvulkan/vulkanhelperfuncs.h"
//#include "myvulkan/vulkanresource.h"
//#include "myvulkan/vulkanshader.h"
//#include "myvulkan/vulkanswapchain.h"


#include "math/general_math.h"
#include "math/quaternion.h"

#include <memory.h>

static double timer_frequency = 0.0;

static void error_callback(int error, const char* description)
{
    printf("Error: %s\n", description);
}




static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanApp *data = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    if(data)
    {
        data->resizeWindow(width, height);
        data->needToResize = true;
    }
}

static void keyboardHandlerCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    VulkanApp *data = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    if(data)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose( window, 1 );
            else if (key >= 0 && key < 512)
            {
                data->keyDowns[ key ].isDown = true;
                ++data->keyDowns[ key ].pressCount;

                if (key >= 32 && key < 128)
                {
                    char letter = ( char ) key;
                    if (key >= 65 && key <= 90)
                    {
                        int adder = 32;
                        if (( mods & ( GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK ) ) != 0)
                            adder = 0;
                        letter += adder;
                    }
                    data->bufferedPresses[ data->bufferedPressesCount ] = letter;
                    ++data->bufferedPressesCount;
                }
            }
        }
        else if(action == GLFW_RELEASE && key >= 0 && key < 512)
        {
            data->keyDowns[ key ].isDown = false;
            ++data->keyDowns[ key ].pressCount;
        }
    }
}



bool VulkanApp::init(const char *windowStr, int screenWidth, int screenHeight)
{

    glfwSetErrorCallback(error_callback);
    int rc = glfwInit();
    ASSERT(rc);
    if (!rc)
    {
        printf("Couldn't initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    inited = true;

    window = glfwCreateWindow(screenWidth, screenHeight, windowStr, NULL, NULL);
    ASSERT(window);
    if (!window)
    {
        printf("Couldn't create glfw window\n");
        return false;
    }
    resizeWindow(screenWidth, screenHeight);
/*

    instance = createInstance();
    ASSERT(instance);
    if(!instance)
    {
        printf("Failed to create vulkan instance!\n");
        return false;
    }

    int w,h;
    glfwGetFramebufferSize(window, &w, &h);
    resizeWindow(w, h);

    glfwSetKeyCallback(window, keyboardHandlerCallback);

    debugCallBack = registerDebugCallback(instance);



    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    ASSERT(surface);
    if(!surface)
    {
        printf("Failed to create vulkan surface!\n");
        return false;
    }


    physicalDevice = createPhysicalDevice(instance, surface);
    ASSERT(physicalDevice);
    if(!physicalDevice)
    {
        printf("Failed to create vulkan physical device!\n");
        return false;
    }


    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    ASSERT(props.limits.timestampComputeAndGraphics);
    if(!props.limits.timestampComputeAndGraphics)
    {
        printf("Physical device not supporting compute and graphics!\n");
        return false;
    }

    deviceWithQueues = createDeviceWithQueues(physicalDevice, surface);
    VkDevice device = deviceWithQueues.device;
    ASSERT(device);
    if(!device)
    {
        printf("Failed to create vulkan device!\n");
        return false;
    }

    {
        VkPhysicalDeviceSubgroupProperties subgroupProperties;
        subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        subgroupProperties.pNext = NULL;

        VkPhysicalDeviceProperties2 physicalDeviceProperties;
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties.pNext = &subgroupProperties;

        vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

        printf("subgroup size: %u\n", subgroupProperties.subgroupSize);
        printf("subgroup operations: %u\n", subgroupProperties.supportedOperations);

    }
    renderPass = createRenderPass(device, deviceWithQueues.computeColorFormat, deviceWithQueues.depthFormat);
    ASSERT(renderPass);
    if(!renderPass)
    {
        printf("Failed to create render pass!\n");
        return false;
    }

    [[maybe_unused]] bool scSuccess = createSwapchain(swapchain, window, deviceWithQueues);
    ASSERT(scSuccess);
    if(!scSuccess)
    {
        printf("Failed to create vulkan swapchain!\n");
        return false;
    }

    queryPool = createQueryPool(device, QUERY_COUNT);
    ASSERT(queryPool);
    if(!queryPool)
    {
        printf("Failed to create vulkan query pool!\n");
        return false;
    }

    acquireSemaphore = createSemaphore(device);
    ASSERT(acquireSemaphore);
    if(!acquireSemaphore)
    {
        printf("Failed to create vulkan acquire semapohore!\n");
        return false;
    }

    releaseSemaphore = createSemaphore(device);
    ASSERT(releaseSemaphore);
    if(!releaseSemaphore)
    {
        printf("Failed to create vulkan release semaphore!\n");
        return false;
    }

    fence = createFence(device);
    ASSERT(fence);
    if(!fence)
    {
        printf("Failed to create vulkan fence!\n");
        return false;
    }

    commandPool = createCommandPool(device, deviceWithQueues.queueFamilyIndices.graphicsFamily);
    ASSERT(commandPool);
    if(!commandPool)
    {
        printf("Failed to create vulkan command pool!\n");
        return false;
    }

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));
    if(!commandBuffer)
    {
        printf("Failed to create vulkan command buffer!\n");
        return false;
    }
    deviceWithQueues.mainCommandBuffer = commandBuffer;
    deviceWithQueues.mainCommandPool = commandPool;




    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


        scratchBuffer = createBuffer(device, memoryProperties, 64 * 1024 * 1024,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

    }
    setObjectName(device, (uint64_t)commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");

    // rdoc....
    //glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    //glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        // Create buffers
        renderFrameBuffer = createBuffer(device, memoryProperties, 64u * 1024 * 1024,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Frame render uniform buffer");

    }
*/
    if(!initVulkan(window))
    {
        printf("Failed to initialize vulkan\n");
        return false;
    }

    if (!fontSystem.init("assets/font/new_font.dat"))

    {
        printf("Failed to initialize the font system!\n");
        return false;
    }
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    return true;

}

VulkanApp::~VulkanApp()
{
    fontSystem.deInit();
    deinitVulkan();
    /*
    VkDevice device = deviceWithQueues.device;
    if(device)
    {


        destroyBuffer(device, scratchBuffer);
        destroyBuffer(device, renderFrameBuffer);

        deleteFrameTargets();

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyQueryPool(device, queryPool, nullptr);

        destroySwapchain(swapchain, device);

        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyFence(device, fence, nullptr);
        vkDestroySemaphore(device, acquireSemaphore, nullptr);
        vkDestroySemaphore(device, releaseSemaphore, nullptr);

        vkDestroyDevice(device, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if (enableValidationLayers)
    {
        auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        dest(instance, debugCallBack, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    */

    if(window)
        glfwDestroyWindow(window);
    if(inited)
        glfwTerminate();
    window = nullptr;
}

void VulkanApp::resizeWindow(int w, int h)
{

    windowWidth = w;
    windowHeight = h;
    printf("Window size: %i: %i\n", w, h);
}

void VulkanApp::setVsyncEnabled(bool enable)
{
    vSync = enable;
    // Use v-sync
}
bool VulkanApp::isPressed(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}
bool VulkanApp::isReleased(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}

bool VulkanApp::isDown(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown;
    return false;
}

bool VulkanApp::isUp(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown;
    return false;
}

/*
bool VulkanApp::startRender()
{
    VkDevice device = deviceWithQueues.device;
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    if (acquireSemaphore == VK_NULL_HANDLE)
        return false;
    VkResult res = ( vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &imageIndex) );
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (resizeSwapchain(swapchain, window, deviceWithQueues))
        {
            recreateSwapchainData();
            VK_CHECK(vkDeviceWaitIdle(device));
            needToResize = false;
        }
        return false;
    }
    else if (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR)
        return true;

    VK_CHECK(res);
    return false;
}
*/
uint32_t VulkanApp::updateRenderFrameBuffer()
{
    struct Buff
    {
        Vector2 areaSize;
        float tmp[6 + 8];
    };

    Buff buff{ Vector2(windowWidth, windowHeight) };

    // use scratch buffer to unifrom buffer transfer
    uint32_t buffSize = uint32_t(sizeof(Buff));
    memcpy(vulk.scratchBuffer.data, &buff, buffSize);
    {
        VkBufferCopy region = { 0, 0, VkDeviceSize(buffSize) };
        vkCmdCopyBuffer(vulk.commandBuffer, vulk.scratchBuffer.buffer, vulk.renderFrameBuffer.buffer, 1, &region);
    }

    VkBufferMemoryBarrier bar[]
    {
        bufferBarrier(vulk.renderFrameBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffSize),
    };

    vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, bar, 0, nullptr);

    uint32_t offset = fontSystem.update(Vector2(windowWidth, windowHeight), buffSize);

    return offset;
}


void VulkanApp::present(Image &presentImage)
{
    ::present(window, presentImage);
/*
    VkDevice device = deviceWithQueues.device;
    // Copy final image to swap chain target
    {
        VkImageMemoryBarrier copyBeginBarriers[] =
        {
            imageBarrier(presentImage.image,
                        presentImage.accessMask, presentImage.layout,
                        VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

                    imageBarrier(swapchain.images[ imageIndex ],
                        0, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(copyBeginBarriers), copyBeginBarriers);


        insertDebugRegion(commandBuffer, "Copy to swapchain", Vec4(1.0f, 1.0f, 0.0f, 1.0f));

        VkImageBlit imageBlitRegion = {};

        imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.srcSubresource.layerCount = 1;
        imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.dstSubresource.layerCount = 1;
        imageBlitRegion.srcOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.srcOffsets[ 1 ] = VkOffset3D{ ( i32 ) swapchain.width, ( i32 ) swapchain.height, 1 };
        imageBlitRegion.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.dstOffsets[ 1 ] = VkOffset3D{ ( i32 ) swapchain.width, ( i32 ) swapchain.height, 1 };


        vkCmdBlitImage(commandBuffer, presentImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        swapchain.images[ imageIndex ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
    }

    // Prepare image for presenting.
    {
        VkImageMemoryBarrier presentBarrier = imageBarrier(swapchain.images[ imageIndex ],
                                                            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                            0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    }


    endDebugRegion(commandBuffer);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    // Submit
    {
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkResetFences(device, 1, &fence);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;
        VK_CHECK(vkQueueSubmit(deviceWithQueues.graphicsQueue, 1, &submitInfo, fence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult res = ( vkQueuePresentKHR(deviceWithQueues.presentQueue, &presentInfo) );
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || needToResize)
        {
            needToResize = true;
            if (resizeSwapchain(swapchain, window, deviceWithQueues))
            {
                recreateSwapchainData();
            }
            needToResize = false;
        }
        else
        {
            VK_CHECK(res);
        }
    }

    VK_CHECK(vkDeviceWaitIdle(device));
*/
    for (int i = 0; i < ARRAYSIZE(keyDowns); ++i)
    {
        keyDowns[ i ].pressCount = 0u;
    }
    bufferedPressesCount = 0u;
    dt = timer.getLapDuration();
}
/*
bool VulkanApp::createGraphics()
{
    VkDevice device = deviceWithQueues.device;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    //recreateSwapchainData();

    // create color and depth images
    {
        mainColorRenderTarget =
            createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
                swapchain.width, swapchain.height,
                //deviceWithQueues.computeColorFormat,
                deviceWithQueues.colorFormat,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                //| VK_IMAGE_USAGE_STORAGE_BIT
                , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                "Main color target image");

        mainDepthRenderTarget = createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
            swapchain.width, swapchain.height, deviceWithQueues.depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "Main depth target image");

        targetFB = createFramebuffer(device, renderPass,
            mainColorRenderTarget.imageView, mainDepthRenderTarget.imageView,
            swapchain.width, swapchain.height);
    }
    return true;
}

void VulkanApp::deleteFrameTargets()
{
    VkDevice device = deviceWithQueues.device;

    vkDestroyFramebuffer(device, targetFB, nullptr);
    destroyImage(device, mainColorRenderTarget);
    destroyImage(device, mainDepthRenderTarget);

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
    ASSERT(deviceWithQueues.queueFamilyIndices.isValid());

}

void VulkanApp::recreateSwapchainData()
{
    deleteFrameTargets();
    createGraphics();
    needToResize = false;
}
*/



void VulkanApp::setTitle(const char *str)
{
    glfwSetWindowTitle(window, str);
}

double VulkanApp::getDeltaTime()
{
    return dt;
}

MouseState VulkanApp::getMouseState()
{
    MouseState mouseState;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouseState.x = xpos;
    mouseState.y = ypos;
    mouseState.y = mouseState.y;

    mouseState.leftButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseState.rightButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseState.middleButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    return mouseState;
}




void VulkanApp::checkCameraKeypresses(float deltaTime, Camera &camera)
{
    //printf("deltatime: %f\n", deltaTime);
    float moveBooster = keyDowns[GLFW_KEY_LEFT_SHIFT].isDown ? 5.0f : 1.0f;
    float rotationBooster = keyDowns[GLFW_KEY_LEFT_SHIFT].isDown ? 2.0f : 1.0f;

    float moveSpeed = deltaTime * 2.0f * moveBooster;
    float rotationSpeed = deltaTime * 1.0f * rotationBooster;

    if (keyDowns[GLFW_KEY_I].isDown)
    {
        camera.pitch -= rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_K].isDown)
    {
        camera.pitch += rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_J].isDown)
    {
        camera.yaw -= rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_L].isDown)
    {
        camera.yaw += rotationSpeed;
    }

    camera.pitch = clamp(camera.pitch, -0.499f * pii, 0.4999f * pii);
    camera.yaw = ffmodf(camera.yaw, 2.0f * pii);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    camera.getCameraDirections(rightDir, upDir, forwardDir);


    if (keyDowns[GLFW_KEY_W].isDown)
    {
        camera.position = camera.position - forwardDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_S].isDown)
    {
        camera.position = camera.position + forwardDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_A].isDown)
    {
        camera.position = camera.position - rightDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_D].isDown)
    {
        camera.position = camera.position + rightDir * moveSpeed;

    }
    if (keyDowns[GLFW_KEY_Q].isDown)
    {
        camera.position = camera.position - upDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_E].isDown)
    {
        camera.position = camera.position + upDir * moveSpeed;
    }
}
