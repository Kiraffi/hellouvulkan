
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
#include <gui/editorsystem.h>

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

#include <atomic>
#include <string.h>

using AtomicType = uint64_t;

static constexpr int SCREEN_WIDTH = 1024;
static constexpr int SCREEN_HEIGHT = 768;

static constexpr int SHADOW_WIDTH = 2048;
static constexpr int SHADOW_HEIGHT = 2048;

static constexpr ma_format DEVICE_FORMAT = ma_format_f32;
static constexpr int DEVICE_CHANNELS = 2;
static constexpr int DEVICE_SAMPLE_RATE = 48000;

static constexpr AtomicType NOTE_COUNT = 32;

static constexpr int SAMPLE_POINTS = 16;

static double getFreq(double index)
{
    return 220.0 * pow(2, index / 12.0);
}

static AtomicType Keys [NOTE_COUNT] = {

    GLFW_KEY_Z,
    GLFW_KEY_S,
    GLFW_KEY_X,
    GLFW_KEY_C,
    GLFW_KEY_F,
    GLFW_KEY_V,
    GLFW_KEY_G,
    GLFW_KEY_B,
    GLFW_KEY_N,
    GLFW_KEY_J,
    GLFW_KEY_M,
    GLFW_KEY_K,
    GLFW_KEY_COMMA,
    GLFW_KEY_L,
    GLFW_KEY_PERIOD,
    GLFW_KEY_SLASH,



    GLFW_KEY_Q,
    GLFW_KEY_2,
    GLFW_KEY_W,
    GLFW_KEY_E,
    GLFW_KEY_4,
    GLFW_KEY_R,
    GLFW_KEY_5,
    GLFW_KEY_T,
    GLFW_KEY_Y,
    GLFW_KEY_7,
    GLFW_KEY_U,
    GLFW_KEY_8,
    GLFW_KEY_I,
    GLFW_KEY_9,
    GLFW_KEY_O,
    GLFW_KEY_P,
};

static double Freqs [NOTE_COUNT] = {
    getFreq(0),
    getFreq(1),
    getFreq(2),
    getFreq(3),
    getFreq(4),
    getFreq(5),
    getFreq(6),
    getFreq(7),
    getFreq(8),
    getFreq(9),
    getFreq(10),
    getFreq(11),
    getFreq(12),
    getFreq(13),
    getFreq(14),
    getFreq(15),


    getFreq(12),
    getFreq(13),
    getFreq(14),
    getFreq(15),
    getFreq(16),
    getFreq(17),
    getFreq(18),
    getFreq(19),
    getFreq(20),
    getFreq(21),
    getFreq(22),
    getFreq(23),
    getFreq(24),
    getFreq(25),
    getFreq(26),
    getFreq(27),
};


enum class NotePlayPhase
{
    Attack,
    Decay,
    Sustain,
    Release,
    Finished,
    Amount,
};

struct NoteFromMainToThread
{
    AtomicType note;
    float amplitudes[SAMPLE_POINTS];
    float tuning[SAMPLE_POINTS];
    int instrument;
    float duration;
};

struct NoteFromThreadToMain
{
    double endTime;
};

struct NoteThread
{
    double startTime = 0.0;
    double releaseStart = 0.0;

    double decStart = 0.0;
    NotePlayPhase phase = NotePlayPhase::Finished;
};

NoteFromMainToThread notesFromMain [NOTE_COUNT] = { };
NoteThread notesThread [NOTE_COUNT] = { };
NoteFromThreadToMain notesFromThread [NOTE_COUNT] = { };

std::atomic<AtomicType> notesRunning(0);
std::atomic<AtomicType> notesReleased(0);


static void checkNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysDown, double time)
{
    AtomicType channelAtom = AtomicType(1) << channel;
    bool runn = (running & channelAtom) != 0;
    bool rel = (released & channelAtom) != 0;
    if(runn && !rel)
    {
        NoteFromMainToThread &note = notesFromMain[channel];

        AtomicType atomKey = AtomicType(1) << note.note;
        bool down = (keysDown & atomKey) != 0;

        // remove from handled keys
        keysDown &= ~(atomKey);

        if(!down)
        {
            std::atomic_fetch_or(&notesReleased, channelAtom);
        }
    }
}

