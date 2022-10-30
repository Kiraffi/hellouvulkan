#include "generated_components.h"
#include "generated_systems.h"

#include <components/components.h>
#include <components/generated_components.h>
#include <components/generated_systems.h>

#include <container/podvector.h>
#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <core/json.h>
#include <core/general.h>
#include <core/glfw_keys.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>
#include <core/writejson.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/matrix_inline_functions.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>



static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;


// Would be header
struct EntitySystems;
class TestSystem
{
public:
    static bool update(EntitySystems& entitySystems, double dt);
    static bool init(EntitySystems &entitySystems);
    //static void deinit(EntitySystems &entitySystems);
};
class TestSystem2
{
public:
    static bool update(EntitySystems &entitySystems, double dt);
    static bool init(EntitySystems &entitySystems);
    //static void deinit(EntitySystems &entitySystems);
};
class TestSystem3
{
public:
    static bool update(EntitySystems &entitySystems);
};
// Implementation here
struct EntitySystems
{
    bool syncPoints();
    GameEntitySystem gameEntitySystem;
};

bool EntitySystems::syncPoints()
{
    return gameEntitySystem.syncReadWrites();
}

bool TestSystem::init(EntitySystems &entitySystems)
{
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;
    {
        auto mtx = entitySystems.gameEntitySystem.getLockedMutexHandle();

        EntitySystemHandle handle1 = gameEnts.addEntity(mtx);
        EntitySystemHandle handle2 = gameEnts.addEntity(mtx);

        gameEnts.addTransformComponentComponent(handle1, TransformComponent{.position = Vector4{1, 2, 3, 1 }});
        gameEnts.addTransformComponentComponent(handle2, TransformComponent{.position = Vector4{4, 5, 6, 1 }});

        gameEnts.addMat4ComponentComponent(handle1, {});
        gameEnts.addMat4ComponentComponent(handle2, {});

        gameEnts.releaseLockedMutexHandle(mtx);
    }
    entitySystems.syncPoints();

    return true;
}
bool TestSystem2::init(EntitySystems &entitySystems)
{
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;
    {
        auto mtx = entitySystems.gameEntitySystem.getLockedMutexHandle();

        EntitySystemHandle handle1 = gameEnts.addEntity(mtx);
        EntitySystemHandle handle2 = gameEnts.addEntity(mtx);

        gameEnts.addTransformComponentComponent(handle1, TransformComponent{ .position = Vector4{ 10, 20, 30, 0 } });
        //gameEnts.addTransformComponentComponent(handle2, TransformComponent{ .position = Vector4{ 40, 50, 60, 0 } });

        //gameEnts.addMat4ComponentComponent(handle1, {});
        gameEnts.addMat4ComponentComponent(handle2, {});

        gameEnts.releaseLockedMutexHandle(mtx);
    }
    entitySystems.syncPoints();

    return true;
}

bool TestSystem::update(EntitySystems &entitySystems, double dt)
{
    if(dt <= 0.0)
        return false;
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;

    auto gameEntsWriteComponents = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::TransformComponent);

    const auto &gameEntsRWHandle = gameEnts.getRWHandle({}, gameEntsWriteComponents);
    u32 gameEntCount = gameEnts.getEntityCount();

    TransformComponent* transformComponents = gameEnts.getTransformComponentWriteArray(gameEntsRWHandle);
    if(transformComponents == nullptr)
        return false;

    for(u32 i = 0; i < gameEntCount; ++i)
    {
        if(!gameEnts.hasComponents(i, gameEntsRWHandle))
            continue;
        transformComponents[i].position.x += 0.01f * dt;
    }

    return true;
}

