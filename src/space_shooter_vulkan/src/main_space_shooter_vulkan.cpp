


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include <container/vector.h>

#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/font_render.h>

#include <string_view>
#include <string.h>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;

static constexpr float POSITION_SCALER = 32768.0f;

static constexpr double SHOT_INTERVAL = 0.1;
static constexpr double BULLET_ALIVE_TIME = 1.0;
static constexpr double ASTEROID_SPAWN_INTERVAL = 5.0;

static constexpr uint32_t AsteroidMaxTypes = 10u;

static constexpr uint32_t AsteroidCorners = 9u;
static constexpr uint32_t BulletCorners = 16u;


enum class EntityModelType : uint32_t
{
    ASTEROID,
    SHIP,
    BULLET
};

struct EntityModel
{
    uint32_t startVertex;
    EntityModelType entityModelType;

    PodVector< uint32_t > modelIndices;
};



struct Entity
{
    float posX;
    float posY;
    float posZ;
    float rotation;

    float speedX;
    float speedY;
    float size;
    uint32_t entityModelIndex;

    double spawnTime;
    uint32_t color;
    uint32_t pad3;
};

struct GpuModelInstance
{
    uint32_t pos;
    uint32_t sinCosRotSize;
    uint32_t color;
    uint32_t modelVertexStartIndex;
};

struct GpuModelVertex
{
    float posX;
    float posY;
};


class SpaceShooter : public VulkanApp
{
public:
    SpaceShooter() {}
    virtual ~SpaceShooter() override;

    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual void resized() override;

    void updateLogic();

    bool createPipelines();
    bool initRun();
public:
    Image renderColorImage;

    Buffer modelVerticesBuffer;
    Buffer instanceBuffer;
    Buffer indexDataBufferModels;

    Pipeline graphicsPipeline;

    Entity playerEntity;

    PodVector< Entity > asteroidEntities;
    PodVector< Entity > bulletEntities;

    PodVector< GpuModelInstance > gpuModelInstances;

    PodVector< GpuModelVertex > vertices;
    Vector< EntityModel > entityModels;

    PodVector< uint32_t > gpuModelIndices;

    std::string_view text = "Space shooter!";


    double lastShot = 0.0;
    double lastAsteroidSpawn = 0.0;
    double currentTime = 0.0;
};



////////////////////////
//
// DEINIT
//
////////////////////////

SpaceShooter::~SpaceShooter()
{
    destroyPipeline(graphicsPipeline);

    destroyBuffer(modelVerticesBuffer);
    destroyBuffer(instanceBuffer);
    destroyBuffer(indexDataBufferModels);

    destroyImage(renderColorImage);
}

bool SpaceShooter::init(const char* windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    modelVerticesBuffer = createBuffer(1024u * 1024u * 16u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Model Vercies data buffer");

    instanceBuffer = createBuffer(64u * 1024 * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Model Instance data buffer");


    indexDataBufferModels = createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer for models");

    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);
    return createPipelines() && initRun();
}


bool SpaceShooter::createPipelines()
{
    {
        Pipeline& pipeline = graphicsPipeline;
        if(!createGraphicsPipeline(
            getShader(ShaderType::SpaceShip2DModelVert), getShader(ShaderType::SpaceShip2DModelFrag),
            { RenderTarget{.format = vulk.defaultColorFormat } }, {  }, pipeline, false))
        {
            printf("failed to create pipeline\n");
            return false;
        }

        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk.renderFrameBufferHandle),
                DescriptorInfo(modelVerticesBuffer),
                DescriptorInfo(instanceBuffer),
            });

        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    resized();
    return true;
}