static void addNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysDown,
    double time, const NoteFromMainToThread &currentNote)
{
    AtomicType channelAtom = AtomicType(1) << channel;
    bool runn = (running & channelAtom) != 0;
    if(keysDown == 0 || runn)
        return;

    AtomicType bitScanForward = 0;

    while((keysDown & (AtomicType(1) << bitScanForward)) == 0)
        bitScanForward++;
    if(bitScanForward >= NOTE_COUNT)
        return;

    // remove from handled keys
    keysDown &= ~(AtomicType(1) << bitScanForward);
    NoteFromMainToThread &note = notesFromMain[channel];

    if(time >= notesFromThread[channel].endTime)
    {
        note = currentNote;
        note.note = bitScanForward;
        std::atomic_fetch_or(&notesRunning, channelAtom);
        return;
    }
}

double evaluateSound(double time, double freq, int instrument)
{
    double fqSampPoint = freq * time;
    double samp = fqSampPoint * 2.0 * Pi;
    switch(instrument)
    {
        case 0:
            return sin(samp);
        break;
        case 1:
            return (sin(samp) > 0 ? 0.5 : -0.5);
        break;
        case 2:
            return (ffmodd(fqSampPoint, 1.0) - 0.5);
        break;
        case 3:
            return sin(samp) + (ffmodd(freq * ( 0.5 + time), 1.0) - 0.5);
        break;
        default:

        break;

    }
    return double(rand()) / double(RAND_MAX);

}

