
#include <container/podvector.h>
#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <core/general.h>
#include <core/glfw_keys.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include "components.h"

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;







/*
class Heritaged : public SerializableClassBase
{
public:
    Heritaged() : SerializableClassBase() { objMagicNumber = magicNumberClass; objVersion = versionClass; }

    static constexpr int getClassMagicNumber() { return magicNumberClass; }
    static constexpr int getClassVersion() { return versionClass; }

    INT_FIELD(tempInt, 10);
    FLOAT_FIELD(tempFloat, 20.0f);

private:
    static constexpr int magicNumberClass = 12;
    static constexpr int versionClass = 2;
};

*/




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

void printClass(const SerializableClassBase &obj, const char *str)
{
    LOG("Obj: %s is: ", str);
    switch(obj.getObjMagicNumber())
    {
        /*
        case SerializableClassBase::getStaticClassMagicNumber():
        {
            LOG("Base class\n");
            break;
        }
        */
        case Heritaged::getStaticClassMagicNumber():
        {
            LOG("Heritaged class\n");
            break;
        }
        default:
        {
            LOG("Unknown class\n");
            break;
        }
    }
}


bool SerializeComponent::init(const char* windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    //SerializableClassBase base;
    Heritaged her;
    //LOG("Filenumber base: %i\n", base.getFileMagicNumber());
    LOG("Filenumber heritaged: %i\n", her.getFileMagicNumber());
    //LOG("base: class magic: %i, obj magic: %i, class version: %i, obj ver: %i\n", base.getClassMagicNumber(), base.getObjMagicNumber(), base.getClassVersion(), base.getObjVersion());
    LOG("heritaged: class magic: %i, obj magic: %i, class version: %i, obj ver: %i\n", her.getStaticClassMagicNumber(), her.getObjMagicNumber(), her.getStaticClassVersion(), her.getObjVersion());

    //printClass(base, "Base");
    printClass(her.getBase(), "Heritaged");

    LOG("TempInt value: %i, TempFloat value: %f\n", her.getTempInt(), her.getTempFloat());
    LOG("TempInt name: %s TempFloat name: %s\n", her.getTempIntString(), her.getTempFloatString());

    her.setTempInt(50935);
    her.setTempFloat(1023.40f);
    LOG("TempInt value: %i, TempFloat value: %f\n", her.getTempInt(), her.getTempFloat());
    LOG("TempInt index: %i, TempFloat index: %i\n", her.getTempIntIndex(), her.getTempFloatIndex());

    float *floatMemory = nullptr;
    int *intMemory = nullptr;
    FieldType type = FieldType::NumTypes;

    if(her.getMemoryPtr("TempInt", (void **)&intMemory, type))
    {
        LOG("add: %p, type: %i\n", intMemory, type);
        LOG("Int is: %i\n", *intMemory);
    }
    if(her.getMemoryPtr("TempFloat", (void **) &floatMemory, type))
    {
        LOG("add: %p, type: %i\n", floatMemory, type);
        LOG("Float is: %f\n", *floatMemory);
    }
    her.serialize();

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