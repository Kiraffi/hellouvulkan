#include "myaudio.h"

#include "core/general.h"
#include "core/glfw_keys.h"

#include <extras/miniaudio_split/miniaudio.h>

#include <atomic>
#include <math.h>

static constexpr ma_format DEVICE_FORMAT = ma_format_f32;
static constexpr int DEVICE_CHANNELS = 2;
static constexpr int DEVICE_SAMPLE_RATE = 48000;


static double getFreq(double index)
{
    return Supa::powd(2, index / 12.0);
}

static const double GlobalFreqs [NOTE_COUNT] = {
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


struct GlobalAudioDevice
{
    ma_device_config deviceConfig;
    ma_device soundDevice;
    bool isInited = false;
};

static GlobalAudioDevice globalAudioDevice;

static NoteFromMainToThread notesFromMain [NOTE_COUNT] = { };
static NoteThread notesThread [NOTE_COUNT] = { };

static std::atomic<AtomicType> notesRunning(0);
static std::atomic<AtomicType> notesReleased(0);


void checkNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysUp, double time)
{
    AtomicType channelAtom = AtomicType(1) << channel;
    bool runn = (running & channelAtom) != 0;
    bool rel = (released & channelAtom) != 0;
    if(runn && !rel)
    {
        NoteFromMainToThread &note = notesFromMain[channel];

        AtomicType atomKey = AtomicType(1) << note.note;
        bool keyReleased = (keysUp & atomKey) != 0;

        // remove from handled keys
        keysUp &= ~(atomKey);

        if(keyReleased)
        {
            std::atomic_fetch_or(&notesReleased, channelAtom);
        }
    }
}

void addNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysDown,
    double time, float baseHz, const NoteFromMainToThread &currentNote)
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

    note = currentNote;
    note.note = bitScanForward;
    note.baseHz = baseHz;
    std::atomic_fetch_or(&notesRunning, channelAtom);
    return;
}

double evaluateSound(double time, double freq, int instrument)
{
    double fqSampPoint = freq * time;
    double samp = fqSampPoint * 2.0 * PI;
    double t = Supa::modd(fqSampPoint, 1.0);

    auto f = [samp](float m) { return Supa::sind(m) ;};
    //auto f = [samp, t](float m) { return t < 0.5 ? 0.5 : -0.5; };

    switch(instrument)
    {
        case -3:
        {
            // 5 / 7 was nice with 2.0
            double pix = f(samp * 1.0) * 0.5 + f(samp * 7.0 / 13.0) * 0.3 + f(samp * 2.0) * 0.2;
            pix *= Supa::expd(-0.0004 * samp);
            pix += pix * pix * pix * pix * pix;
            pix *= 1.0 + 16.0 * time * Supa::expd(-6.0 * time);
            return pix;
        }
        case -2:
        {
            double pix = f(samp * 1.0) * 0.5 + f(samp * 1.25) * 0.3 + f(samp * 1.5) * 0.2;
            pix *= Supa::expd(-0.0004 * samp);
            pix += pix * pix * pix * pix * pix;
            pix *= 1.0 + 16.0 * time * Supa::expd(-6.0 * time);
            return pix;
        }

        case -1:
        {
            //pix *= pix * pix;
            double pix = f(samp) * 0.6;
            pix += f(samp * 2.0) * 0.4;

            pix *= Supa::expd(-0.0005 * samp);
            double pix3 = pix * pix * pix;
            pix += pix3 + pix3 * pix * pix;
            pix *= 0.25 + 1.0 * time * Supa::expd(-2.0 * time);
            return pix;
            //return pix + sin(Pi * 2.0 / 3.0 + samp);

        }
        case 0:
        {
            double pix = f(samp * 1.0);
            return pix;
        }
        case 1:
            return t < 0.5 ? 0.5 : -0.5;
        case 2:
            return t - 0.5;
        case 3:
            t += 0.25;
            return t < 0.5 ? -0.5 + 2.0 * t : 0.5 - 2.0 * (t - 0.5);
        default:

        break;

    }
    return double(rand()) / double(RAND_MAX) * 2.0 - 1.0;

}

