
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <core/general.h>
#include <app/glfw_keys.h>
#include <core/mytypes.h>
#include <core/timer.h>
#include <app/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/fontrendersystem.h>

#include <math.h> // rand and RAND_MAX

static constexpr i32 SCREEN_WIDTH = 800;
static constexpr i32 SCREEN_HEIGHT = 600;

static constexpr float POSITION_SCALER = 32768.0f;

static constexpr double SHOT_INTERVAL = 0.1;
static constexpr double BULLET_ALIVE_TIME = 1.0;
static constexpr double ASTEROID_SPAWN_INTERVAL = 5.0;

static constexpr u32 AsteroidMaxTypes = 10u;

static constexpr u32 AsteroidCorners = 9u;
static constexpr u32 BulletCorners = 16u;


enum class EntityModelType : u32
{
    ASTEROID,
    SHIP,
    BULLET
};

struct EntityModel
{
    u32 startVertex;
    EntityModelType entityModelType;

    PodVector< u32 > modelIndices;
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
    u32 entityModelIndex;

    double spawnTime;
    u32 color;
    u32 pad3;
};

struct GpuModelInstance
{
    u32 pos;
    u32 sinCosRotSize;
    u32 color;
    u32 modelVertexStartIndex;
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

    virtual bool init(const char* windowStr, i32 screenWidth, i32 screenHeight) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

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

    PodVector< u32 > gpuModelIndices;

    const char *text = "Space shooter!";


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

bool SpaceShooter::init(const char* windowStr, i32 screenWidth, i32 screenHeight)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight))
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
    //setObjectTag(device, (u64)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);
    return createPipelines() && initRun();
}


bool SpaceShooter::createPipelines()
{
    {
        Pipeline& pipeline = graphicsPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        pipeline.renderPass = createRenderPass(
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR } },
            {});
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };


        if(!createGraphicsPipeline(
            getShader(ShaderType::SpaceShip2DModelVert), getShader(ShaderType::SpaceShip2DModelFrag),
            { rgbaAtt }, {  }, pipeline, "Space shooter 2d rendering"))
        {
            printf("failed to create pipeline\n");
            return false;
        }
        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(modelVerticesBuffer),
                DescriptorInfo(instanceBuffer),
            };
        }
        if (!updateBindDescriptorSet(pipeline))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    return resized();
}

