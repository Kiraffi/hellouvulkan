#pragma once

#include "core/mytypes.h"

using AtomicType = u64;

static constexpr AtomicType NOTE_COUNT = 32;
static constexpr i32 SAMPLE_POINTS = 16;

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
    float amplitudes[SAMPLE_POINTS];
    i32 tuning[SAMPLE_POINTS];
    i32 oscType;
    i32 oscLFOType;
    float oscLFOHz;

    float freqHz;

    float attackAmplitude;
    float sustainAmplitude;

    float attackDuration;
    float decayDuration;
    float releaseDuration;
};

struct NoteThread
{
    double startTime = 0.0;
    float runningTime = 0.0;
    float releaseStart = 1.0e30f;
    NotePlayPhase phase = NotePlayPhase::Finished;
};

bool initAudio();
void deinitAudio();

double evaluateSound(double time, double freq, i32 instrument);

AtomicType getRunningNotes();
AtomicType getReleasedNotes();

//Note fix these
//void checkNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysUp, double time);
void releaseChannel(AtomicType channelToRelease);
AtomicType addNote(float playFreqHz, const NoteFromMainToThread &currentNote);
/*
void addNotes(AtomicType channel, AtomicType running, AtomicType released, AtomicType &keysDown,
    double time, float baseHz, const NoteFromMainToThread &currentNote);
*/