#pragma once

#include <random>

#ifdef PINGPONGENVIRONMENT_EXPORTS
#include <sg/sg.h>
#include <NetworkConfigurator.h>
#endif

// .specs/2026-10-13_ping-pong-imitation-learning.md

#define ENVIRONMENT_STATE_SHARED_MEMORY_NAME "ping-pong-environment.sm"

const float DEFAULT_RACKET_SIZE = 0.18F;

struct SharedEnvironmentState
{
    float x;
    float y;
    float racket_y;
    float racket_size;
};

enum class RacketCommand
{
    Silent,
    Down,
    Up
};

extern float rRacketSize;
extern float rRacketStep;
extern float rMovementThreshold;

void ApplyRacketCommand(RacketCommand cmd);
void JitterRacket(float rAmplitude);

#ifdef FOR_LINUX
#define DYNAMIC_LIBRARY_EXPORTED_CLASS
typedef __uint64_t UNS64;
#else
#define DYNAMIC_LIBRARY_EXPORTED_CLASS __declspec(dllexport)
typedef unsigned __int64 UNS64;
#endif

const unsigned nSpatialZones = 30;
const int nVelocityZones = 9;
const unsigned nInputs = 3 * nSpatialZones + 2 * nVelocityZones;

void UpdateWorld(std::vector<float> &vr_PhaseSpacePoint);
float rMakeBallVelocity(void);

class RandomNumberGenerator
{
    std::uniform_real_distribution<> urd;
    std::mt19937_64                  mt;
public:
    RandomNumberGenerator() : urd(0., 1.) {}
    double operator()() { return urd(mt); }
    template<class T> T operator()(T max) { return (T)((*this)() * max); }
    void Randomize(void)
    {
        std::mt19937_64 mtRandomized(std::random_device{}());
        mt = mtRandomized;
    }
};

const int RewardTrainLength = 1 /*10*/;
const int RewardTrainPeriod = 2;
const int afterRewardSilence = 30;