bool SpaceShooter::resized()
{
    // create color and depth images
    if (!createRenderTargetImage(vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", renderColorImage))
    {
        printf("Failed to create render target image\n");
        return false;
    }

    fontSystem.setRenderTarget(renderColorImage);
    ASSERT(createFramebuffer(graphicsPipeline, { renderColorImage }));
    return true;
}

static u32 getPackedPosition(float x, float y)
{
    u32 result = u32(( x / POSITION_SCALER ) * 65535.0f);
    result += u32(( y / POSITION_SCALER ) * 65535.0f) << 16u;

    return result;
}

static u32 getPackedSizeRot(float sz, float rotInRad)
{
    u32 result = u32(( sz / 64.0f ) * 1023.0f);
    float sinv = Supa::sinf(rotInRad);
    float cosv = Supa::cosf(rotInRad);
    result += u32(( sinv * 0.5f + 0.5f ) * 1023.0f) << 10u;
    result += u32(( cosv * 0.5f + 0.5f ) * 1023.0f) << 20u;

    return result;
}


static void updateGpuEntity(const Entity &ent, const Vector<EntityModel> &entityModels,
    PodVector< GpuModelInstance > &gpuModelInstances, PodVector< u32 > &gpuModelIndices)
{
    const EntityModel &model = entityModels[ ent.entityModelIndex ];
    u32 entityIndex = u32(gpuModelInstances.size());

    // Notice this quantatization doesnt work too well with high high resolutions....
    gpuModelInstances.emplace_back(GpuModelInstance{ .pos = getPackedPosition(ent.posX, ent.posY),
        .sinCosRotSize = getPackedSizeRot(ent.size, ent.rotation),
        .color = ent.color,
        .modelVertexStartIndex = model.startVertex });


    for (u32 indice = 0u; indice < u32(model.modelIndices.size()); ++indice)
    {
        u32 ind = model.modelIndices[ indice ];
        ind = ind + ( entityIndex << 16u );
        gpuModelIndices.push_back(ind);
    }
}

static void updateGpuEntities(const PodVector<Entity> &entities, const Vector<EntityModel> &entityModels,
    PodVector< GpuModelInstance > &gpuModelInstances, PodVector< u32 > &gpuModelIndices)
{
    for (u32 entityIndex = 0u; entityIndex < entities.size(); ++entityIndex)
    {
        updateGpuEntity(entities[entityIndex], entityModels, gpuModelInstances, gpuModelIndices);
    }
}

static Entity spawnAsteroidEntity(double spawnTime)
{
    float xPos = float(rand()) / float(RAND_MAX) * 4096.0f;
    float yPos = float(rand()) / float(RAND_MAX) * 2048.0f;
    float size = 5.0f + 20.0f * float(rand()) / float(RAND_MAX);

    u32 modelIndex = (abs(rand()) % AsteroidMaxTypes ) + 1u;

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
            u32 startVertex = u32(vertices.size());
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
        for (u32 asteroidTypes = 0u; asteroidTypes < AsteroidMaxTypes; ++asteroidTypes)
        {

            u32 startVertex = u32(vertices.size());
            entityModels.emplace_back(EntityModel{ .startVertex = startVertex, .entityModelType = EntityModelType::SHIP });
            EntityModel& model = entityModels.back();

            vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f });
            for (u32 i = 0; i < AsteroidCorners; ++i)
            {
                float angle = -float(i) * float(2.0f * PI) / float(AsteroidCorners);
                float x = Supa::cosf(angle);
                float y = Supa::sinf(angle);
                float r = 0.5f + 0.5f * (float(rand()) / float(RAND_MAX));

                vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y * r });

                model.modelIndices.emplace_back(0u);
                model.modelIndices.emplace_back((i + 1) % AsteroidCorners + 1);
                model.modelIndices.emplace_back((i + 0) % AsteroidCorners + 1);
            }
        }

        // Bullets
        {

            u32 startVertex = u32(vertices.size());
            entityModels.emplace_back(EntityModel{ .startVertex = startVertex, .entityModelType = EntityModelType::SHIP });
            EntityModel& model = entityModels.back();

            vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f });
            for (u32 i = 0; i < BulletCorners; ++i)
            {
                float angle = -float(i) * float(2.0f * PI) / float(BulletCorners);
                float x = Supa::cosf(angle);
                float y = Supa::sinf(angle);
                float r = 1.0f;

                vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y * r });

                model.modelIndices.emplace_back(0u);
                model.modelIndices.emplace_back((i + 1) % BulletCorners + 1);
                model.modelIndices.emplace_back((i + 0) % BulletCorners + 1);
            }
        }


        u32 offset = 0u;
        offset = uploadToScratchbuffer((void*)vertices.data(), size_t(sizeof(GpuModelVertex) * vertices.size()), offset);
        uploadScratchBufferToGpuBuffer(modelVerticesBuffer, offset);

    }


    // Create instances.
    {
        double spawnTime = getTime();
        // Player instance.
        {
            float xPos = 200.0f;
            float yPos = 200.0f;
            float size = 10.0f;
            u32 modelIndex = 0u;

            playerEntity = (Entity{
                .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f,
                .speedX = 0.0f, .speedY = 0.0f, .size = size, .entityModelIndex = modelIndex,
                .spawnTime = spawnTime,
                .color = getColor(1.0, 1.0, 0.0, 1.0f) });
        }
        // Asteroid instances.
        for (u32 asteroidInstances = 0u; asteroidInstances < 256; ++asteroidInstances)
        {
            asteroidEntities.emplace_back(spawnAsteroidEntity(spawnTime));
        }
    }

    return true;
}

