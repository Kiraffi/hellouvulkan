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
#include <app/glfw_keys.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <app/vulkan_app.h>
#include <core/writejson.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/matrix_inline_functions.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include <render/myimguirenderer.h>

#include <imgui.h>

static constexpr i32 SCREEN_WIDTH = 640;
static constexpr i32 SCREEN_HEIGHT = 540;


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
    OtherTestEntity otherEntitySystem;
};

bool EntitySystems::syncPoints()
{
    return gameEntitySystem.syncReadWrites() && otherEntitySystem.syncReadWrites();
}

bool TestSystem::init(EntitySystems &entitySystems)
{
    GameEntitySystem &gameEnts = entitySystems.gameEntitySystem;
    {
        auto mtx = entitySystems.gameEntitySystem.getLockedMutexHandle();

        EntitySystemHandle handle1 = gameEnts.addEntity(mtx);
        EntitySystemHandle handle2 = gameEnts.addEntity(mtx);

        gameEnts.addTransformComponent(handle1, TransformComponent{.position = Vector4{1, 2, 3, 1 }});
        gameEnts.addTransformComponent(handle2, TransformComponent{.position = Vector4{4, 5, 6, 1 }});

        gameEnts.addMat4Component(handle1, {});
        gameEnts.addMat4Component(handle2, {});

        gameEnts.addCameraComponent(handle1, {});

        gameEnts.releaseLockedMutexHandle(mtx);
    }

    OtherTestEntity &otherEnts = entitySystems.otherEntitySystem;
    {
        auto mtx = otherEnts.getLockedMutexHandle();

        EntitySystemHandle handle1 = otherEnts.addEntity(mtx);
        EntitySystemHandle handle2 = otherEnts.addEntity(mtx);

        otherEnts.addHeritaged1Component(handle1, {});
        otherEnts.addHeritaged1Component(handle2, {});
        otherEnts.releaseLockedMutexHandle(mtx);

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

        gameEnts.addTransformComponent(handle1, TransformComponent{ .position = Vector4{ 10, 20, 30, 0 } });
        //gameEnts.addTransformComponent(handle2, TransformComponent{ .position = Vector4{ 40, 50, 60, 0 } });

        //gameEnts.addMat4Component(handle1, {});
        gameEnts.addMat4Component(handle2, {});

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
    const u64* components = gameEnts.getComponentsReadArray();
    if(transformComponents == nullptr || components == nullptr)
        return false;
    u64 requiredComponents = gameEntsRWHandle.readArrays | gameEntsRWHandle.writeArrays;

    for(u32 i = 0; i < gameEntCount; ++i)
    {
        //if(!gameEnts.hasComponents(i, gameEntsRWHandle))
        //    continue;
        if((components[i] & requiredComponents) != requiredComponents)
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
        .addComponent(ComponentType::Mat3x4Component)
        .addComponent(ComponentType::CameraComponent);

    const auto &gameEntsRWHandle = gameEnts.getRWHandle(gameEntsReadComponents, {});
    u32 gameEntCount = gameEnts.getEntityCount();

    const TransformComponent *transformComponents = gameEnts.getTransformComponentReadArray(gameEntsRWHandle);
    const Mat4Component *matComponents = gameEnts.getMat4ComponentReadArray(gameEntsRWHandle);
    const CameraComponent* cameraComponents = gameEnts.getCameraComponentReadArray(gameEntsRWHandle);
    if(transformComponents == nullptr || matComponents == nullptr)
        return false;

    auto requiredComponents = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::TransformComponent)
        .addComponent(ComponentType::Mat3x4Component);

    auto camComponent = gameEnts.getComponentArrayHandleBuilder()
        .addComponent(ComponentType::CameraComponent);


    for(u32 i = 0; i < gameEntCount; ++i)
    {
        if(!gameEnts.hasComponents(i, requiredComponents))
            continue;
        const Vector4 &pos = transformComponents[i].position;
        const Mat3x4 &m = matComponents[i].mat;
        LOG("%u: pos[%.2f, %.2f, %.2f]\n", i, pos.x, pos.y, pos.z);
        LOG("[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n",
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11]);

        // Handle case when this object has camera component.
        if(gameEnts.hasComponents(i, camComponent))
        {
            const auto& cmp = cameraComponents[i];
            const Vec3 dirs = Vec3(cmp.pitch, cmp.yaw, cmp.roll);
            LOG("Camera component pitch: %.2f, yaw: %.2f, roll: %.2f\n", dirs.x, dirs.y, dirs.z);
        }
        //getMatrixFromTransform(transformComponents[i], matComponents[i].mat);
    }
    LOG("\n");
    return true;
}







