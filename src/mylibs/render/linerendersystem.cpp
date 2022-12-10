#include "linerendersystem.h"

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/vector.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

#include <scene/scene.h>

LineRenderSystem::~LineRenderSystem()
{
    MyVulkan::destroyPipeline(lineRenderPipeline);
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        VulkanResources::destroyBuffer(vertexBuffer[i]);
}


bool LineRenderSystem::init()
{
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        vertexBuffer[i] = VulkanResources::createBuffer(4u * 1024u * 1024u,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    // pipelines.
    {
        Pipeline& pipeline = lineRenderPipeline;
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
                DescriptorInfo(vertexBuffer[i]),
            };
        }
        if (!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;
    }

    return true;
}

void LineRenderSystem::clear()
{
    lines.clear();
}

void LineRenderSystem::addLine(const Vec3 &posStart, const Vec3 &posEnd, u32 color)
{
    lines.push_back( Line{ .start = posStart, .colorStart = color,
        .end = posEnd, .colorEnd = color } );
}


bool LineRenderSystem::prepareToRender()
{
    if(lines.size() == 0)
        return false;

    VulkanResources::addToCopylist(sliceFromPodVectorBytes(lines), vertexBuffer[vulk->frameIndex]);
    return true;
}

void LineRenderSystem::render(const Image &colorImage, const Image &depthImage)
{
    if(lines.size() == 0)
        return;

    MyVulkan::beginDebugRegion("Line rendering", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    MyVulkan::beginRenderPass(lineRenderPipeline, {});

    /*
    beginRendering({
        RenderImage{.image = &colorImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD }},
        {.image = &depthImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD });
        */
    MyVulkan::bindGraphicsPipelineWithDecriptors(lineRenderPipeline, vulk->frameIndex);
    vkCmdDraw(vulk->commandBuffer, lines.size() * 2, 1, 0, 0);


    //vkCmdEndRendering(vulk->commandBuffer);
    vkCmdEndRenderPass(vulk->commandBuffer);

    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}


void LineRenderSystem::setRendertargets(const Image &colorImage, const Image &depthImage)
{
    ASSERT(VulkanResources::createFramebuffer(lineRenderPipeline, { colorImage, depthImage }));
}