//#include <chrono>
//std::chrono::high_resolution_clock::time_point tp;
static void soundCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    //std::chrono::high_resolution_clock::time_point chTime = std::chrono::high_resolution_clock::now();
    //printf("time: %f, frames: %u\n", (chTime - tp).count() / 100000.0f, frameCount);
    //tp = chTime;
    static uint64_t threadFrameCounter = 0;
    double offset = *( double * )pDevice->pUserData;
    AtomicType threadNotesRunning = notesRunning.load();
    AtomicType threadNotesReleased = notesReleased.load();

    AtomicType newThreadsFinished = 0u;

    double dur = 1.0 / DEVICE_SAMPLE_RATE;

    for(AtomicType j = 0; j < NOTE_COUNT; ++j)
    {
        NoteThread &noteThread = notesThread [j];
        NoteFromMainToThread &noteMain = notesFromMain [j];
        bool running = ( ( threadNotesRunning >> j ) & 1 ) == 1;
        bool released = ( ( threadNotesReleased >> j ) & 1 ) == 1;

        if(running && released && noteThread.releaseStart < noteThread.startTime)
            noteThread.releaseStart = threadFrameCounter * dur;

        if(noteThread.phase == NotePlayPhase::Finished && running)
        {
            noteThread.startTime = threadFrameCounter * dur;
            noteThread.phase = NotePlayPhase::Release;
        }

    }

    float *f32Out = (float *)pOutput;
    for(uint32_t i = 0; i < frameCount; ++i)
    {
        double value = 0.0;
        double time = dur * threadFrameCounter;
        for(AtomicType j = 0; j < NOTE_COUNT; ++j)
        {
            if(( ( threadNotesRunning >> j ) & 1 ) == 0)
                continue;
            NoteFromMainToThread &noteMain = notesFromMain[j];
            NoteThread &noteThread = notesThread [j];
            NoteFromThreadToMain &noteToMain = notesFromThread [j];

            double tmpValue = 0.0;
            double amplitude = 0.0;
            if(time >= noteThread.startTime && noteThread.phase != NotePlayPhase::Finished)
            {
                double timePoint = time - noteThread.startTime;
                double value = timePoint / noteMain.duration;
                value *= double(SAMPLE_POINTS);
                int iValue = value < (SAMPLE_POINTS - 1) ? value : (SAMPLE_POINTS - 1);
                int iValue2 = value < (SAMPLE_POINTS - 1) ? value + 1 : (SAMPLE_POINTS - 1);
                float lerping = ffmodf(value, 1.0f);
                value = noteMain.tuning[iValue] * (1.0f - lerping) + noteMain.tuning[iValue2] * lerping;
                double freq = Freqs[noteMain.note];

                amplitude = noteMain.amplitudes[iValue] * (1.0f - lerping) + noteMain.amplitudes[iValue2] * lerping;
                tmpValue = evaluateSound(timePoint, freq, noteMain.instrument);
                tmpValue = evaluateSound(timePoint, value, 0)  * tmpValue;

/*                tmpValue = evaluateSound(timePoint, freq * tmpValue * 2.0, noteMain.instrument);
*/
                //horn

               /*
               nice sound
                tmpValue = evaluateSound(timePoint, freq * 0.5, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.instrument)  * tmpValue;
                */

/*                tmpValue = evaluateSound(timePoint, freq * 0.01, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.instrument)  * tmpValue;
*/
/*
                tmpValue = evaluateSound(timePoint, 30, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.instrument)  * tmpValue;
*/
                //tmpValue = evaluateSound(timePoint, freq * tmpValue, 1);
                //tmpValue *= evaluateSound(timePoint + 0.0, 220.0, 2);
            }
            value += tmpValue * amplitude;

            switch(noteThread.phase)
            {
                // NOTE: The fall through is on purpose, so if there is no decrease time or something.
                case NotePlayPhase::Release:
                    if(/*noteThread.releaseStart > noteThread.startTime || */time < noteThread.startTime + noteMain.duration )
                        break;
                    noteThread.phase = NotePlayPhase::Finished;
                    noteToMain.endTime = time + offset;
                    newThreadsFinished |= AtomicType(1) << j;
                    threadNotesRunning &= ~( AtomicType(1) << j );
                case NotePlayPhase::Finished:
                case NotePlayPhase::Amount:
                default:
                    break;
            }
        }
        value = ffclampd(-1.0, 1.0, value);
        for(uint32_t j = 0; j < DEVICE_CHANNELS; ++j)
            f32Out[i * DEVICE_CHANNELS + j] = value;

        ++threadFrameCounter;
    }

    if(newThreadsFinished)
    {
        newThreadsFinished = ~newThreadsFinished;
        std::atomic_fetch_and(&notesRunning, newThreadsFinished);
        std::atomic_fetch_and(&notesReleased, newThreadsFinished);
    }
    //printf("start: %f, end: %f, samplecount: %u\n", float(lastPos), float(startPos), frameCount);
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

    SoundTest() : scene(meshRenderSystem), editorSystem(scene, lineRenderSystem, SCREEN_WIDTH, SCREEN_HEIGHT) { }

    virtual ~SoundTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    EditorSystem editorSystem;

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

    double startTime;

    NoteFromMainToThread currentNote;
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
    currentNote.note = 0;
    currentNote.instrument = 0;
    currentNote.duration = 0.5;
    for(uint32_t i = 0; i < SAMPLE_POINTS; ++i)
    {
        currentNote.amplitudes[i] = 0.125 * 0.25;
        if(i < 4)
            currentNote.amplitudes[i] *= i * 4 / double(SAMPLE_POINTS);
        if(i > 12)
            currentNote.amplitudes[i] *= (15 - i) * 4 / double(SAMPLE_POINTS);
        currentNote.tuning[i] = evaluateSound(double(i) / (SAMPLE_POINTS - 1), 100.0, 0);
        currentNote.tuning[i] = double(i) / (SAMPLE_POINTS - 1);
    }
    for(AtomicType i = 0; i < NOTE_COUNT; ++i)
    {
        notesThread[i].phase = NotePlayPhase::Finished;
        notesThread[i].releaseStart = 0.0;
        notesThread[i].startTime = 0.0;

        notesFromThread[i].endTime = 0.0;

        /*
        notesFromMain[i].note = i;

        notesFromMain[i].attackAmplitude = 0.15;
        notesFromMain[i].sustainAmplitude = 0.10;

        notesFromMain[i].attackDur = 0.2;
        notesFromMain[i].decDur = 0.05;
        notesFromMain[i].releaseDur = 0.2;
        */
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

    if(!editorSystem.init(window))
        return false;

    {
        //sineWaveConfig = ma_waveform_config_init(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE, ma_waveform_type_sine, 0.2, 220);
        //ma_waveform_init(&sineWaveConfig, &sineWave);

        startTime = getTime();
        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format    = DEVICE_FORMAT;
        deviceConfig.playback.channels  = DEVICE_CHANNELS;
        deviceConfig.sampleRate         = DEVICE_SAMPLE_RATE;
        deviceConfig.dataCallback       = soundCallback;
        deviceConfig.pUserData          = &startTime;
        deviceConfig.performanceProfile = ma_performance_profile_low_latency;

        if (ma_device_init(NULL, &deviceConfig, &soundDevice) != MA_SUCCESS) {
            printf("Failed to open playback device.\n");
            return false;
        }

        printf("Device Name: %s\n", soundDevice.playback.name);

        if (ma_device_start(&soundDevice) != MA_SUCCESS) {
            printf("Failed to start playback device.\n");
            ma_device_uninit(&soundDevice);
            return false;
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

    editorSystem.resizeWindow(meshRenderTargets.albedoImage);

    return true;
}

void SoundTest::logicUpdate()
{
    VulkanApp::logicUpdate();
    lineRenderSystem.clear();
    MouseState mouseState = getMouseState();

    editorSystem.logicUpdate(*this);

    if(!editorSystem.guiHasFocus())
    {

    }
    //checkCameraKeypresses(dt, camera);

    if(isPressed(GLFW_KEY_KP_ADD))
        ++currentNote.instrument;
    if(isPressed(GLFW_KEY_KP_SUBTRACT))
        --currentNote.instrument;
    //if(currentNote.instrument < 0) chosenInstrument = 0;
    //if(chosenInstrument > 4) chosenInstrument = 4;

    double currTime = getTime();
    AtomicType runnings = notesRunning.load();
    AtomicType releases = notesReleased.load();

    AtomicType keysDown = 0;
    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
        if(isDown(Keys[index]))
            keysDown |= AtomicType(1) << index;

    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
        checkNotes(index,  runnings, releases, keysDown, currTime);

    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
        addNotes(index, runnings, releases, keysDown, currTime, currentNote);


    camera.lookAt(Vec3(0, 0, 0));
    sunCamera.lookAt(Vec3(0, 0, 0));

    if (isPressed(GLFW_KEY_LEFT_BRACKET))
        showNormalMap = !showNormalMap;

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.pitch >= 2.0f * PI) sunCamera.pitch -= 2.0f * PI;
    while (sunCamera.pitch <= 0.0f) sunCamera.pitch += 2.0f * PI;
    while (sunCamera.yaw >= 2.0f * PI) sunCamera.yaw -= 2.0f * PI;
    while (sunCamera.yaw <= 0.0f) sunCamera.yaw += 2.0f * PI;

    Vec2 renderPos = camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, use sun camera: %s, instrument: %i",
        showNormalMap ? "on" : "off", useSunCamera ? "on" : "off", currentNote.instrument);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    meshRenderSystem.clear();
}

