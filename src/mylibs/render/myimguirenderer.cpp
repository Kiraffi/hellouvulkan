#include "myimguirenderer.h"

#include <container/podvector.h>

#include <core/vulkan_app.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanglobal.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

//#include <string.h>
#include <stdlib.h>         // abort

static void check_vk_result(VkResult err)
{
    VK_CHECK(err);
    if (err == 0)
        return;
    printf("[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}


MyImguiRenderer::~MyImguiRenderer()
{
    //if(renderPass && descriptorPool && frameBuffer)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    if(renderPass)
        vkDestroyRenderPass(vulk->device, renderPass, nullptr);
    if(descriptorPool)
        vkDestroyDescriptorPool(vulk->device, descriptorPool, nullptr);
    if(frameBuffer)
        vkDestroyFramebuffer(vulk->device, frameBuffer, nullptr);

    renderPass = VK_NULL_HANDLE;
    descriptorPool = VK_NULL_HANDLE;
    frameBuffer = VK_NULL_HANDLE;
}

bool MyImguiRenderer::init(GLFWwindow *window)
{

    renderPass =
        createRenderPass({ RenderTarget{.format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE } }, {});

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VkResult err = vkCreateDescriptorPool(vulk->device, &pool_info, nullptr, &descriptorPool);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();


    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulk->instance;
    init_info.PhysicalDevice = vulk->physicalDevice;
    init_info.Device = vulk->device;
    init_info.QueueFamily = vulk->queueFamilyIndices.graphicsFamily;
    init_info.Queue = vulk->graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = vulk->swapchain.images.size();
    init_info.ImageCount = vulk->swapchain.images.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, renderPass);


    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        // Use any command queue
        VkCommandPool command_pool = vulk->commandPool;
        VkCommandBuffer command_buffer = vulk->commandBuffer;

        VkResult err = vkResetCommandPool(vulk->device, command_pool, 0);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(command_buffer, &begin_info);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        err = vkEndCommandBuffer(command_buffer);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;
        err = vkQueueSubmit(vulk->graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;

        err = vkDeviceWaitIdle(vulk->device);
        VK_CHECK(err);
        if (err != VK_SUCCESS)
            return false;
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }


    return true;

}

bool MyImguiRenderer::updateRenderTarget(const Image &renderTargetImage)
{
    if (!renderPass)
        return false;
    if (frameBuffer)
        vkDestroyFramebuffer(vulk->device, frameBuffer, nullptr);
    frameBuffer = VK_NULL_HANDLE;

    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.layers = 1;
    createInfo.pAttachments = &renderTargetImage.imageView;
    createInfo.width = renderTargetImage.width;
    createInfo.height = renderTargetImage.height;

    VK_CHECK(vkCreateFramebuffer(vulk->device, &createInfo, nullptr, &frameBuffer));

    frameBufferWidth = renderTargetImage.width;
    frameBufferHeight = renderTargetImage.height;

    return true;
}

bool MyImguiRenderer::addTexture(const Image &image, VkDescriptorSet &currentId)
{
    if(currentId == 0)
    {
        currentId = ImGui_ImplVulkan_AddTexture(
            vulk->globalTextureSampler, image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    else
    {
        // Update the Descriptor Set:
        {
            VkDescriptorImageInfo desc_image[1] = {};
            desc_image[0].sampler = vulk->globalTextureSampler;
            desc_image[0].imageView = image.imageView;
            desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkWriteDescriptorSet write_desc[1] = {};
            write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_desc[0].dstSet = currentId;
            write_desc[0].descriptorCount = 1;
            write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_desc[0].pImageInfo = desc_image;
            vkUpdateDescriptorSets(vulk->device, 1, write_desc, 0, NULL);
        }
    }
    return true;
}

void MyImguiRenderer::renderBegin()
{
    ASSERT(renderPass && frameBuffer);

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void MyImguiRenderer::render()
{
    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized && renderPass && frameBuffer)
    {

        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = renderPass;
            info.framebuffer = frameBuffer;
            info.renderArea.extent.width = frameBufferWidth;
            info.renderArea.extent.height = frameBufferHeight;
            info.clearValueCount = 0;
            vkCmdBeginRenderPass(vulk->commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, vulk->commandBuffer);

        // Submit command buffer
        vkCmdEndRenderPass(vulk->commandBuffer);
    }
    writeStamp();
}