bool TestSystem2::update(EntitySystems &entitySystems, double dt)
{
    if(dt <= 0.0)
        return false;
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;

    auto gameEntsReadComponents = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::TransformComponent);

    auto gameEntsWriteComponents = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::Mat3x4Component);

    const auto &gameEntsRWHandle = gameEnts.getRWHandle(gameEntsReadComponents, gameEntsWriteComponents);
    u32 gameEntCount = gameEnts.getEntityCount();

    const TransformComponent *transformComponents = gameEnts.getTransformComponentReadArray(gameEntsRWHandle);
    Mat4Component* matComponents = gameEnts.getMat4ComponentWriteArray(gameEntsRWHandle);

    if(transformComponents == nullptr || matComponents == nullptr)
        return false;

    for(u32 i = 0; i < gameEntCount; ++i)
    {
        if(!gameEnts.hasComponents(i, gameEntsRWHandle))
            continue;

        getMatrixFromTransform(transformComponents[i], matComponents[i].mat);
    }

    return true;
}

bool TestSystem3::update(EntitySystems &entitySystems)
{
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;

    auto gameEntsReadComponents = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::TransformComponent)
        .addComponent(ComponentType::Mat3x4Component);

    const auto &gameEntsRWHandle = gameEnts.getRWHandle(gameEntsReadComponents, {});
    u32 gameEntCount = gameEnts.getEntityCount();

    const TransformComponent *transformComponents = gameEnts.getTransformComponentReadArray(gameEntsRWHandle);
    const Mat4Component *matComponents = gameEnts.getMat4ComponentReadArray(gameEntsRWHandle);

    if(transformComponents == nullptr || matComponents == nullptr)
        return false;

    for(u32 i = 0; i < gameEntCount; ++i)
    {
        if(!gameEnts.hasComponents(i, gameEntsRWHandle))
            continue;
        const Vector4 &pos = transformComponents[i].position;
        const Mat3x4 &m = matComponents[i].mat;
        LOG("%u: pos[%.2f, %.2f, %.2f]\n", i, pos.x, pos.y, pos.z);
        LOG("[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n",
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11]);
        //getMatrixFromTransform(transformComponents[i], matComponents[i].mat);
    }

    return true;
}







class SerializeComponent : public VulkanApp
{
public:
    SerializeComponent() {}
    virtual ~SerializeComponent() override;

    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

    void updateText(StringView str);

private:
    Vector2 fontSize = Vector2(8.0f, 12.0f);
    Image renderColorImage;
    String text = "Text";

    TransformComponent trans;
    EntitySystems entitySystems;
};


////////////////////////
//
// DEINIT
//
////////////////////////

SerializeComponent::~SerializeComponent()
{
    destroyImage(renderColorImage);
}


