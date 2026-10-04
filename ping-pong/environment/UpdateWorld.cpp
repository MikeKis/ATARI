#include "EnvironmentState.hpp"

using namespace std;

extern EnvironmentState es;
extern RandomNumberGenerator rng;

pair<float, float> prr_BallSpeed;

void UpdateWorld(vector<float> &vr_PhaseSpacePoint)
{
    if (!es.prRacket) {
        es.prRacket = &((pair<pair<float, float>, float> *)es.pprr_Ball)->second;
        es.ResetRacket();
        es.ResetBall();
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
        if (es.pprr_Ball->first < -0.5F && es.pprr_Ball->second > *es.prRacket - RACKET_SIZE / 2 && es.pprr_Ball->second < *es.prRacket + RACKET_SIZE / 2) {
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