void SpaceShooter::logicUpdate()
{
    VulkanApp::logicUpdate();

    currentTime = getTime();

    Timer updateDurTimer;
    static double dtSplit = 0.0;
    dtSplit += dt;
    // Update position, definitely not accurate physics, if dt is big this doesn't work properly, trying to split it into several updates.
    while (dtSplit > 0.005f)
    {
        double dddt = Supa::minf(dtSplit, 0.005f);
        float origSpeed = 1.0f * Supa::sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);

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
        playerEntity.rotation = Supa::modf(playerEntity.rotation, PI * 2.0);
        if (isDown(GLFW_KEY_UP) || isDown(GLFW_KEY_W))
        {
            playerEntity.speedX += Supa::cosf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
            playerEntity.speedY += Supa::sinf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
        }


        if (isDown(GLFW_KEY_SPACE) && (timeHere - lastShot) >= SHOT_INTERVAL)
        {
            if (bulletEntities.size() < (1 << 15))
            {
                float rotX = Supa::cosf(playerEntity.rotation + float(PI) * 0.5f);
                float rotY = Supa::sinf(playerEntity.rotation + float(PI) * 0.5f);

                float speedX = rotX * 1000.0f;
                float speedY = rotY * 1000.0f;

                float posX = playerEntity.posX + rotX * 10.0f;
                float posY = playerEntity.posY + rotY * 10.0f;

                bulletEntities.emplace_back(Entity{
                    .posX = posX, .posY = posY, .posZ = 0.5f, .rotation = 0.0f,
                    .speedX = speedX, .speedY = speedY, .size = 4.0f, .entityModelIndex = 1 + AsteroidMaxTypes,
                    .spawnTime = getTime(),
                    .color = getColor(0.0, 0.5, 1.0, 1.0f) });

                lastShot = timeHere;
            }
        }

        {
            float origSpeed = Supa::sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);
            float dec = dddt * 0.001f * origSpeed;
            float speed = Supa::maxf(origSpeed - dec, 0.0f);


            playerEntity.posX += playerEntity.speedX * dddt;
            playerEntity.posY += playerEntity.speedY * dddt;
        }


        // Threading for updating bullets and asteroids?
        for (i32 bullInd = bulletEntities.size(); bullInd > 0; --bullInd)
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
                for (i32 astInd = asteroidEntities.size(); astInd > 0; --astInd)
                {
                    Entity& asteroidEnt = asteroidEntities[astInd - 1];

                    double minSize = (ent.size + asteroidEnt.size) * 0.8f;

                    if (Supa::absf(ent.posX - asteroidEnt.posX) < minSize && Supa::absf(ent.posY - asteroidEnt.posY) < minSize)
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
    const float slowDown = Supa::maxf(0.0, 1.0 - 10.0 * dt);
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
        u32 spawnAmount = 256u - asteroidEntities.size();
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
    prepareToGraphicsSampleWrite(renderColorImage);

    // Drawingg
    {

        static constexpr VkClearValue colorClear = { .color{ 0.0f, 0.0f, 0.0f, 1.0f } };
        beginRenderPass(graphicsPipeline, { colorClear });
        // draw calls here
        // Render
        {
            bindGraphicsPipelineWithDecriptors(graphicsPipeline, vulk->frameIndex);
            vkCmdBindIndexBuffer(vulk->commandBuffer, indexDataBufferModels.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, u32(gpuModelIndices.size()), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vulk->commandBuffer);

        fontSystem.render();

    }

    present(renderColorImage);
}





i32 main(i32 argCount, char **argv)
{
    initMemory();
    {
        SpaceShooter app;
        if (app.init("Vulkan, space shooter", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}



