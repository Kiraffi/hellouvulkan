#include "linerendersystem.h"

#include <core/nullable.h>

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/vector.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <scene/scene.h>

struct LineRenderSystemData
{
    struct Line
    {
        Vec3 m_start;
        u32 m_colorStart;
        Vec3 m_end;
        u32 m_colorEnd;
    };

    Buffer m_vertexBuffer[VulkanGlobal::FramesInFlight];
    Pipeline m_lineRenderPipeline;

    PodVector<Line> m_lines;
};

static Nullable<LineRenderSystemData> s_lineRenderSystemData;

void LineRenderSystem::deinit()
{
    if(s_lineRenderSystemData.get())
    {
        MyVulkan::destroyPipeline(s_lineRenderSystemData.get()->m_lineRenderPipeline);
        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
            VulkanResources::destroyBuffer(s_lineRenderSystemData.get()->m_vertexBuffer[i]);
        s_lineRenderSystemData.destroy();
    }
}


bool LineRenderSystem::init()
{
    s_lineRenderSystemData.create();
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        s_lineRenderSystemData.get()->m_vertexBuffer[i] = VulkanResources::createBuffer(4u * 1024u * 1024u,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    // pipelines.
    {
        Pipeline& pipeline = s_lineRenderSystemData.get()->m_lineRenderPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        pipeline.renderPass = MyVulkan::createRenderPass(
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE } },
            { .format = vulk->depthFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE });
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if (!MyVulkan::createGraphicsPipeline(
            VulkanShader::getShader(ShaderType::LineVert), VulkanShader::getShader(ShaderType::ColoredQuadFrag),
            { rgbaAtt },
            { .depthTarget = RenderTarget{.format = vulk->depthFormat }, .useDepthTest = true, .writeDepth = true },
            pipeline, "line render system", VK_PRIMITIVE_TOPOLOGY_LINE_LIST))
        {
            printf("Failed to create graphics pipeline\n");
        }

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(s_lineRenderSystemData.get()->m_vertexBuffer[i]),
            };
        }
        if (!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;
    }

    return true;
}

void LineRenderSystem::clear()
{
    s_lineRenderSystemData.get()->m_lines.clear();
}

void LineRenderSystem::addLine(const Vec3 &posStart, const Vec3 &posEnd, u32 color)
{
    s_lineRenderSystemData.get()->m_lines.push_back(LineRenderSystemData::Line{
        .m_start = posStart,
        .m_colorStart = color,
        .m_end = posEnd,
        .m_colorEnd = color } );
}


bool LineRenderSystem::prepareToRender()
{
    if(s_lineRenderSystemData.get()->m_lines.size() == 0)
        return false;

    VulkanResources::addToCopylist(sliceFromPodVectorBytes(s_lineRenderSystemData.get()->m_lines),
        s_lineRenderSystemData.get()->m_vertexBuffer[vulk->frameIndex]);
    return true;
}

void LineRenderSystem::render(const Image &colorImage, const Image &depthImage)
{
    if(s_lineRenderSystemData.get()->m_lines.size() == 0)
        return;

    MyVulkan::beginDebugRegion("Line rendering", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    MyVulkan::beginRenderPass(s_lineRenderSystemData.get()->m_lineRenderPipeline, {});

    /*
    beginRendering({
        RenderImage{.image = &colorImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD }},
        {.image = &depthImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD });
        */
    MyVulkan::bindGraphicsPipelineWithDescriptors(s_lineRenderSystemData.get()->m_lineRenderPipeline,
        vulk->frameIndex);
    vkCmdDraw(vulk->commandBuffer, s_lineRenderSystemData.get()->m_lines.size() * 2, 1, 0, 0);


    //vkCmdEndRendering(vulk->commandBuffer);
    vkCmdEndRenderPass(vulk->commandBuffer);

    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}


void LineRenderSystem::setRendertargets(const Image &colorImage, const Image &depthImage)
{
    ASSERT(VulkanResources::createFramebuffer(
        s_lineRenderSystemData.get()->m_lineRenderPipeline, { colorImage, depthImage }));
}