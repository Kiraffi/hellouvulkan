
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
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include "components.h"

#include "../generated_header.h"

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;





static FORCE_INLINE Mat3x4 getMatrixFromTransform(const TransformComponent &trans)
{
    Mat3x4 result{ Uninit };
    float xy2 = 2.0f * trans.rotation.v.x * trans.rotation.v.y;
    float xz2 = 2.0f * trans.rotation.v.x * trans.rotation.v.z;
    float yz2 = 2.0f * trans.rotation.v.y * trans.rotation.v.z;

    float wx2 = 2.0f * trans.rotation.w * trans.rotation.v.x;
    float wy2 = 2.0f * trans.rotation.w * trans.rotation.v.y;
    float wz2 = 2.0f * trans.rotation.w * trans.rotation.v.z;

    float xx2 = 2.0f * trans.rotation.v.x * trans.rotation.v.x;
    float yy2 = 2.0f * trans.rotation.v.y * trans.rotation.v.y;
    float zz2 = 2.0f * trans.rotation.v.z * trans.rotation.v.z;

    result._00 = (1.0f - yy2 - zz2) * trans.scale.x;
    result._01 = (xy2 - wz2) * trans.scale.x;
    result._02 = (xz2 + wy2) * trans.scale.x;

    result._10 = (xy2 + wz2) * trans.scale.y;
    result._11 = (1.0f - xx2 - zz2) * trans.scale.y;
    result._12 = (yz2 - wx2) * trans.scale.y;

    result._20 = (xz2 - wy2) * trans.scale.z;
    result._21 = (yz2 + wx2) * trans.scale.z;
    result._22 = (1.0f - xx2 - yy2) * trans.scale.z;

    result._03 = trans.position.x;
    result._13 = trans.position.y;
    result._23 = trans.position.z;

    return result;
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


bool SerializeComponent::init(const char* windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
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

            testEntity.addHeritaged2Component(handle1, Heritaged2{.tempFloat2 = 234234.40});
            testEntity.addHeritaged1Component(handle1, Heritaged1{.tempInt = 78});
            testEntity.addHeritaged2Component(handle2, Heritaged2{.tempInt2 = 1234, .tempFloat2 = 1234});
            //testEntity.addHeritaged2Component(handle3, Heritaged21{});
            testEntity.addHeritaged1Component(handle4, Heritaged1{.tempFloat = 2304.04f});
            testEntity.addHeritaged2Component(handle5, Heritaged2{});

            testEntity.releaseLockedMutexHandle(mtx);
        }
        testEntity.syncReadWrites(); // Needs sync to not assert for read/write + adding

        auto testEntityReadCompArrayHandle = testEntity.getComponentArrayHandleBuilder()
            .addComponent(ComponentType::HeritagedType); // Will assert if trying to access heritaged1readarray without inserting this to read accessor
            // .addComponent(ComponentType::HeritagedType2); // Will assert when trying to read and write to same array without syncing

        auto testEntityWriteCompArrayHandle = testEntity.getComponentArrayHandleBuilder()
            .addComponent(ComponentType::HeritagedType2); // Will assert if trying to access Heritaged2WriteArray without inserting this to write accessors

        const auto &readWriteHandle1 = testEntity.getReadWriteHandle(testEntityReadCompArrayHandle, testEntityWriteCompArrayHandle);
        testEntity.syncReadWrites();

        const auto &readWriteHandle2 = testEntity.getReadWriteHandle(testEntityReadCompArrayHandle, testEntityWriteCompArrayHandle);
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
            testEntity2.addHeritaged1Component(newHandle, Heritaged1{.tempInt = 123});

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