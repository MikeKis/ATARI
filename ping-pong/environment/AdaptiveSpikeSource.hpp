#ifndef ADAPTIVESPIKESOURCE_HPP
#define ADAPTIVESPIKESOURCE_HPP

class AdaptiveSpikeSource
{
    int LastTactinThisState;
    float rCurrentFrequency;
public:
    AdaptiveSpikeSource(): LastTactinThisState(-2) {}
    bool bFire();
};

#endif // ADAPTIVESPIKESOURCE_HPP