void SpaceShooter::resized()
{
    destroyImage(renderColorImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    fontSystem.setRenderTarget(renderColorImage);
    createFramebuffer(graphicsPipeline, { renderColorImage.imageView }, renderColorImage.width, renderColorImage.height);
}

static uint32_t getPackedPosition(float x, float y)
{
    uint32_t result = uint32_t(( x / POSITION_SCALER ) * 65535.0f);
    result += uint32_t(( y / POSITION_SCALER ) * 65535.0f) << 16u;

    return result;
}

static uint32_t getPackedSizeRot(float sz, float rotInRad)
{
    uint32_t result = uint32_t(( sz / 64.0f ) * 1023.0f);
    float sinv = fsinf(rotInRad);
    float cosv = fcosf(rotInRad);
    result += uint32_t(( sinv * 0.5f + 0.5f ) * 1023.0f) << 10u;
    result += uint32_t(( cosv * 0.5f + 0.5f ) * 1023.0f) << 20u;

    return result;
}


static void updateGpuEntity(const Entity &ent, const Vector<EntityModel> &entityModels,
    PodVector< GpuModelInstance > &gpuModelInstances, PodVector< uint32_t > &gpuModelIndices)
{
    const EntityModel &model = entityModels[ ent.entityModelIndex ];
    uint32_t entityIndex = uint32_t(gpuModelInstances.size());

    // Notice this quantatization doesnt work too well with high high resolutions....
    gpuModelInstances.emplace_back(GpuModelInstance{ .pos = getPackedPosition(ent.posX, ent.posY),
        .sinCosRotSize = getPackedSizeRot(ent.size, ent.rotation),
        .color = ent.color,
        .modelVertexStartIndex = model.startVertex });


    for (uint32_t indice = 0u; indice < uint32_t(model.modelIndices.size()); ++indice)
    {
        uint32_t ind = model.modelIndices[ indice ];
        ind = ind + ( entityIndex << 16u );
        gpuModelIndices.push_back(ind);
    }
}

static void updateGpuEntities(const PodVector<Entity> &entities, const Vector<EntityModel> &entityModels,
    PodVector< GpuModelInstance > &gpuModelInstances, PodVector< uint32_t > &gpuModelIndices)
{
    for (uint32_t entityIndex = 0u; entityIndex < entities.size(); ++entityIndex)
    {
        updateGpuEntity(entities[entityIndex], entityModels, gpuModelInstances, gpuModelIndices);
    }
}

static Entity spawnAsteroidEntity(double spawnTime)
{
    float xPos = float(rand()) / float(RAND_MAX) * 4096.0f;
    float yPos = float(rand()) / float(RAND_MAX) * 2048.0f;
    float size = 5.0f + 20.0f * float(rand()) / float(RAND_MAX);

    uint32_t modelIndex = ( std::abs(rand()) % AsteroidMaxTypes ) + 1u;

    return Entity{
        .posX = xPos, .posY = yPos, .posZ = 0.5f, .rotation = 0.0f,
        .speedX = 0.0f, .speedY = 0.0f, .size = size, .entityModelIndex = modelIndex,
        .spawnTime = spawnTime,
        .color = getColor(0.5, 0.5, 0.5, 1.0f) };
}


bool SpaceShooter::initRun()
{
    gpuModelInstances.reserve(1u << 16u);
    gpuModelIndices.reserve(1u << 20u);

    // Building models.
    {
        // Building ship model
        {
            uint32_t startVertex = uint32_t(vertices.size());
            entityModels.emplace_back(EntityModel{ .startVertex = startVertex, .entityModelType = EntityModelType::SHIP });
            EntityModel& model = entityModels.back();

            vertices.emplace_back(GpuModelVertex{ .posX = -1.0f, .posY = -1.0f });
            vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 1.5f });
            vertices.emplace_back(GpuModelVertex{ .posX = 1.0f, .posY = -1.0f });
            model.modelIndices.emplace_back(0);
            model.modelIndices.emplace_back(2);
            model.modelIndices.emplace_back(1);
        }

        // Asteroid models
        for (uint32_t asteroidTypes = 0u; asteroidTypes < AsteroidMaxTypes; ++asteroidTypes)
        {

            uint32_t startVertex = uint32_t(vertices.size());
            entityModels.emplace_back(EntityModel{ .startVertex = startVertex, .entityModelType = EntityModelType::SHIP });
            EntityModel& model = entityModels.back();

            vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f });
            for (uint32_t i = 0; i < AsteroidCorners; ++i)
            {
                float angle = -float(i) * float(2.0f * PI) / float(AsteroidCorners);
                float x = fcosf(angle);
                float y = fsinf(angle);
                float r = 0.5f + 0.5f * (float(rand()) / float(RAND_MAX));

                vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y * r });

                model.modelIndices.emplace_back(0u);
                model.modelIndices.emplace_back((i + 1) % AsteroidCorners + 1);
                model.modelIndices.emplace_back((i + 0) % AsteroidCorners + 1);
            }
        }

        // Bullets
        {

            uint32_t startVertex = uint32_t(vertices.size());
            entityModels.emplace_back(EntityModel{ .startVertex = startVertex, .entityModelType = EntityModelType::SHIP });
            EntityModel& model = entityModels.back();

            vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f });
            for (uint32_t i = 0; i < BulletCorners; ++i)
            {
                float angle = -float(i) * float(2.0f * PI) / float(BulletCorners);
                float x = fcosf(angle);
                float y = fsinf(angle);
                float r = 1.0f;

                vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y * r });

                model.modelIndices.emplace_back(0u);
                model.modelIndices.emplace_back((i + 1) % BulletCorners + 1);
                model.modelIndices.emplace_back((i + 0) % BulletCorners + 1);
            }
        }


        uint32_t offset = 0u;
        offset = uploadToScratchbuffer((void*)vertices.data(), size_t(sizeof(GpuModelVertex) * vertices.size()), offset);
        uploadScratchBufferToGpuBuffer(modelVerticesBuffer, offset);

    }


    // Create instances.
    {
        double spawnTime = glfwGetTime();
        // Player instance.
        {
            float xPos = 200.0f;
            float yPos = 200.0f;
            float size = 10.0f;
            uint32_t modelIndex = 0u;

            playerEntity = (Entity{
                .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f,
                .speedX = 0.0f, .speedY = 0.0f, .size = size, .entityModelIndex = modelIndex,
                .spawnTime = spawnTime,
                .color = getColor(1.0, 1.0, 0.0, 1.0f) });
        }
        // Asteroid instances.
        for (uint32_t asteroidInstances = 0u; asteroidInstances < 256; ++asteroidInstances)
        {
            asteroidEntities.emplace_back(spawnAsteroidEntity(spawnTime));
        }
    }

    return true;
}

