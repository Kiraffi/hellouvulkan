
#include <components/transform_functions.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/glfw_keys.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <container/arraysliceview.h>

#include <gui/componentviews.h>

#include <math/general_math.h>
#include <math/hitpoint.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/ray.h>
#include <math/vector3.h>
#include <math/quaternion_inline_functions.h>

#include <model/gltf.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>


#include <render/convertrendertarget.h>
#include <render/lightrendersystem.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>
#include <render/tonemaprendersystem.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>


#include <scene/scene.h>

//#define MA_NO_DECODING
//#define MA_NO_ENCODING
//#define MINIAUDIO_IMPLEMENTATION
//#include <miniaudio.h>
#include <extras/miniaudio_split/miniaudio.h>

#include <mutex>
#include <string.h>

static constexpr int SCREEN_WIDTH = 1024;
static constexpr int SCREEN_HEIGHT = 768;

static constexpr int SHADOW_WIDTH = 2048;
static constexpr int SHADOW_HEIGHT = 2048;

static constexpr ma_format DEVICE_FORMAT = ma_format_f32;
static constexpr int DEVICE_CHANNELS = 2;
static constexpr int DEVICE_SAMPLE_RATE = 48000;

enum class NotePlayPhase
{
    Attack,
    Decay,
    Sustain,
    Release,
    Finished,
    Amount,
};

struct Note
{
    double attackAmplitude;
    double sustainAmplitude;
    double freq;

    double startTime;
    double lastSamplingTime;

    double attackDur;
    double decDur;
    double releaseDur;

    double decStart;
    double releaseStart;

    NotePlayPhase phase;
};

Note notes[16] = {};
std::mutex notesMutex;

double globalTime = 0.0;
double previousGlobalTime = 0.0;
double freq = 220.0;
static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    #if 0
    ma_waveform* pSineWave;

    ASSERT(pDevice->playback.channels == DEVICE_CHANNELS);

    pSineWave = (ma_waveform*)pDevice->pUserData;
    ASSERT(pSineWave != NULL);

    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);

    (void)pInput;   /* Unused. */
    #else

    Note myNotes[16];
    notesMutex.lock();
    memcpy(myNotes, notes, sizeof(Note) * 16);
    notesMutex.unlock();

    double dur = 1.0 / DEVICE_SAMPLE_RATE;
    static double lastPos = 0.0;
    double startPos = lastPos;

    float *f32Out = (float *)pOutput;
    for(uint32_t i = 0; i < frameCount; ++i)
    {
        double value = 0.0;
        for(uint32_t j = 0; j < 16; ++j)
        {
            Note &note = myNotes[j];
            double tmpValue = 0.0;
            double amplitude = 0.0;
            switch(note.phase)
            {
                case NotePlayPhase::Attack:
                    amplitude = note.attackAmplitude * fmin(1.0, (note.lastSamplingTime - note.startTime) / note.attackDur);
                    break;
                case NotePlayPhase::Decay:
                    amplitude = note.attackAmplitude -
                        (fmin(1.0, fmax(0.0, ((note.lastSamplingTime - note.decStart) / note.decDur))) *
                            (note.attackAmplitude - note.sustainAmplitude));
                    break;
                case NotePlayPhase::Release:
                    amplitude = fmax(0.0, (1.0f - (note.lastSamplingTime - note.releaseStart) / note.releaseDur)) * note.sustainAmplitude;
                    break;
                case NotePlayPhase::Sustain:
                    amplitude = note.sustainAmplitude;
                    break;
                case NotePlayPhase::Finished:
                    amplitude = 0.0;
                    break;
                case NotePlayPhase::Amount:
                    continue;
            }
            tmpValue = sin(note.freq * (note.lastSamplingTime - note.startTime) * 2.0 * Pi) * amplitude;
            value += tmpValue;

            note.lastSamplingTime += dur;
            switch(note.phase)
            {
                // NOTE: The fall through is on purpose, so if there is no decrease time or something.
                case NotePlayPhase::Attack:
                    if((note.lastSamplingTime - note.startTime) < note.attackDur)
                        break;
                    note.phase = NotePlayPhase::Decay;
                    note.decStart = note.startTime + note.attackDur;
                case NotePlayPhase::Decay:
                    if((note.lastSamplingTime - note.decStart) < note.decDur)
                        break;
                    note.phase = NotePlayPhase::Sustain;

                case NotePlayPhase::Sustain:
                    if(note.releaseStart < note.startTime)
                        break;
                    note.phase = NotePlayPhase::Release;

                case NotePlayPhase::Release:
                    if((note.lastSamplingTime - note.releaseStart) >= note.releaseDur)
                        note.phase = NotePlayPhase::Finished;
                    break;

                case NotePlayPhase::Finished:
                case NotePlayPhase::Amount:
                    break;
            }
        }
        for(uint32_t j = 0; j < DEVICE_CHANNELS; ++j)
            f32Out[i * DEVICE_CHANNELS + j] = value;
        startPos += dur * freq;

    }
    notesMutex.lock();
    memcpy(notes, myNotes, sizeof(Note) * 16);
    notesMutex.unlock();


    //printf("start: %f, end: %f, samplecount: %u\n", float(lastPos), float(startPos), frameCount);
    startPos  = fmod(startPos, 1.0);
    lastPos = startPos;

    #endif
}