static void drawSoundGui(NoteFromMainToThread &currentNote)
{
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sound");
    ImGui::DragFloat("Attack dur", &currentNote.duration, 0.01f, 0.0f, 10.0f);

    ImGui::PushID("set1");
    for(int i = 0; i < SAMPLE_POINTS; i++)
    {
        if(i > 0) ImGui::SameLine();
        ImGui::PushID(i);
        //ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4) ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4) ImColor::HSV(i / 7.0f, 0.6f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4) ImColor::HSV(i / 7.0f, 0.7f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4) ImColor::HSV(i / 7.0f, 0.9f, 0.9f));
        ImGui::VSliderFloat("##v", ImVec2(18, 160), &currentNote.tuning[i], -16.0f, 16.0f, "");
        if(ImGui::IsItemActive() || ImGui::IsItemHovered())
            ImGui::SetTooltip("%.3f", currentNote.tuning[i]);
        //ImGui::PopStyleColor(4);
        ImGui::PopID();
    }
    ImGui::PopID();

/*
    ImGui::DragFloat("Attack dur", &currentNote.attackDur, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Decrease dur", &currentNote.decDur, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Release dur", &currentNote.releaseDur, 0.01f, 0.0f, 10.0f);

    ImGui::DragFloat("Attack amplitude", &currentNote.attackAmplitude, 0.01f, 0.0f, 1.5f);
    ImGui::DragFloat("Sustain amplitude", &currentNote.sustainAmplitude, 0.01f, 0.0f, 1.5f);
*/
    ImGui::End();
    return;
}

void SoundTest::renderUpdate()
{
    VulkanApp::renderUpdate();

    scene.update(dt);

    editorSystem.renderUpdateGui();
    drawSoundGui(currentNote);

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
    {
        prepareToGraphicsSampleRead(meshRenderTargets.albedoImage);
        editorSystem.renderDraw();
    }
    Image &finalImage = editorSystem.getRenderTargetImage();
    present(finalImage);
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