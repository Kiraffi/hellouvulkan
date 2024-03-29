
#include <audio/myaudio.h>

#include <components/transform_functions.h>

#include <container/podvector.h>
#include <container/podvectortypedefine.h>

#include <core/camera.h>
#include <core/file.h>
#include <core/general.h>
#include <app/glfw_keys.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <app/vulkan_app.h>
#include <core/writejson.h>

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

#include <mygui/mygui.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/convertrendertarget.h>
#include <render/lightrendersystem.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>
#include <render/tonemaprendersystem.h>
#include <render/viewport.h>


#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>

#include <scene/scene.h>

//#define MA_NO_DECODING
//#define MA_NO_ENCODING
//#define MINIAUDIO_IMPLEMENTATION
//#include <miniaudio.h>

//#include <string.h>


static constexpr i32 SCREEN_WIDTH = 1024;
static constexpr i32 SCREEN_HEIGHT = 768;

static constexpr i32 SHADOW_WIDTH = 2048;
static constexpr i32 SHADOW_HEIGHT = 2048;

static AtomicType Keys [NOTE_COUNT] =
{
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
static double getFreq(u32 index)
{
    return Supa::powd(2, double(index) / 12.0);
}

static double FreqTable[]
{
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
};

static const u32 GlobalKeyMap[NOTE_COUNT] =
{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,

    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
};



static Vec3 getSunDirection(const Camera &camera)
{
    Vec3 sundir[3];
    getDirectionsFromPitchYawRoll(camera.pitch, camera.yaw, 0.0f, sundir[0], sundir[1], sundir[2]);

    return -sundir[2];
}
static u32 SoundsMagicNumber = 3495835844;
static u32 SoundsVersionNumber = 1;


static float getFreqHz(u32 noteIndex)
{
    return 27.5f * float(1u << (noteIndex / 12)) * FreqTable[noteIndex % 12];
}

static bool loadNotes(const char *filename, NoteFromMainToThread *notes, u32 noteLen)
{
    if(!filename || !notes || noteLen == 0)
        return false;

    PodVector<char> buffer;

    if(!loadBytes(filename, buffer.getBuffer()))
        return false;

    JsonBlock json;
    bool parseSuccess = json.parseJson(StringView(buffer.data(), buffer.size()));

    if(!parseSuccess)
    {
        printf("Failed to parse: %s\n", filename);
        return false;
    }
    else
    {
        //json.print();
    }

    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    if(!json.getChild("magicNumber").equals(SoundsMagicNumber))
        return false;

    u32 versionNumber;
    if(!json.getChild("versionNumber").parseUInt(SoundsVersionNumber))
        return false;

    if(json.getChild("sounds").getChildCount() == 0)
        return false;

    if(json.getChild("sounds").getChildCount() > noteLen)
        return false;

    u32 i = 0;
    for(const auto &obj : json.getChild("sounds"))
    {
        auto &note = notes[i++];
        bool isValid = true;
        isValid &= obj.getChild("amplitudes").parseNumberArray(note.amplitudes, ARRAYSIZES(note.amplitudes));
        isValid &= obj.getChild("tunes").parseIntegerArray(note.tuning, ARRAYSIZES(note.tuning));

        isValid &= obj.getChild("AttackAmplitude").parseNumber(note.attackAmplitude);
        isValid &= obj.getChild("SustainAmplitude").parseNumber(note.sustainAmplitude);

        isValid &= obj.getChild("Osc LFOtype").parseInt(note.oscLFOType);
        isValid &= obj.getChild("Osc type").parseInt(note.oscType);

        isValid &= obj.getChild("Osc LFOHz").parseNumber(note.oscLFOHz);

        //isValid &= obj.getChild("Base HZ").parseNumber(note.baseHz);

        isValid &= obj.getChild("Attack duration").parseNumber(note.attackDuration);
        isValid &= obj.getChild("Decay duration").parseNumber(note.decayDuration);
        isValid &= obj.getChild("Release duration").parseNumber(note.releaseDuration);

        if(!isValid)
            return false;
    }
    return true;
}



static bool saveNotes(const char *filename, NoteFromMainToThread *notes, u32 noteLen)
{
    if(!filename || !notes || noteLen == 0)
        return false;

    WriteJson writeJson(SoundsMagicNumber, SoundsVersionNumber);
    writeJson.addArray("sounds");
    for(u32 i = 0; i < noteLen; ++i)
    {
        writeJson.addObject();
        const auto &note = notes[i];
        writeJson.addNumberArray("amplitudes", note.amplitudes, u32(ARRAYSIZES(note.amplitudes)));
        writeJson.addIntegerArray("tunes", note.tuning, u32(ARRAYSIZES(note.amplitudes)));

        writeJson.addNumber("AttackAmplitude", note.attackAmplitude);
        writeJson.addNumber("SustainAmplitude", note.sustainAmplitude);

        writeJson.addInteger("Osc LFOtype", note.oscLFOType);
        writeJson.addInteger("Osc type", note.oscType);

        writeJson.addNumber("Osc LFOHz", note.oscLFOHz);

        //writeJson.addNumber("Base HZ", note.baseHz);

        writeJson.addNumber("Attack duration", note.attackDuration);
        writeJson.addNumber("Decay duration", note.decayDuration);
        writeJson.addNumber("Release duration", note.releaseDuration);
        writeJson.endObject();
    }
    writeJson.endArray();
    writeJson.finishWrite();
    bool result = writeJson.isValid() &&
        writeBytes(filename, writeJson.getString().getBuffer());
    printf("Saving was: %s\n", result ? "succesful" : "failure");
    return result;
}

struct SongChannel
{
    PodVector<u32> notes;
};

struct SongPattern
{
    PodVector<u32> songPatternChannels;
};

struct Song
{
    Vector<SongPattern> songPatterns;
    Vector<SongChannel> songChannels;
    u32 bpm = 120;
    u32 beatsPerNote = 4;
    u32 notesPerPattern = 64;
    u32 channelsPerPattern = 4;
};

class SoundTest : public VulkanApp
{
public:

    SoundTest() : scene(meshRenderSystem), editorSystem(scene, lineRenderSystem, SCREEN_WIDTH, SCREEN_HEIGHT) { }

    virtual ~SoundTest() override;
    virtual bool init(const char* windowStr, i32 screenWidth, i32 screenHeight) override;
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


    NoteFromMainToThread notes[32];

    u32 currentNoteIndex = 0;
    i32 currentOctave = 4;

    Viewport viewport;

    Song song;
    i32 currentPatternIndex = 0;
    i32 currentRowIndex = 0;
    i32 currentColumnIndex = 0;

    u32 playingNoteChannels[NOTE_COUNT];

    PodVector<u32> playingMusicChannelNotes;

    u32 jumpRowAmount = 1u;
    bool editMode = false;
    bool playMode = false;
    double playStartTime = 0.0;
};


////////////////////////
//
// DEINIT
//
////////////////////////

SoundTest::~SoundTest()
{
    deinitAudio();
}

static void resetNote(NoteFromMainToThread &note)
{
    note.oscType = 0;
    note.oscLFOType = 0;
    note.oscLFOHz = 100.0;

    note.attackAmplitude = 0.2f;
    note.sustainAmplitude = 0.15f;

    note.attackDuration = 0.15f;
    note.decayDuration = 0.05f;
    note.releaseDuration = 0.2f;

    for(u32 i = 0; i < SAMPLE_POINTS; ++i)
    {
        note.amplitudes[i] = 0.125 * 0.25;

        if(i < 4)
            note.amplitudes[i] *= i * 4 / double(SAMPLE_POINTS);
        if(i > 12)
            note.amplitudes[i] *= (15 - i) * 4 / double(SAMPLE_POINTS);
        note.tuning[i] = evaluateSound(double(i) / (SAMPLE_POINTS - 1), 100.0, 0);
        note.tuning[i] = double(i) / (SAMPLE_POINTS - 1);
        note.tuning[i] = 0;
    }
}

static u32 addNewChannel(Song &song)
{
    song.songChannels.pushBack(SongChannel());
    SongChannel &channel = song.songChannels.back();
    channel.notes.resize(song.notesPerPattern, 0);
    return song.songChannels.size() - 1;
}

bool SoundTest::init(const char* windowStr, i32 screenWidth, i32 screenHeight)
{
    for(u32 i = 0; i < ARRAYSIZES(notes); ++i)
        resetNote(notes[i]);

    if (!VulkanApp::init(windowStr, screenWidth, screenHeight))
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

    if(!initAudio())
        return false;

    // init song
    {
        song.bpm = 120;
        for(u32 i = 0; i < 4; ++i)
            addNewChannel(song);
        song.songPatterns = { SongPattern {.songPatternChannels {0, 1, 2, 3}}};
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
    u32 wholeWidth = vulk->swapchain.width;
    u32 wholeHeight = vulk->swapchain.height;
    viewport = editorSystem.getEditorWindowViewport();
    windowWidth = u32(viewport.size.x);
    windowHeight = u32(viewport.size.y);

    if(!meshRenderTargets.resizeMeshTargets(windowWidth, windowHeight))
        return false;
    if(!lightingRenderTargets.resizeLightingTargets(windowWidth, windowHeight))
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
        ++notes[currentNoteIndex].oscType;
    if(isPressed(GLFW_KEY_KP_SUBTRACT))
        --notes[currentNoteIndex].oscType;

    if(isPressed(GLFW_KEY_KP_9))
        if(currentOctave < 6) ++currentOctave;
    if(isPressed(GLFW_KEY_KP_6))
        if(currentOctave > 0) --currentOctave;

    if(isPressed(GLFW_KEY_KP_8))
        ++jumpRowAmount;
    if(isPressed(GLFW_KEY_KP_6))
        if(jumpRowAmount > 0) --jumpRowAmount;

    if(isPressed(GLFW_KEY_KP_7))
        if(currentNoteIndex < ARRAYSIZES(notes)) ++currentNoteIndex;
    if(isPressed(GLFW_KEY_KP_4))
        if(currentNoteIndex > 0) --currentNoteIndex;

    if(isPressed(GLFW_KEY_S) && isDown(GLFW_KEY_LEFT_SHIFT))
    {
        saveNotes("assets/sound/notes.json", notes, ARRAYSIZES(notes));
    }
    if(isPressed(GLFW_KEY_L) && isDown(GLFW_KEY_LEFT_SHIFT))
    {
        loadNotes("assets/sound/notes.json", notes, ARRAYSIZES(notes));
    }

    if(isPressed(GLFW_KEY_UP))
        --currentRowIndex;

    if(isPressed(GLFW_KEY_DOWN))
        ++currentRowIndex;

    if(isPressed(GLFW_KEY_LEFT))
        --currentColumnIndex;

    if(isPressed(GLFW_KEY_RIGHT))
        ++currentColumnIndex;


    if(isPressed(GLFW_KEY_SPACE))
    {
        playMode = false;
        editMode = !editMode;
        for(u32 i = 0; i < playingMusicChannelNotes.size(); ++i)
        {
            releaseChannel(playingMusicChannelNotes[i]);
            playingMusicChannelNotes[i] = ~0u;
        }
    }
    double currTime = getTime();

    if(isPressed(GLFW_KEY_ENTER))
    {
        playMode = true;
        editMode = false;
        currentRowIndex = 0;
        playStartTime = currTime;
    }
    if(playingMusicChannelNotes.size() != song.channelsPerPattern)
        playingMusicChannelNotes.resize(song.channelsPerPattern, ~0u);

    while(currentRowIndex < 0)
        currentRowIndex += song.notesPerPattern;
    while(currentColumnIndex < 0)
        currentColumnIndex += song.channelsPerPattern;

    while(currentRowIndex >= song.notesPerPattern)
        currentRowIndex -= song.notesPerPattern;
    while(currentColumnIndex >= song.channelsPerPattern)
        currentColumnIndex -= song.channelsPerPattern;

    //if(currentNote.oscType < 0) chosenInstrument = 0;
    //if(chosenInstrument > 4) chosenInstrument = 4;

    AtomicType runnings = getRunningNotes();
    AtomicType releases = getReleasedNotes();

    AtomicType keysDown = 0;
    AtomicType keysUp = 0;

    if(playMode)
    {
        u32 prevPos = currentRowIndex;
        currentRowIndex = (currTime - playStartTime) * song.bpm / song.beatsPerNote;
        currentRowIndex %= song.notesPerPattern;
        if(prevPos != currentRowIndex || currTime == playStartTime)
        {
            auto &pattern = song.songPatterns[currentPatternIndex];
            for(u32 columnIndex = 0; columnIndex < song.channelsPerPattern; ++columnIndex)
            {
                auto &channel = song.songChannels[pattern.songPatternChannels[columnIndex]];
                auto &note = channel.notes[currentRowIndex];

                if(note != 0)
                {
                    u32 noteIndex = note >> 7u;
                    u32 freqIndex = (note - 1) & 127;
                    releaseChannel(playingMusicChannelNotes[columnIndex]);
                    playingMusicChannelNotes[columnIndex] = ~0u;

                    if(note != ~0u)
                        playingMusicChannelNotes[columnIndex] =
                            addNote(getFreqHz(freqIndex), notes[noteIndex]);
                }
            }
        }
    }


    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
    {
        if(isPressed(Keys[index]))
        {
            keysDown |= AtomicType(1) << index;
        }
        if(isReleased(Keys[index]))
            keysUp |= AtomicType(1) << index;
    }

    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
    {
        u32 note = currentOctave * 12 + GlobalKeyMap[index] + 1 + (currentNoteIndex << 7u);
        if(((keysUp >> index) & 1) == 1)
            releaseChannel(playingNoteChannels[index]);
    }

    for(AtomicType index = 0; index < NOTE_COUNT; ++index)
    {
        u32 note = currentOctave * 12 + GlobalKeyMap[index];
        if(((keysDown >> index) & 1) == 1)
            playingNoteChannels[index] =
                addNote(getFreqHz(note), notes[currentNoteIndex]);
    }

    if(keysDown && editMode)
    {
        auto &pattern = song.songPatterns[currentPatternIndex];
        auto &channel = song.songChannels[pattern.songPatternChannels[currentColumnIndex]];
        auto &note = channel.notes[currentRowIndex];

        AtomicType index = 0;
        for(AtomicType index = 0; index < 64; ++index)
        {
            if((keysDown & (AtomicType(1) << index)) == 0)
                continue;
            auto &note = channel.notes[currentRowIndex];
            note = 1 + (currentNoteIndex << 7) + (currentOctave * 12 + GlobalKeyMap[index]);
            if(editMode)
            {
                currentRowIndex += jumpRowAmount;
                currentRowIndex %= song.notesPerPattern;
            }
        }
    }
    if(editMode && isPressed(GLFW_KEY_CAPS_LOCK))
    {
        auto &pattern = song.songPatterns[currentPatternIndex];
        auto &channel = song.songChannels[pattern.songPatternChannels[currentColumnIndex]];
        auto &note = channel.notes[currentRowIndex];
        if(editMode)
        {
            currentRowIndex += jumpRowAmount;
            currentRowIndex %= song.notesPerPattern;
        }
        note = ~0u;
    }

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
        showNormalMap ? "on" : "off", useSunCamera ? "on" : "off", notes[currentNoteIndex].oscType);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    meshRenderSystem.clear();
}

static void drawSoundGui(NoteFromMainToThread &currentNote, u32 currentOctave, u32 noteIndex)
{
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sound");
    ImGui::Text("Sound index: %u", noteIndex);
    ImGui::Text("Octave: %u", currentOctave);
    ImGui::DragInt("Osc type", &currentNote.oscType, 1, 0, 4);
    ImGui::DragInt("Osc LFO type", &currentNote.oscLFOType, 1, 0, 4);
    ImGui::DragFloat("Osc LFO Hz", &currentNote.oscLFOHz, 1, 0.1, 10000);

    ImGui::DragFloat("Attack dur", &currentNote.attackDuration, 0.01f, 0.005f, 10.0f);
    ImGui::DragFloat("Decay dur", &currentNote.decayDuration, 0.01f, 0.005f, 4.0f);
    ImGui::DragFloat("Release dur", &currentNote.releaseDuration, 0.01f, 0.005f, 10.0f);

    ImGui::DragFloat("Attack amplitude", &currentNote.attackAmplitude, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("Sustain amplitude", &currentNote.sustainAmplitude, 0.01f, 0.0f, 2.0f);

    ImGui::PushID("set1");
    for(i32 i = 0; i < SAMPLE_POINTS; i++)
    {
        if(i > 0) ImGui::SameLine();
        ImGui::PushID(i);
        //ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4) ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4) ImColor::HSV(i / 7.0f, 0.6f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4) ImColor::HSV(i / 7.0f, 0.7f, 0.5f));
        //ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4) ImColor::HSV(i / 7.0f, 0.9f, 0.9f));
        ImGui::VSliderInt("##v", ImVec2(18, 160), &currentNote.tuning[i], -24, 24, "");
        if(ImGui::IsItemActive() || ImGui::IsItemHovered())
            ImGui::SetTooltip("%i", currentNote.tuning[i]);
        //ImGui::PopStyleColor(4);
        ImGui::PopID();
    }
    ImGui::PopID();


    ImGui::PushID("set2");
    for(i32 i = 0; i < SAMPLE_POINTS; i++)
    {
        if(i > 0) ImGui::SameLine();
        ImGui::PushID(i);
        ImGui::VSliderFloat("##v", ImVec2(18, 160), &currentNote.amplitudes[i], 0, 1, "");
        if(ImGui::IsItemActive() || ImGui::IsItemHovered())
            ImGui::SetTooltip("%f", currentNote.amplitudes[i]);
        ImGui::PopID();
    }
    ImGui::PopID();
    ImGui::End();



    return;
}

static void drawSongGui(Song &song, u32 currentPatternIndex, u32 currentRowIndex,
    u32 currentColumnIndex, bool editMode)
{
    ASSERT(song.songPatterns.size() > 0);
    currentPatternIndex = Supa::maxu32(currentPatternIndex, song.songPatterns.size() - 1);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Song");

    Vector<String> lines;
    u32 startRowIndex = Supa::maxu32(currentRowIndex, 16) - 16;
    u32 endRowIndex = Supa::minu32(startRowIndex + 33, song.notesPerPattern);
    startRowIndex = endRowIndex - 33;
    u32 rowAmount = endRowIndex - startRowIndex;
    lines.resize(rowAmount);

    currentRowIndex -= startRowIndex;

    for(u32 rowIndex = 0; rowIndex < rowAmount; ++rowIndex)
    {
        auto &line = lines[rowIndex];
        u32 lineIndex = rowIndex + startRowIndex;
        if(lineIndex < 10)
            line.append("0");
        line.append(lineIndex);
        line.append(" | ");
    }

    for(u32 channelIndex = 0; channelIndex < song.channelsPerPattern; ++channelIndex)
    {
        if(channelIndex >= song.songChannels.size())
            continue;

        auto &channel = song.songChannels[channelIndex];
        for(u32 rowIndex = 0; rowIndex < rowAmount; ++rowIndex)
        {
            auto &note = channel.notes[rowIndex + startRowIndex];
            auto &line = lines[rowIndex];

            if(note == 0)
                line.append("---- --");
            else if(note == ~0u)
                line.append("==== --");
            else
            {
                // cos note == 0 cannot exist...
                u32 octave = ((note + 9 - 1) & 127) / 12;
                u32 notenote = ((note - 1) & 127) % 12;
                u32 instrumentIndex = note >> 7;

                switch(notenote)
                {
                    case 0:  line.append("A-"); break;
                    case 1:  line.append("A#"); break;
                    case 2:  line.append("H-"); break;
                    case 3:  line.append("C-"); break;
                    case 4:  line.append("C#"); break;
                    case 5:  line.append("D-"); break;
                    case 6:  line.append("D#"); break;
                    case 7:  line.append("E-"); break;
                    case 8:  line.append("F-"); break;
                    case 9:  line.append("F#"); break;
                    case 10: line.append("G-"); break;
                    case 11: line.append("G#"); break;
                }
                if(octave < 10)
                    line.append("0");
                line.append(octave);
                if(instrumentIndex < 10)
                    line.append(" 0");
                else
                    line.append(" ");
                line.append(instrumentIndex);

            }

            if(channelIndex + 1 < song.channelsPerPattern)
                line.append(" | ");
        }
    }

    for(u32 i = 0; i < lines.size(); ++i)
    {
        String s(lines[i].getStr(), 3);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", s.getStr());
        for(u32 j = 0; j < song.channelsPerPattern; ++j)
        {
            s = String(lines[i].getStr() + 3 + (j * 10), 9);
            ImGui::SameLine();
            //printf("%s\n", s.getStr());
            if(i == currentRowIndex && j == currentColumnIndex && editMode)
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", s.getStr());
            else if(i == currentRowIndex && j == currentColumnIndex)
                ImGui::TextColored(ImVec4(1, 0, 1, 1), "%s", s.getStr());

            else if(i == currentRowIndex)
                ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", s.getStr());
            else
                ImGui::TextColored(ImVec4(0, 0, 1, 1), "%s", s.getStr());
        }
    }
    ImGui::End();

}

void SoundTest::renderUpdate()
{
    windowWidth = u32(viewport.size.x);
    windowHeight = u32(viewport.size.y);

    VulkanApp::renderUpdate();

    editorSystem.renderUpdateGui();
    viewport = editorSystem.getEditorWindowViewport();

    if(viewport.size.x != windowWidth || viewport.size.y != windowHeight)
        vulk->needToResize = needToResize = true;

    editorSystem.renderUpdateViewport();


    scene.update(dt);

    drawSoundGui(notes[currentNoteIndex], currentOctave, currentNoteIndex);
    drawSongGui(song, currentPatternIndex, currentRowIndex, currentColumnIndex, editMode);

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

i32 main(i32 argCount, char **argv)
{
    initMemory();
    if(initMyGui())
    {
        SoundTest app;
        if (app.init("Sound test", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMyGui();
    deinitMemory();
    return 0;
}