#include "../ping-pong-environment.h"
#include "AdaptiveSpikeSource.hpp"

extern int ntact;
extern RandomNumberGenerator rng;

float rStateFiringFrequency = 0.9F;

bool AdaptiveSpikeSource::bFire()
{
    if (LastTactinThisState < ntact - 1)
        rCurrentFrequency = rStateFiringFrequency;
    bool bret = rng() < rCurrentFrequency;
    LastTactinThisState = ntact;
    return bret;
}