bool SerializeComponent::init(const char *windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters &params)
{
    if(!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    {
        StaticModelEntity testEntity;
        /* Will assert having adding and read or write happening without sync.
        const auto &readWriteHandle1 = testEntity.getReadWriteHandle(testEntity.getReadWriteHandleBuilder()
            .addArrayRead(ComponentType::HeritagedType) // Will assert if trying to access heritaged1readarray without inserting this to read accessor
            // .addArrayRead(ComponentType::HeritagedType2) // Will assert when trying to read and write to same array without syncing
            .addArrayWrite(ComponentType::HeritagedType2) // Will assert if trying to access Heritaged2WriteArray without inserting this to write accessors
        );
        */
        // Needs something like getLockGuard / getLock
        {
            auto mtx = testEntity.getLockedMutexHandle();

            EntitySystemHandle handle1 = testEntity.addEntity(mtx);
            EntitySystemHandle handle2 = testEntity.addEntity(mtx);
            EntitySystemHandle handle3 = testEntity.addEntity(mtx);
            EntitySystemHandle handle4 = testEntity.addEntity(mtx);
            EntitySystemHandle handle5 = testEntity.addEntity(mtx);

            testEntity.addHeritaged2Component(handle1, Heritaged2{ .tempFloat2 = 234234.40 });
            testEntity.addHeritaged1Component(handle1, Heritaged1{ .tempInt = 78 });
            testEntity.addHeritaged2Component(handle2, Heritaged2{ .tempInt2 = 1234, .tempFloat2 = 1234 });
            //testEntity.addHeritaged2Component(handle3, Heritaged21{});
            testEntity.addHeritaged1Component(handle4, Heritaged1{ .tempFloat = 2304.04f });
            testEntity.addHeritaged2Component(handle5, Heritaged2{});

            testEntity.releaseLockedMutexHandle(mtx);
        }
        testEntity.syncReadWrites(); // Needs sync to not assert for read/write + adding

        auto testEntityReadCompArrayHandle = testEntity.getComponentArrayHandleBuilder()
            .addComponent(ComponentType::HeritagedType); // Will assert if trying to access heritaged1readarray without inserting this to read accessor
            // .addComponent(ComponentType::HeritagedType2); // Will assert when trying to read and write to same array without syncing

        auto testEntityWriteCompArrayHandle = testEntity.getComponentArrayHandleBuilder()
            .addComponent(ComponentType::HeritagedType2); // Will assert if trying to access Heritaged2WriteArray without inserting this to write accessors

        const auto &readWriteHandle1 = testEntity.getRWHandle(testEntityReadCompArrayHandle, testEntityWriteCompArrayHandle);
        testEntity.syncReadWrites();

        const auto &readWriteHandle2 = testEntity.getRWHandle(testEntityReadCompArrayHandle, testEntityWriteCompArrayHandle);
        {
            //auto ptr = testEntity.getHeritaged1ReadArray(readWriteHandle1); // Should assert using old sync point handle
            auto ptr = testEntity.getHeritaged1ReadArray(readWriteHandle2);
            u32 entityCount = testEntity.getEntityCount();
            for(u32 i = 0; i < entityCount; ++i)
            {
                LOG("%u: %s.tempInt:%i, %s.tempFloat:%f\n", i, ptr->componentName, ptr->tempInt,
                    ptr->componentName, ptr->tempFloat);
                //ptr->tempInt += 1;
                ++ptr;
            }
            //auto ptrRef = testEntity.getHeritaged2WriteArray(readWriteHandle1); // Should assert using old sync point handle
            auto ptrRef = testEntity.getHeritaged2WriteArray(readWriteHandle2);
            for(u32 i = 0; i < entityCount; ++i)
            {
                ptrRef->tempInt += 239 + i;
                ptrRef->tempInt2 += 1;
                ptrRef->tempFloat2 += 0.5f;
                LOG("REF%u: %s.tempInt:%i, %s.tempInt2:%i, %s.tempFloat2:%f\n", i,
                    ptrRef->componentName, ptrRef->tempInt,
                    ptrRef->componentName, ptrRef->tempInt2,
                    ptrRef->componentName, ptrRef->tempFloat2);
                ++ptrRef;
            }
            auto checkComponentsBoth = testEntity.getComponentArrayHandleBuilder()
                .addComponent(ComponentType::HeritagedType)
                .addComponent(ComponentType::HeritagedType2);

            auto checkComponents1 = testEntity.getComponentArrayHandleBuilder()
                .addComponent(ComponentType::HeritagedType);

            auto checkComponents2 = testEntity.getComponentArrayHandleBuilder()
                .addComponent(ComponentType::HeritagedType2);

            for(u32 i = 0; i < entityCount; ++i)
            {
                LOG("Has both components: %u, 1:%u, 2:%u\n", testEntity.hasComponents(
                    testEntity.getEntitySystemHandle(i), checkComponentsBoth),
                    testEntity.hasComponents(i, checkComponents1),
                    testEntity.hasComponents(i, checkComponents2)
                );
            }
        }
        testEntity.syncReadWrites();
        WriteJson writeJson1(1, 1);

        testEntity.serialize(writeJson1);
        writeJson1.finishWrite();

        JsonBlock json;
        const String &strJson = writeJson1.getString();
        bool parseSuccess = json.parseJson(StringView(strJson.data(), strJson.size()));
        if(!parseSuccess)
        {
            printf("Failed to parse writeJson1\n");
            return false;
        }

        StaticModelEntity testEntity2;
        {
            auto mtx = testEntity2.getLockedMutexHandle();
            // auto lck = testEntity2.getModifyLock();
            testEntity2.deserialize(json, mtx);
            auto entHandle = testEntity2.getEntitySystemHandle(3);
            testEntity2.removeEntity(entHandle, mtx);
            // testEntity2.removeEntity(entHandle, mtx); //Will assert trying to remove same entity
            EntitySystemHandle newHandle = testEntity2.addEntity(mtx);
            testEntity2.addHeritaged1Component(newHandle, Heritaged1{ .tempInt = 123 });

            testEntity2.releaseLockedMutexHandle(mtx);
        }

        OtherTestEntity otherEntity;
        {
            auto mtx = otherEntity.getLockedMutexHandle();
            EntitySystemHandle otherHandle = otherEntity.addEntity(mtx);
            {
                otherEntity.addHeritaged1Component(otherHandle, Heritaged1{ .tempV2{ 97, 23 } });
            }
            LOG("has comp: %u\n", testEntity2.hasComponent(
                testEntity2.getEntitySystemHandle(1), ComponentType::HeritagedType));
            otherEntity.releaseLockedMutexHandle(mtx);
        }
    }


    {
        TestSystem::init(entitySystems);
        TestSystem2::init(entitySystems);
    }

    return resized();
}


void SerializeComponent::updateText(StringView str)
{
    String tmpStr = "w";
    tmpStr.append(int32_t(fontSize.x));
    tmpStr.append(",h");
    tmpStr.append(int32_t(fontSize.y));

    fontSystem.addText(tmpStr.getStr(), Vector2(100.0f, 400.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    fontSystem.addText(String(str.ptr, str.length).getStr(), Vector2(100.0f, 100.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

bool SerializeComponent::resized()
{
    // create color and depth images
    if(!createRenderTargetImage(vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", renderColorImage))
    {
        printf("Failed to create font render target image!\n");
        return false;
    }
    fontSystem.setRenderTarget(renderColorImage);
    return true;
}

void SerializeComponent::logicUpdate()
{
    VulkanApp::logicUpdate();

    TestSystem::update(entitySystems, getDeltaTime());
    entitySystems.syncPoints();

    TestSystem2::update(entitySystems, getDeltaTime());
    entitySystems.syncPoints();

    TestSystem3::update(entitySystems);
    entitySystems.syncPoints();
    {
        bool textNeedsUpdate = false;
        for (int i = 0; i < bufferedPressesCount; ++i)
        {
            text.append(char(bufferedPresses[i]));
            textNeedsUpdate = true;
        }

        if (keyDowns[GLFW_KEY_LEFT].isDown)
        {
            fontSize.x--;
            if (fontSize.x < 2)
                ++fontSize.x;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_RIGHT].isDown)
        {
            fontSize.x++;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_DOWN].isDown)
        {
            fontSize.y++;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_UP].isDown)
        {
            fontSize.y--;
            if (fontSize.y < 2)
                ++fontSize.y;
            textNeedsUpdate = true;
        }

        updateText(text.getStr());
    }
}

void SerializeComponent::renderUpdate()
{
    VulkanApp::renderUpdate();
}

void SerializeComponent::renderDraw()
{
    prepareToGraphicsSampleWrite(renderColorImage);
    fontSystem.render();

    present(renderColorImage);
}

int main(int argCount, char **argv)
{
    initMemory();
    {
        SerializeComponent app;
        if(app.init("Serialize component test", SCREEN_WIDTH, SCREEN_HEIGHT,
            {
                .showInfoMessages = false,
                .useHDR = false,
                .useIntegratedGpu = true,
                .useValidationLayers = true,
                .useVulkanDebugMarkersRenderDoc = false,
                .vsync = VSyncType::IMMEDIATE_NO_VSYNC
            }))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}