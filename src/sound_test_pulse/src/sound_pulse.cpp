#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>

#include <math.h>

// start latency in micro seconds
static i32 latency = 10000;

static pa_buffer_attr bufattr;
static i32 underflows = 0;
static pa_stream *pulseAudioStream = nullptr;
static pa_mainloop *pulseAudioMainloop = nullptr;
static pa_mainloop_api *pulseAudioMainloopApi = nullptr;
static pa_context *pulseAudioContext = nullptr;

static constexpr double SampleFreq = 48000.0;
static constexpr double w = 2.0 * M_PI / SampleFreq;
static constexpr u32 SoundChannels = 2;

#if 0
    static constexpr pa_sample_format_t SampleFormat = PA_SAMPLE_FLOAT32NE;
#else
    static constexpr pa_sample_format_t SampleFormat = PA_SAMPLE_S16NE;
#endif
static constexpr u32 BytesPerSample = SoundChannels * (SampleFormat == PA_SAMPLE_FLOAT32NE ? 4 : 2);


static constexpr pa_sample_spec ss =
{
    .format = SampleFormat,
    .rate = u32(SampleFreq),
    .channels = SoundChannels
};



// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void stateCallback(pa_context *c, void *userdata)
{
    pa_context_state_t state;
    i32 *pa_ready = (i32 *)userdata;
    state = pa_context_get_state(c);
    switch  (state)
    {
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            *pa_ready = 2;
        break;
        case PA_CONTEXT_READY:
            *pa_ready = 1;
        break;

        // These are just here for reference
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
        break;
    }
}

// length = channels * buffer type (s16 = 2, f32 = 4)
static void updateBufferCallback(pa_stream *s, size_t length, void *userdata)
{
    pa_usec_t usec;
    i32 neg;
    pa_stream_get_latency(s, &usec, &neg);

    void *data;
    pa_stream_begin_write(s, (void **)&data, &length);
    if(!data)
        return;

    static size_t lastPos = 0;
    size_t startPos = lastPos;
    lastPos += length / (BytesPerSample);

    //printf("latency %8d us, len: %5d, ptr: %p, neg: %2d\n",(int)usec, int(length), data, neg);

    for(size_t i = 0; i < length / BytesPerSample; ++i)
    {
        double wave = (cos(w * (i + startPos) * 440.0));
        double pan = (sin(w * 0.125 * (i + startPos))) * 0.5 + 0.5;

        if(SampleFormat == PA_SAMPLE_FLOAT32NE)
        {
            double v = wave * 0.5;
            ((float *)data)[i * SoundChannels + 0] = v * pan; // right
            if(SoundChannels == 2)
                ((float *)data)[i * SoundChannels + 1] = v * (1.0 - pan); // left
        }
        else
        {
            short v = wave * 32767.0;
            ((short *)data)[i * SoundChannels + 0] = v * pan; // right
            if(SoundChannels == 2)
                ((short *)data)[i * SoundChannels + 1] = v * (1.0 - pan); // left
        }
    }
    pa_stream_write(s, data, length, NULL, 0LL, PA_SEEK_RELATIVE);
}

static void underflowCallback(pa_stream *s, void *userdata)
{
    underflows++;
    if (underflows >= 2 && latency < 200000)
    {
        latency = (latency*3)/2;
        bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
        bufattr.tlength = pa_usec_to_bytes(latency,&ss);
        pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
        underflows = 0;
        printf("latency increased to %d\n", latency);
    }
}

i32 soundMain()
{
    // Create a mainloop API and connection to the default server
    pulseAudioMainloop = pa_mainloop_new();
    pulseAudioMainloopApi = pa_mainloop_get_api(pulseAudioMainloop);
    pulseAudioContext = pa_context_new(pulseAudioMainloopApi, "Simple PA test application");
    pa_context_connect(pulseAudioContext, NULL, pa_context_flags_t(0), NULL);

    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    i32 pulseAudioReady = 0;
    pa_context_set_state_callback(pulseAudioContext, stateCallback, &pulseAudioReady);

    // We can't do anything until PA is ready, so just iterate the mainloop
    // and continue
    while (pulseAudioReady == 0)
        pa_mainloop_iterate(pulseAudioMainloop, 1, NULL);

    if (pulseAudioReady == 2)
        return -1;

    pulseAudioStream = pa_stream_new(pulseAudioContext, "Playback", &ss, NULL);
    if (!pulseAudioStream)
    {
        printf("pa_stream_new failed\n");
    }
    pa_stream_set_write_callback(pulseAudioStream, updateBufferCallback, NULL);
    pa_stream_set_underflow_callback(pulseAudioStream, underflowCallback, NULL);
    bufattr.fragsize = (u32)-1;
    bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
    bufattr.minreq = pa_usec_to_bytes(0,&ss);
    bufattr.prebuf = (u32)-1;
    bufattr.tlength = pa_usec_to_bytes(latency,&ss);

    i32 result = pa_stream_connect_playback(pulseAudioStream, NULL, &bufattr,
        pa_stream_flags_t(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE),
        NULL, NULL);
    if (result < 0)
    {
        // Old pulse audio servers don't like the ADJUST_LATENCY flag, so retry without that
        result = pa_stream_connect_playback(pulseAudioStream, NULL, &bufattr,
            pa_stream_flags_t(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE), NULL, NULL);
    }
    if (result < 0)
    {
        printf("pa_stream_connect_playback failed\n");
        return -1;
    }

    // Run the mainloop until pa_mainloop_quit() is called
    // (this example never calls it, so the mainloop runs forever).
    pa_mainloop_run(pulseAudioMainloop, NULL);
    return 0;
}

i32 main(i32 argc, char *argv[])
{
    i32 value = soundMain();
    // clean up and disconnect
    pa_context_disconnect(pulseAudioContext);
    pa_context_unref(pulseAudioContext);
    pa_mainloop_free(pulseAudioMainloop);
    return value;
}