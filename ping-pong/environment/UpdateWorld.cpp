// .specs/2026-10-13_ping-pong-imitation-learning.md
#include "EnvironmentState.hpp"

using namespace std;

extern EnvironmentState es;
extern RandomNumberGenerator rng;

pair<float, float> prr_BallSpeed;

float rRacketSize = DEFAULT_RACKET_SIZE;
float rRacketStep = 1.F / nSpatialZones;
float rMovementThreshold = 0.5F / nSpatialZones;

static void ClampRacket(void)
{
    float rLimit = 0.5F - rRacketSize / 2;
    if (*es.prRacket > rLimit)
        *es.prRacket = rLimit;
    else if (*es.prRacket < -rLimit)
        *es.prRacket = -rLimit;
}

void ApplyRacketCommand(RacketCommand cmd)
{
    if (cmd == RacketCommand::Down)
        *es.prRacket -= rRacketStep;
    else if (cmd == RacketCommand::Up)
        *es.prRacket += rRacketStep;
    if (cmd != RacketCommand::Silent)
        ClampRacket();
}

void JitterRacket(float rAmplitude)
{
    if (rAmplitude <= 0.F)
        return;
    *es.prRacket += (float)((rng() * 2. - 1.) * rAmplitude);
    ClampRacket();
}

void UpdateWorld(vector<float> &vr_PhaseSpacePoint)
{
    if (!es.bReady) {
        es.pst->racket_size = rRacketSize;
        es.ResetRacket();
        es.ResetBall();
        es.bReady = true;
    }

    vr_PhaseSpacePoint[0] = es.pprr_Ball->first;
    vr_PhaseSpacePoint[1] = es.pprr_Ball->second;
    vr_PhaseSpacePoint[2] = prr_BallSpeed.first;
    vr_PhaseSpacePoint[3] = prr_BallSpeed.second;
    vr_PhaseSpacePoint[4] = *es.prRacket;

    if (es.pprr_Ball->first < -0.5F)   // PUNISHMENT
        es.ResetBall(true);
    else {
        bool bnegx = es.pprr_Ball->first < 0.;
        es.pprr_Ball->first += prr_BallSpeed.first;
        if (es.pprr_Ball->first < -0.5F && es.pprr_Ball->second > *es.prRacket - rRacketSize / 2 && es.pprr_Ball->second < *es.prRacket + rRacketSize / 2) {
            es.pprr_Ball->first = -0.5F;   // REWARD
            prr_BallSpeed.first = -prr_BallSpeed.first;
        }
        if (es.pprr_Ball->first > 0.5F) {
            es.pprr_Ball->first = 0.5F;
            prr_BallSpeed.first = -prr_BallSpeed.first;
        }
        es.pprr_Ball->second += prr_BallSpeed.second;
        if (es.pprr_Ball->second < -0.5F) {
            es.pprr_Ball->second = -0.5F;
            prr_BallSpeed.second = -prr_BallSpeed.second;
        }
        if (es.pprr_Ball->second > 0.5F) {
            es.pprr_Ball->second = 0.5F;
            prr_BallSpeed.second = -prr_BallSpeed.second;
        }
        if (bnegx && es.pprr_Ball->first > 0.)
            es.ResetBall(false);
    }
}