class SerializeComponent : public VulkanApp
{
public:
    SerializeComponent() {}
    virtual ~SerializeComponent() override;

    virtual bool init(const char* windowStr, i32 screenWidth, i32 screenHeight) override;
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

    MyImguiRenderer imgui;

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


bool SerializeComponent::init(const char *windowStr, i32 screenWidth, i32 screenHeight)
{
    if(!VulkanApp::init(windowStr, screenWidth, screenHeight))
        return false;

    if (!imgui.init(window))
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

            testEntity.addHeritaged2Component(handle1, Heritaged2Component{ .tempFloat2 = 234234.40 });
            testEntity.addHeritaged1Component(handle1, Heritaged1Component{ .tempInt = 78 });
            testEntity.addHeritaged2Component(handle2, Heritaged2Component{ .tempInt2 = 1234, .tempFloat2 = 1234 });
            //testEntity.addHeritaged2Component(handle3, {});
            testEntity.addHeritaged1Component(handle4, Heritaged1Component{ .tempFloat = 2304.04f });
            testEntity.addHeritaged2Component(handle5, {});

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
            auto ptr = testEntity.getHeritaged1ComponentReadArray(readWriteHandle2);
            u32 entityCount = testEntity.getEntityCount();
            for(u32 i = 0; i < entityCount; ++i)
            {
                LOG("%u: %s.tempInt:%i, %s.tempFloat:%f\n", i, ptr->componentName, ptr->tempInt,
                    ptr->componentName, ptr->tempFloat);
                //ptr->tempInt += 1;
                ++ptr;
            }
            //auto ptrRef = testEntity.getHeritaged2WriteArray(readWriteHandle1); // Should assert using old sync point handle
            auto ptrRef = testEntity.getHeritaged2ComponentWriteArray(readWriteHandle2);
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
            testEntity2.addHeritaged1Component(newHandle, Heritaged1Component{ .tempInt = 123 });

            testEntity2.releaseLockedMutexHandle(mtx);
        }

        OtherTestEntity otherEntity;
        {
            auto mtx = otherEntity.getLockedMutexHandle();
            EntitySystemHandle otherHandle = otherEntity.addEntity(mtx);
            {
                otherEntity.addHeritaged1Component(otherHandle, Heritaged1Component{ .tempV2{ 97, 23 } });
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
    tmpStr.append(i32(fontSize.x));
    tmpStr.append(",h");
    tmpStr.append(i32(fontSize.y));

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

    imgui.updateRenderTarget(renderColorImage);

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
        for (i32 i = 0; i < bufferedPressesCount; ++i)
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
    imgui.renderBegin();


    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static i32 counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        //ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    {
        entitySystems.gameEntitySystem.imguiRenderEntity();
        entitySystems.otherEntitySystem.imguiRenderEntity();
        entitySystems.syncPoints();

        //entitySystems.gameEntitySystem.imguiRenderEntity();
        //entitySystems.syncPoints();

        TestEnum t;
        LOG("T: %u\n", t.enumValue = TestEnum::TestEnumValue2);

        switch(t.enumValue)
        {
            case TestEnum::TestEnumValue1:
            case TestEnum::TestEnumValue2:

            break;

            case TestEnum::EnumValueCount:
            break;
        }
    }
}

void SerializeComponent::renderDraw()
{
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    const VkClearColorValue color{0.0f, 0.0f, 0.0f, 1.0f};
    vulk->imageMemoryGraphicsBarriers.pushBack(imageBarrier(renderColorImage,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
    flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);//VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdClearColorImage(vulk->commandBuffer, renderColorImage.image, renderColorImage.layout,
        &color, 1, &range);
    prepareToGraphicsSampleWrite(renderColorImage);
    fontSystem.render();

    imgui.render();
    present(renderColorImage);
}

i32 main(i32 argCount, char **argv)
{
    initMemory();
    {
        SerializeComponent app;
        if(app.init("Serialize component test", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}