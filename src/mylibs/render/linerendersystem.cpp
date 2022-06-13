#include "linerendersystem.h"

#include <container/arraysliceview.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

#include <scene/scene.h>


#include <string>

LineRenderSystem::~LineRenderSystem()
{
    destroyPipeline(lineRenderPipeline);

    destroyBuffer(vertexBuffer);
}


bool LineRenderSystem::init()
{
    vertexBuffer = createBuffer(16u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    // pipelines.
    {
        Pipeline& pipeline = lineRenderPipeline;

        if (!createGraphicsPipeline(
            getShader(ShaderType::LineVert), getShader(ShaderType::ColoredQuadFrag),
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD } },
            { .depthTarget = RenderTarget{.format = vulk->depthFormat }, .useDepthTest = true, .writeDepth = true },
            pipeline, "line render system", true, VK_PRIMITIVE_TOPOLOGY_LINE_LIST))
        {
            printf("Failed to create graphics pipeline\n");
        }


        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk->renderFrameBufferHandle),

                DescriptorInfo(vertexBuffer),

            });

        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }

    return true;
}

void LineRenderSystem::clear()
{
    lines.clear();
}

void LineRenderSystem::addLine(const Vec3 &posStart, const Vec3 &posEnd, uint32_t color)
{
    lines.push_back( Line{ .start = posStart, .colorStart = color,
        .end = posEnd, .colorEnd = color } );
}


bool LineRenderSystem::prepareToRender()
{
    if(lines.size() == 0)
        return false;

    addToCopylist(sliceFromPodVectorBytes(lines), vertexBuffer);
    return true;
}

void LineRenderSystem::render(const Image &colorImage, const Image &depthImage)
{
    if(lines.size() == 0)
        return;

    beginDebugRegion("Line rendering", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    beginRendering({
        RenderImage{.image = &colorImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD }},
        {.image = &depthImage, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD });

    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, lineRenderPipeline);
    vkCmdDraw(vulk->commandBuffer, lines.size() * 2, 1, 0, 0);


    vkCmdEndRendering(vulk->commandBuffer);

    endDebugRegion();
    writeStamp();
}