static Vec3 getSunDirection(const Camera &camera)
{
    Vec3 sundir[3];
    getDirectionsFromPitchYawRoll(camera.pitch, camera.yaw, 0.0f, sundir[0], sundir[1], sundir[2]);

    return -sundir[2];
}

class SoundTest : public VulkanApp
{
public:
    SoundTest() : scene(meshRenderSystem) { }
    virtual ~SoundTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;
    LightRenderSystem lightRenderSystem;
    TonemapRenderSystem tonemapRenderSystem;

    LightingRenderTargets lightingRenderTargets;
    MeshRenderTargets meshRenderTargets;
    LineRenderSystem lineRenderSystem;

    ConvertRenderTarget convertFromS16{ VK_FORMAT_R16G16B16A16_SNORM };
    Vec2 fontSize{ 8.0f, 12.0f };

    bool showNormalMap = false;


    ma_waveform sineWave;
    ma_device_config deviceConfig;
    ma_device soundDevice;
    ma_waveform_config sineWaveConfig;


};


////////////////////////
//
// DEINIT
//
////////////////////////

SoundTest::~SoundTest()
{
    ma_device_uninit(&soundDevice);
}



bool SoundTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    for(uint32_t i = 0; i < 16; ++i)
    {
        notes[i].phase = NotePlayPhase::Finished;
        notes[i].freq = 220.0 * pow(2, double(i)/12.0);
        notes[i].attackAmplitude = 0.15;
        notes[i].sustainAmplitude = 0.10;

        notes[i].releaseStart = 0.0;
        notes[i].attackDur = 0.3;
        notes[i].decDur = 0.05;
        notes[i].releaseDur = 0.1;
    }

    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    // TEMPORARY!
    //glfwSetWindowPos(window, 2000, 100);

    if (!convertFromS16.init(ShaderType::ConvertFromRGBAS16))
        return false;

    if (!meshRenderSystem.init())
        return false;

    if (!scene.init())
        return false;

    if (!lightRenderSystem.init())
        return false;

    if (!tonemapRenderSystem.init())
        return false;

    if (!meshRenderTargets.resizeShadowTarget(SHADOW_WIDTH, SHADOW_HEIGHT))
        return false;

    if (!lineRenderSystem.init())
        return false;

    {
        sineWaveConfig = ma_waveform_config_init(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE, ma_waveform_type_sine, 0.2, 220);
        ma_waveform_init(&sineWaveConfig, &sineWave);

        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format   = DEVICE_FORMAT;
        deviceConfig.playback.channels = DEVICE_CHANNELS;
        deviceConfig.sampleRate        = DEVICE_SAMPLE_RATE;
        deviceConfig.dataCallback      = data_callback;
        deviceConfig.pUserData         = &sineWave;

        if (ma_device_init(NULL, &deviceConfig, &soundDevice) != MA_SUCCESS) {
            printf("Failed to open playback device.\n");
            return -4;
        }

        printf("Device Name: %s\n", soundDevice.playback.name);

        if (ma_device_start(&soundDevice) != MA_SUCCESS) {
            printf("Failed to start playback device.\n");
            ma_device_uninit(&soundDevice);
            return -5;
        }
    }


    scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR });

    for(float x = -10.0f; x <= 10.0f; x += 5.0f)
    {
        scene.addGameEntity({ .transform = { .pos = { x, 0.0f, -10.0f } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { x, 0.0f, 10.0f } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { -10, 0.0f, x } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { 10, 0.0f, x } }, .entityType = EntityType::TREE_SMOOTH });
    }

    sunCamera.pitch = toRadians(330.0f);
    sunCamera.yaw = toRadians(60.0f);

    camera.position = Vec3(0.0f, 20.0f, 30.0f);
    camera.fovY = 15.0f;
    previousGlobalTime = getTime();
    return resized();
}


bool SoundTest::resized()
{
    if (!meshRenderTargets.resizeMeshTargets())
        return false;
    if (!lightingRenderTargets.resizeLightingTargets())
        return false;

    meshRenderSystem.setRenderTargets(meshRenderTargets);
    lineRenderSystem.setRendertargets(meshRenderTargets.albedoImage, meshRenderTargets.depthImage);
    fontSystem.setRenderTarget(meshRenderTargets.albedoImage);
    convertFromS16.updateSourceImages(meshRenderTargets);

    lightRenderSystem.updateReadTargets(meshRenderTargets, lightingRenderTargets);
    tonemapRenderSystem.updateReadTargets(lightingRenderTargets.lightingTargetImage, meshRenderTargets.albedoImage);

    return true;
}