//#include <chrono>
//std::chrono::high_resolution_clock::time_point tp;
static void soundCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    //std::chrono::high_resolution_clock::time_point chTime = std::chrono::high_resolution_clock::now();
    //printf("time: %f, frames: %u\n", (chTime - tp).count() / 100000.0f, frameCount);
    //tp = chTime;
    static uint64_t threadFrameCounter = 0;
    //double offset = *( double * )pDevice->pUserData;

    AtomicType threadNotesRunning = notesRunning.load();
    AtomicType threadNotesReleased = notesReleased.load();

    AtomicType newThreadsFinished = 0u;

    double dur = 1.0 / DEVICE_SAMPLE_RATE;
    double time = dur * threadFrameCounter;

    for(AtomicType j = 0; j < NOTE_COUNT; ++j)
    {
        NoteThread &noteThread = notesThread [j];
        NoteFromMainToThread &noteMain = notesFromMain [j];
        bool running = ( ( threadNotesRunning >> j ) & 1 ) == 1;

        if(noteThread.phase == NotePlayPhase::Finished && running)
        {
            noteThread.startTime = time;
            noteThread.runningTime = 0.0f;
            noteThread.phase = NotePlayPhase::Attack;
            noteThread.releaseStart = 1.0e30f;
        }
    }

    float *f32Out = (float *)pOutput;
    for(uint32_t i = 0; i < frameCount; ++i)
    {
        double frameValue = 0.0;
        time = dur * threadFrameCounter;
        for(AtomicType j = 0; j < NOTE_COUNT; ++j)
        {
            if(( ( threadNotesRunning >> j ) & 1 ) == 0)
                continue;
            NoteFromMainToThread &noteMain = notesFromMain[j];
            NoteThread &noteThread = notesThread [j];
            bool released = ( ( threadNotesReleased >> j ) & 1 ) == 1;

            double tmpValue = 0.0;
            double amplitude = 0.0;

            switch(noteThread.phase)
            {
                // NOTE: The fall through is on purpose, so if there is no decrease time or something.

                case NotePlayPhase::Attack:
                {
                    float attackTimePos = noteThread.runningTime;
                    if(attackTimePos < noteMain.attackDuration)
                    {
                        float attackPos = attackTimePos / noteMain.attackDuration;
                        amplitude = attackPos * noteMain.attackAmplitude;

                        break;
                    }
                    noteThread.phase = NotePlayPhase::Decay;
                }
                case NotePlayPhase::Decay:
                {
                    float decayTimePos = noteThread.runningTime - noteMain.attackDuration;
                    if(decayTimePos < noteMain.decayDuration)
                    {
                        float decayPos = decayTimePos / noteMain.decayDuration;
                        amplitude = decayPos * noteMain.sustainAmplitude + (1.0f - decayPos) * noteMain.attackAmplitude;
                        break;
                    }
                    noteThread.phase = NotePlayPhase::Sustain;
                }
                case NotePlayPhase::Sustain:
                {
                    if(!released && noteMain.sustainAmplitude > 0.0f)
                    {
                        amplitude = noteMain.sustainAmplitude;
                        break;
                    }
                    noteThread.phase = NotePlayPhase::Release;
                    noteThread.releaseStart = Supa::minf(noteThread.runningTime, noteThread.releaseStart);

                }
                case NotePlayPhase::Release:
                {
                    float releaseTimePos = noteThread.runningTime - noteThread.releaseStart;
                    if(releaseTimePos < noteMain.releaseDuration && noteMain.sustainAmplitude > 0.0f)
                    {
                        float releasePos = releaseTimePos / noteMain.releaseDuration;
                        amplitude = exp(-8.0 * releasePos / noteMain.releaseDuration) * noteMain.sustainAmplitude;
                        //amplitude = (1.0f - releasePos) * noteMain.sustainAmplitude;
                        break;
                    }
                    noteThread.phase = NotePlayPhase::Finished;
                    newThreadsFinished |= AtomicType(1) << j;
                    threadNotesRunning &= ~( AtomicType(1) << j );
                }
                default:
                    break;
            }

            if(noteThread.phase != NotePlayPhase::Finished)
            {
                double timePoint = time - noteThread.startTime;
                float duration = Supa::maxd(0.005, noteMain.attackDuration + noteMain.decayDuration);
                double freq = GlobalFreqs[noteMain.note] * noteMain.baseHz;
                double value =  timePoint * noteMain.oscLFOHz; //timePoint / duration;
                value *= double(SAMPLE_POINTS);
                int iValue = int(value) % SAMPLE_POINTS;
                int iValue2 = (iValue + 1) % SAMPLE_POINTS;
                float lerping = value - iValue;
                value = noteMain.tuning[iValue] * (1.0f - lerping) + noteMain.tuning[iValue2] * lerping;

                value = pow(2.0, value / 12.0);
                //printf("Value: %f\n", value);
                //amplitude *= noteMain.amplitudes[iValue] * (1.0f - lerping) + noteMain.amplitudes[iValue2] * lerping;
                //tmpValue = value / 24.0;
                tmpValue = evaluateSound(timePoint, freq * value, noteMain.oscType);
                //tmpValue *= evaluateSound(timePoint, noteMain.oscLFOHz, noteMain.oscLFOType);
                //amplitude *= value;
                //tmpValue *= value;
                //tmpValue = evaluateSound(timePoint, value, 2)  * tmpValue;

/*                tmpValue = evaluateSound(timePoint, freq * tmpValue * 2.0, noteMain.oscType);
*/
                //horn

               /*
               nice sound
                tmpValue = evaluateSound(timePoint, freq * 0.5, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.oscType)  * tmpValue;
                */

/*                tmpValue = evaluateSound(timePoint, freq * 0.01, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.oscType)  * tmpValue;
*/
/*
                tmpValue = evaluateSound(timePoint, 30, 0);
                tmpValue = evaluateSound(timePoint, freq, noteMain.oscType)  * tmpValue;
*/
                //tmpValue = evaluateSound(timePoint, freq * tmpValue, 1);
                //tmpValue *= evaluateSound(timePoint + 0.0, 220.0, 2);
            }
            frameValue += tmpValue * amplitude;

            switch(noteThread.phase)
            {
                case NotePlayPhase::Attack:
                case NotePlayPhase::Decay:
                case NotePlayPhase::Release:
                    noteThread.runningTime += dur;
                    break;
                default:
                    break;
            }
        }
        frameValue = Supa::clampd(-1.0, 1.0, frameValue);
        for(uint32_t j = 0; j < DEVICE_CHANNELS; ++j)
            f32Out[i * DEVICE_CHANNELS + j] = frameValue;

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

bool initAudio()
{
    if(globalAudioDevice.isInited)
        return false;
    globalAudioDevice.deviceConfig = ma_device_config_init(ma_device_type_playback);
    globalAudioDevice.deviceConfig.playback.format    = DEVICE_FORMAT;
    globalAudioDevice.deviceConfig.playback.channels  = DEVICE_CHANNELS;
    globalAudioDevice.deviceConfig.sampleRate         = DEVICE_SAMPLE_RATE;
    globalAudioDevice.deviceConfig.dataCallback       = soundCallback;
    globalAudioDevice.deviceConfig.pUserData          = nullptr; //&startTime;
    globalAudioDevice.deviceConfig.performanceProfile = ma_performance_profile_low_latency;

    if (ma_device_init(NULL, &globalAudioDevice.deviceConfig, &globalAudioDevice.soundDevice) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return false;
    }
    globalAudioDevice.isInited = true;

    printf("Device Name: %s\n", globalAudioDevice.soundDevice.playback.name);

    if (ma_device_start(&globalAudioDevice.soundDevice) != MA_SUCCESS)
    {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&globalAudioDevice.soundDevice);
        return false;
    }

    for(AtomicType i = 0; i < NOTE_COUNT; ++i)
    {
        notesThread[i].phase = NotePlayPhase::Finished;
        notesThread[i].releaseStart = 1.0e30f;
        notesThread[i].startTime = 0.0;
    }

   return true;
}

void deinitAudio()
{
    if(globalAudioDevice.isInited)
        ma_device_uninit(&globalAudioDevice.soundDevice);
    globalAudioDevice.isInited = false;
}

AtomicType getRunningNotes()
{
    return notesRunning.load();
}
AtomicType getReleasedNotes()
{
    return notesReleased.load();
}