void SpaceShooter::logicUpdate()
{
    VulkanApp::logicUpdate();

    currentTime = glfwGetTime();

    Timer updateDurTimer;
    static double dtSplit = 0.0;
    dtSplit += dt;
    // Update position, definitely not accurate physics, if dt is big this doesn't work properly, trying to split it into several updates.
    while (dtSplit > 0.005f)
    {
        double dddt = ffminf(dtSplit, 0.005f);
        float origSpeed = 1.0f * fsqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);

        double timeHere = currentTime + dddt;

        if (isDown(GLFW_KEY_LEFT) || isDown(GLFW_KEY_A))
        {
            float rotSpeed = 1.0f; // fminf(origSpeed, 1.00f);
            rotSpeed = rotSpeed * 2.0f - (1.0f - rotSpeed) * 0.005f;
            playerEntity.rotation += rotSpeed * dddt;
        }
        if (isDown(GLFW_KEY_RIGHT) || isDown(GLFW_KEY_D))
        {
            float rotSpeed = 1.0f; //fminf(origSpeed, 1.0f);
            rotSpeed = rotSpeed * 2.0f - (1.0f - rotSpeed) * 0.005f;
            playerEntity.rotation -= rotSpeed * dddt;
        }
        playerEntity.rotation = ffmodf(playerEntity.rotation, PI * 2.0);
        if (isDown(GLFW_KEY_UP) || isDown(GLFW_KEY_W))
        {
            playerEntity.speedX += fcosf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
            playerEntity.speedY += fsinf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
        }


        if (isDown(GLFW_KEY_SPACE) && (timeHere - lastShot) >= SHOT_INTERVAL)
        {
            if (bulletEntities.size() < (1 << 15))
            {
                float rotX = fcosf(playerEntity.rotation + float(PI) * 0.5f);
                float rotY = fsinf(playerEntity.rotation + float(PI) * 0.5f);

                float speedX = rotX * 1000.0f;
                float speedY = rotY * 1000.0f;

                float posX = playerEntity.posX + rotX * 10.0f;
                float posY = playerEntity.posY + rotY * 10.0f;

                bulletEntities.emplace_back(Entity{
                    .posX = posX, .posY = posY, .posZ = 0.5f, .rotation = 0.0f,
                    .speedX = speedX, .speedY = speedY, .size = 4.0f, .entityModelIndex = 1 + AsteroidMaxTypes,
                    .spawnTime = glfwGetTime(),
                    .color = getColor(0.0, 0.5, 1.0, 1.0f) });

                lastShot = timeHere;
            }
        }

        {
            float origSpeed = fsqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);
            float dec = dddt * 0.001f * origSpeed;
            float speed = ffmaxf(origSpeed - dec, 0.0f);


            playerEntity.posX += playerEntity.speedX * dddt;
            playerEntity.posY += playerEntity.speedY * dddt;
        }


        // Threading for updating bullets and asteroids?
        for (int32_t bullInd = bulletEntities.size(); bullInd > 0; --bullInd)
        {
            Entity& ent = bulletEntities[bullInd - 1];
            ent.posX += ent.speedX * dddt;
            ent.posY += ent.speedY * dddt;



            if (timeHere - ent.spawnTime > BULLET_ALIVE_TIME)
            {
                bulletEntities[bullInd - 1] = bulletEntities.back();
                bulletEntities.resize(bulletEntities.size() - 1);
            }

            else
            {
                for (int32_t astInd = asteroidEntities.size(); astInd > 0; --astInd)
                {
                    Entity& asteroidEnt = asteroidEntities[astInd - 1];

                    double minSize = (ent.size + asteroidEnt.size) * 0.8f;

                    if (std::abs(ent.posX - asteroidEnt.posX) < minSize && std::abs(ent.posY - asteroidEnt.posY) < minSize)
                    {
                        asteroidEntities[astInd - 1] = asteroidEntities.back();
                        asteroidEntities.resize(asteroidEntities.size() - 1);

                        bulletEntities[bullInd - 1] = bulletEntities.back();
                        bulletEntities.resize(bulletEntities.size() - 1);
                        break;
                    }
                }
            }
        }


        dtSplit -= dddt;
    }
    const float slowDown = std::max(0.0, 1.0 - 10.0 * dt);
    playerEntity.speedX *= slowDown;
    playerEntity.speedY *= slowDown;

    while (playerEntity.posX > windowWidth)
    {
        playerEntity.posX -= windowWidth;
    }
    while (playerEntity.posX < 0.0f)
    {
        playerEntity.posX += windowWidth;
    }
    while (playerEntity.posY > windowHeight)
    {
        playerEntity.posY -= windowHeight;
    }
    while (playerEntity.posY < 0.0f)
    {
        playerEntity.posY += windowHeight;
    }


    if (currentTime - lastAsteroidSpawn > ASTEROID_SPAWN_INTERVAL)
    {
        lastAsteroidSpawn = currentTime;
        uint32_t spawnAmount = 256u - asteroidEntities.size();
        spawnAmount = spawnAmount > 10u ? 10u : spawnAmount;
        for (; spawnAmount > 0; --spawnAmount)
        {
            asteroidEntities.emplace_back(spawnAsteroidEntity(currentTime));
        }
    }

    float updateDur = float(updateDurTimer.getDuration());


    // This could be moved to be done in gpu compute shader (and probably should be, could also include culling). Then using indirect drawing to render the stuff out, instead of
    // building the index buffer every frame on cpu then copying it on gpu. The instance buffer could be used to update the data more smartly perhaps? But just easy way out.
    {
        gpuModelIndices.clear();
        gpuModelInstances.clear();

        updateGpuEntity(playerEntity, entityModels, gpuModelInstances, gpuModelIndices);
        updateGpuEntities(asteroidEntities, entityModels, gpuModelInstances, gpuModelIndices);
        updateGpuEntities(bulletEntities, entityModels, gpuModelInstances, gpuModelIndices);

        ASSERT(gpuModelInstances.size() < (1 << 16u));
    }
    fontSystem.addText(text, Vector2(100.0f, 10.0f), Vec2(8.0f, 12.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f));
}

void SpaceShooter::renderUpdate()
{
    VulkanApp::renderUpdate();

    addToCopylist(sliceFromPodVectorBytes(gpuModelInstances), instanceBuffer.buffer, 0);
    addToCopylist(sliceFromPodVectorBytes(gpuModelIndices), indexDataBufferModels.buffer, 0);
}

void SpaceShooter::renderDraw()
{
    addImageBarrier(imageBarrier(renderColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    flushBarriers();

    // Drawingg
    {

        static constexpr VkClearValue colorClear = { .color{ 0.0f, 0.0f, 0.0f, 1.0f } };
        beginRenderPass(graphicsPipeline, { colorClear });
        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdBindIndexBuffer(vulk.commandBuffer, indexDataBufferModels.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk.commandBuffer, uint32_t(gpuModelIndices.size()), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vulk.commandBuffer);

        fontSystem.render();

    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    present(renderColorImage);
}





int main(int argCount, char **argv)
{
    SpaceShooter app;
    if (app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = false,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = false,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}