void checkStatus(Note &note, bool down, double time)
{
    if(!down && note.phase == NotePlayPhase::Finished)
        return;

    if(!down && uint32_t(note.phase) < uint32_t(NotePlayPhase::Release))
    {
        note.releaseStart = time;
    }
    if(down && note.phase == NotePlayPhase::Finished)
    {
        note.startTime = time;
        note.lastSamplingTime = time;
        if(note.attackDur > 0.0)
            note.phase = NotePlayPhase::Attack;
        else
            note.phase = NotePlayPhase::Sustain;
    }
}

void SoundTest::logicUpdate()
{
    VulkanApp::logicUpdate();
    lineRenderSystem.clear();
    globalTime = getTime();
    MouseState mouseState = getMouseState();

    //checkCameraKeypresses(dt, camera);
    // maybe mutex guard is better
    double currTime = getTime();
    notesMutex.lock();
    checkStatus(notes[0],  isDown(GLFW_KEY_Q), currTime);
    checkStatus(notes[1],  isDown(GLFW_KEY_2), currTime);
    checkStatus(notes[2],  isDown(GLFW_KEY_W), currTime);
    checkStatus(notes[3],  isDown(GLFW_KEY_E), currTime);
    checkStatus(notes[4],  isDown(GLFW_KEY_4), currTime);
    checkStatus(notes[5],  isDown(GLFW_KEY_R), currTime);
    checkStatus(notes[6],  isDown(GLFW_KEY_5), currTime);
    checkStatus(notes[7],  isDown(GLFW_KEY_T), currTime);
    checkStatus(notes[8],  isDown(GLFW_KEY_Y), currTime);
    checkStatus(notes[9],  isDown(GLFW_KEY_7), currTime);
    checkStatus(notes[10], isDown(GLFW_KEY_U), currTime);
    checkStatus(notes[11], isDown(GLFW_KEY_8), currTime);
    checkStatus(notes[12], isDown(GLFW_KEY_I), currTime);
    checkStatus(notes[13], isDown(GLFW_KEY_9), currTime);
    checkStatus(notes[14], isDown(GLFW_KEY_O), currTime);
    checkStatus(notes[15], isDown(GLFW_KEY_P), currTime);
    notesMutex.unlock();

    camera.lookAt(Vec3(0, 0, 0));
    sunCamera.lookAt(Vec3(0, 0, 0));

    if (isPressed(GLFW_KEY_P))
        showNormalMap = !showNormalMap;

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.pitch >= 2.0f * PI) sunCamera.pitch -= 2.0f * PI;
    while (sunCamera.pitch <= 0.0f) sunCamera.pitch += 2.0f * PI;
    while (sunCamera.yaw >= 2.0f * PI) sunCamera.yaw -= 2.0f * PI;
    while (sunCamera.yaw <= 0.0f) sunCamera.yaw += 2.0f * PI;

    Vec2 renderPos = camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, use sun camera: %s",
        showNormalMap ? "on" : "off", useSunCamera ? "on" : "off");
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    meshRenderSystem.clear();
}

void SoundTest::renderUpdate()
{
    VulkanApp::renderUpdate();

    scene.update(dt);
    meshRenderSystem.prepareToRender();

    Vec3 sundir = getSunDirection(sunCamera);
    lightRenderSystem.setSunDirection(sundir);

    lightRenderSystem.update();

    lineRenderSystem.prepareToRender();

}

void SoundTest::renderDraw()
{
    meshRenderTargets.prepareTargetsForMeshRendering();
    meshRenderTargets.prepareTargetsForShadowRendering();
    // Drawingg
    {
        meshRenderSystem.render(meshRenderTargets);
        meshRenderSystem.renderShadows(meshRenderTargets);
    }

    if (showNormalMap)
    {
        prepareToComputeSampleRead(meshRenderTargets.normalMapImage);

        prepareToComputeImageWrite(meshRenderTargets.albedoImage);

        convertFromS16.render(meshRenderTargets.albedoImage.width, meshRenderTargets.albedoImage.height);

    }
    else
    {
        {
            meshRenderTargets.prepareTargetsForLightingComputeSampling();
            lightingRenderTargets.prepareTargetsForLightingComputeWriting();

            lightRenderSystem.render(lightingRenderTargets.lightingTargetImage.width, lightingRenderTargets.lightingTargetImage.height);
        }

        {
            lightingRenderTargets.prepareForTonemapSampling();

            prepareToComputeImageWrite(meshRenderTargets.albedoImage);

            tonemapRenderSystem.render(meshRenderTargets.albedoImage.width, meshRenderTargets.albedoImage.height);

        }
    }

    prepareToGraphicsSampleWrite(meshRenderTargets.albedoImage);
    {
        prepareToGraphicsSampleWrite(meshRenderTargets.depthImage);
        lineRenderSystem.render(meshRenderTargets.albedoImage, meshRenderTargets.depthImage);
    }

    {
        fontSystem.render();
    }

    present(meshRenderTargets.albedoImage);
}

int main(int argCount, char **argv)
{
    SoundTest app;
    if (app.init("Sound test", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = false,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = true,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}