
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

        // Needs something like getLockGuard / getLock
        {
            // auto lck = testEntity.getModifyLockGuard();

            EntitySystemHandle handle1 = testEntity.addEntity();
            EntitySystemHandle handle2 = testEntity.addEntity();
            EntitySystemHandle handle3 = testEntity.addEntity();
            EntitySystemHandle handle4 = testEntity.addEntity();
            EntitySystemHandle handle5 = testEntity.addEntity();

            testEntity.addHeritaged2Component(handle1, Heritaged2{.tempFloat2 = 234234.40});
            testEntity.addHeritaged1Component(handle1, Heritaged1{.tempInt = 78});
            testEntity.addHeritaged2Component(handle2, Heritaged2{.tempInt2 = 1234, .tempFloat2 = 1234});
            //testEntity.addHeritaged2Component(handle3, Heritaged21{});
            testEntity.addHeritaged1Component(handle4, Heritaged1{.tempFloat = 2304.04f});
            testEntity.addHeritaged2Component(handle5, Heritaged2{});
        }

        {
            auto ptr = testEntity.gether1ReadArray();
            u32 entityCount = testEntity.getEntityCount();
            for(u32 i = 0; i < entityCount; ++i)
            {
                LOG("%u: Heri1.tempInt:%i, Heri1.tempFloat:%f\n", i, ptr->tempInt, ptr->tempFloat);
                //ptr->tempInt += 1;
                ++ptr;
            }

            auto ptrRef = testEntity.gether2WriteArray();
            for(u32 i = 0; i < entityCount; ++i)
            {
                ptrRef->tempInt += 239 + i;
                ptrRef->tempInt2 += 1;
                ptrRef->tempFloat2 += 0.5f;
                LOG("REF%u: Heri2.tempInt:%i, Heri2.tempInt2:%i, Heri2.tempFloat2:%f\n", i, ptrRef->tempInt, ptrRef->tempInt2, ptrRef->tempFloat2);
                ++ptrRef;
            }
        }

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
            // auto lck = testEntity2.getModifyLock();
            testEntity2.deserialize(json);
            testEntity2.removeEntity(testEntity2.getEntitySystemHandle(1));
            EntitySystemHandle newHandle = testEntity2.addEntity();
            testEntity2.addHeritaged1Component(newHandle, Heritaged1{.tempInt = 123});

            // testEntity2.releaseModifyLock(lck);
        }

        OtherTestEntity otherEntity;
        EntitySystemHandle otherHandle = otherEntity.addEntity();
        {
            otherEntity.addHeritaged1Component(otherHandle, Heritaged1{.tempV2{97,23}});
        }
        LOG("has comp: %u\n", testEntity2.hasComponent(
            testEntity2.getEntitySystemHandle(1), ComponentType::HeritagedType));

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