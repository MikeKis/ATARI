/*
Copyright (C) 2022 Mikhail Kiselev

Emulates ping-pong game.

*/
// .specs/2026-10-13_ping-pong-imitation-learning.md

#define _USE_MATH_DEFINES

#include <math.h>
#include <random>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>

#include "EnvironmentState.hpp"
#include "AdaptiveSpikeSource.hpp"

using namespace std;
using namespace boost::interprocess;

extern EnvironmentState es;
extern RandomNumberGenerator rng;

extern pair<float, float> prr_BallSpeed;

extern bool b_forVerifier_Reward;

int nGoalLevels;

int ntact = -1;
int tactStart;

int nRewards = 0;
int nPunishments = 0;
int nRewardsTot = 0;
int nPunishmentsTot = 0;
extern int nAgentStartTime;
extern int nRewardsAgent;
extern int nPunishmentsAgent;
int InputBlockCounter = 0;

int nActionCommandPeriod = 1;
int nAfterRewardSilence = afterRewardSilence;
bool bDumpState = false;
bool bGridTact = false;
bool bFlyingLeft = false;
RacketCommand cmdTeacher = RacketCommand::Silent;
int nLabelUntil = 0;
int nTeacherDown = 0;
int nTeacherUp = 0;
int nLeftDecisions = 0;

class DYNAMIC_LIBRARY_EXPORTED_CLASS Evaluator: public IReceptors
{
public:
    enum type
    {
        reward,
        punishment,
    };
private:
    enum Evaluator::type typ;
    int                  TrainCounter = 0;
    int                  PeriodCounter;

protected:
    virtual void GetMeanings(VECTOR<STRING> &vstr_Meanings) const override
    {
            vstr_Meanings.resize(1);
            vstr_Meanings.front() = typ == reward ? "REW" : "PUN";
            }
public:
    Evaluator(enum Evaluator::type t, size_t tactbeg = 0): typ(t) {}
    virtual bool bGenerateSignals(unsigned *pfl, int bitoffset) override
    {
        switch (typ) {
            case punishment: if (es.pprr_Ball->first < -0.5F) {
                                TrainCounter = RewardTrainLength;
                                PeriodCounter = 1;
                                if (ntact >= tactStart)
                                    ++nPunishmentsTot;
                                if (ntact >= nAgentStartTime)
                                    ++nPunishmentsAgent;
                                ++nPunishments;
                             }
                             break;
            case reward:     if (es.pprr_Ball->first == -0.5F) {
                                TrainCounter = RewardTrainLength;
                                PeriodCounter = 1;
                                if (ntact >= tactStart)
                                    ++nRewardsTot;
                                if (ntact >= nAgentStartTime)
                                    ++nRewardsAgent;
                                ++nRewards;
                             }
                             break;
            default:         return true;
        }
        *pfl = 0;
        if (TrainCounter && !--PeriodCounter) {
            *pfl = 1;
            PeriodCounter = RewardTrainPeriod;
            --TrainCounter;
            InputBlockCounter = nAfterRewardSilence;
        }
        return true;
    }
    virtual void Randomize(void) override {};
    virtual void SaveStatus(Serializer &ser) const override
    {
        IReceptors::SaveStatus(ser);
        ser << typ;
    }
    virtual ~Evaluator() = default;
    void LoadStatus(Serializer &ser)
    {
        IReceptors::LoadStatus(ser);
        ser >> typ;
    }
};

vector<float> vr_CurrentPhaseSpacePoint(5);

class DYNAMIC_LIBRARY_EXPORTED_CLASS rec_ping_pong: public IReceptors
{
    vector<float> vr_VelocityZoneBoundary;
    vector<AdaptiveSpikeSource> vass_;
protected:
    virtual bool bGenerateSignals(unsigned *pfl, int bitoffset) override
    {
        vector<int> vind_(6);
#define indxBall vind_[0]
#define indyBall vind_[1]
#define indvxBall vind_[2]
#define indvyBall vind_[3]
#define indRacket vind_[4]

        ++ntact;
        UpdateWorld(vr_CurrentPhaseSpacePoint);

        static ofstream ofsState;
        static ofstream ofsStateDiscrete;
        static bool bDumpOpened = false;
        if (bDumpState) {
            if (!bDumpOpened) {
                ofsState.open("ping_pong_state.csv");
                ofsStateDiscrete.open("ping_pong_state_discrete.csv");
                bDumpOpened = true;
                if (ofsState.is_open())
                    ofsState << "ntact,x,y,vx,vy,ry\n";
                if (ofsStateDiscrete.is_open())
                    ofsStateDiscrete << "ntact,x,y,vx,vy,ry\n";
            }
            if (ofsState.is_open()) {
                ofsState << ntact;
                for (auto z: vr_CurrentPhaseSpacePoint)
                    ofsState << ',' << z;
                ofsState << endl;
            }
        }

        indxBall = (int)((vr_CurrentPhaseSpacePoint[0] + 0.5) / (1. / nSpatialZones));
        if (indxBall == nSpatialZones)
            indxBall = nSpatialZones - 1;
        indyBall = (int)((vr_CurrentPhaseSpacePoint[1] + 0.5) / (1. / nSpatialZones));
        if (indyBall == nSpatialZones)
            indyBall = nSpatialZones - 1;
        indvxBall = (int)(lower_bound(vr_VelocityZoneBoundary.begin(), vr_VelocityZoneBoundary.end(), abs(vr_CurrentPhaseSpacePoint[2])) - vr_VelocityZoneBoundary.begin());
        indvxBall = vr_CurrentPhaseSpacePoint[2] < 0 ? nVelocityZones / 2 - indvxBall : nVelocityZones / 2 + indvxBall;
        indvyBall = (int)(lower_bound(vr_VelocityZoneBoundary.begin(), vr_VelocityZoneBoundary.end(), abs(vr_CurrentPhaseSpacePoint[3])) - vr_VelocityZoneBoundary.begin());
        indvyBall = vr_CurrentPhaseSpacePoint[3] < 0 ? nVelocityZones / 2 - indvyBall : nVelocityZones / 2 + indvyBall;
        indRacket = (int)((vr_CurrentPhaseSpacePoint[4] + 0.5) / (1. / nSpatialZones));
        if (indRacket == nSpatialZones)
            indRacket = nSpatialZones - 1;
        vector<unsigned> vfl_(AfferentSpikeBufferSizeDW(GetNReceptors()), 0);
        if (!InputBlockCounter) {
#define set_input_spike(ind) if (vass_[ind].bFire()) &vfl_.front() |= BitMaskAccess(ind)
            set_input_spike(indxBall);
            set_input_spike(nSpatialZones + indyBall);
            set_input_spike(nSpatialZones * 2 + indvxBall);
            set_input_spike(nSpatialZones * 2 + nVelocityZones + indvyBall);
            set_input_spike(nSpatialZones * 2 + nVelocityZones * 2 + indRacket);

            if (bDumpState && ofsStateDiscrete.is_open())
                ofsStateDiscrete << ntact << ',' << indxBall << ',' << indyBall << ',' << indvxBall << ',' << indvyBall << ',' << indRacket << endl;

        } else --InputBlockCounter;
        copy(vfl_.begin(), vfl_.end(), pfl);

        return true;
    }
    virtual void GetMeanings(VECTOR<STRING> &vstr_Meanings) const override
    {
        vstr_Meanings.resize(nInputs);
        int i = 0;
        int x;
        for (x = 0; x < nSpatialZones; ++x) {
            stringstream ss;
            ss << "x" << x;
            vstr_Meanings[i++] = ss.str();
        }
        for (x = 0; x < nSpatialZones; ++x) {
            stringstream ss;
            ss << "y" << x;
            vstr_Meanings[i++] = ss.str();
        }
        for (x = 0; x < nVelocityZones; ++x) {
            stringstream ss;
            ss << "vx" << x - nVelocityZones / 2;
            vstr_Meanings[i++] = ss.str();
        }
        for (x = 0; x < nVelocityZones; ++x) {
            stringstream ss;
            ss << "vy" << x - nVelocityZones / 2;
            vstr_Meanings[i++] = ss.str();
        }
        for (x = 0; x < nSpatialZones; ++x) {
            stringstream ss;
            ss << "ry" << x;
            vstr_Meanings[i++] = ss.str();
        }
    }
public:
    rec_ping_pong(): vr_VelocityZoneBoundary((nVelocityZones - 1) / 2), vass_(nInputs)
    {
        vector<float> vr_samples(9000);
        for (auto &i: vr_samples) {
            float rBallVelocity = rMakeBallVelocity();
            float rBallMovementDirection = (float)rng(M_PI / 2);
            i = rBallVelocity * sin(rBallMovementDirection);
        }
        sort(vr_samples.begin(), vr_samples.end());
        FORI((nVelocityZones - 1) / 2)
            vr_VelocityZoneBoundary[_i] = vr_samples[vr_samples.size() / 9 + _i * 2 * vr_samples.size() / 9];

    }
    virtual void Randomize(void) override {rng.Randomize();}
    virtual void SaveStatus(Serializer &ser) const override
    {
        IReceptors::SaveStatus(ser);
        ser << *es.pprr_Ball;
        ser << *es.prRacket;
        ser << prr_BallSpeed;
        ser << vr_VelocityZoneBoundary;
        ser << rng;
    }
    virtual ~rec_ping_pong() = default;
    void LoadStatus(Serializer &ser)
    {
        IReceptors::LoadStatus(ser);
        ser >> *es.pprr_Ball;
        es.prRacket = &es.pst->racket_y;
        ser >> *es.prRacket;
        ser >> prr_BallSpeed;
        ser >> vr_VelocityZoneBoundary;
        ser >> rng;
        es.bReady = true;
    }
};

#ifndef BIGREALNUMBER
#define BIGREALNUMBER 1.e10F
#endif

static const float LEFT_WALL_X   = -0.5F;
static const float RIGHT_WALL_X  =  0.5F;
static const float BOTTOM_WALL_Y = -0.5F;
static const float TOP_WALL_Y    =  0.5F;

static float rFoldToSegment(float rValue, float rLeft, float rRight)
{
    float rLength = rRight - rLeft;
    float rShifted = (rValue - rLeft) / rLength;
    float rMod = fmod(rShifted, 2.F);
    if (rMod < 0.F)
        rMod += 2.F;
    float rFolded = rMod <= 1.F ? rMod : 2.F - rMod;
    return rLeft + rFolded * rLength;
}

static float rTargetYAtLeftWall(void)
{
    float x0 = vr_CurrentPhaseSpacePoint[0];
    float y0 = vr_CurrentPhaseSpacePoint[1];
    float vx = vr_CurrentPhaseSpacePoint[2];
    float vy = vr_CurrentPhaseSpacePoint[3];

    if (x0 <= LEFT_WALL_X)
        return y0;
    if (fabs(vx) < 1e-12F)
        return y0;

    float t = vx < 0.F ? (x0 - LEFT_WALL_X) / (-vx)
                       : (RIGHT_WALL_X - x0) / vx + (RIGHT_WALL_X - LEFT_WALL_X) / vx;
    return rFoldToSegment(y0 + vy * t, BOTTOM_WALL_Y, TOP_WALL_Y);
}

class DYNAMIC_LIBRARY_EXPORTED_CLASS Actions: public IReceptors
{
protected:
    virtual void GetMeanings(VECTOR<STRING> &vstr_Meanings) const override
    {
        vstr_Meanings.resize(2);
        vstr_Meanings[0] = "ActDown";
        vstr_Meanings[1] = "ActUp";
    }
public:
    Actions() {}
    virtual bool bGenerateSignals(unsigned *pfl, int bitoffset) override
    {
        bGridTact = nActionCommandPeriod > 0 && ntact % nActionCommandPeriod == 0;
        bFlyingLeft = vr_CurrentPhaseSpacePoint[2] < 0.F;
        cmdTeacher = RacketCommand::Silent;
        *pfl = 0;
        if (!bGridTact)
            return true;
        if (!bFlyingLeft)
            return true;
        ++nLeftDecisions;
        float rTargetY = rTargetYAtLeftWall();
        float rRacketY = vr_CurrentPhaseSpacePoint[4];
        if (fabs(rTargetY - rRacketY) > rMovementThreshold)
            cmdTeacher = rTargetY > rRacketY ? RacketCommand::Up : RacketCommand::Down;
        if (nLabelUntil != 0 && ntact >= nLabelUntil)
            return true;
        if (cmdTeacher == RacketCommand::Down) {
            *pfl |= 1u << bitoffset;
            ++nTeacherDown;
        } else if (cmdTeacher == RacketCommand::Up) {
            *pfl |= 2u << bitoffset;
            ++nTeacherUp;
        }
        return true;
    }
    virtual void Randomize(void) override {};
    virtual void SaveStatus(Serializer &ser) const override
    {
        IReceptors::SaveStatus(ser);
        ser << nActionCommandPeriod;
        ser << nLabelUntil;
        ser << rRacketStep;
        ser << rRacketSize;
        ser << rMovementThreshold;
    }
    virtual ~Actions() = default;
    void LoadStatus(Serializer &ser)
    {
        IReceptors::LoadStatus(ser);
        ser >> nActionCommandPeriod;
        ser >> nLabelUntil;
        ser >> rRacketStep;
        ser >> rRacketSize;
        ser >> rMovementThreshold;
    }
};

RECEPTORS_SET_PARAMETERS(pchMyReceptorSectionName, nReceptors, xn)
{
	static int CallNo = 0;
	switch (CallNo++) {
		case 0: nReceptors = 1;
			    return new Evaluator(Evaluator::punishment);
		case 1: nReceptors = 1;
			    return new Evaluator(Evaluator::reward);
		case 2: nReceptors = nInputs;
                {
                    int nSilence = afterRewardSilence;
                    bool bDump = false;
                    GetAllParameters(xn, "after_reward_silence", &nSilence, "dump_state", &bDump);
                    if (nSilence < 0) {
                        cout << "after_reward_silence must be >= 0\n";
                        exit(-1);
                    }
                    nAfterRewardSilence = nSilence;
                    bDumpState = bDump;
                }
			    return new rec_ping_pong;
        case 3: nReceptors = 2;
                {
                    int nPeriod = 1;
                    int nLearn = 0;
                    float rStep = 1.F / nSpatialZones;
                    float rSize = DEFAULT_RACKET_SIZE;
                    float rThreshold = 0.5F / nSpatialZones;
                    GetAllParameters(xn,
                        "action_period", &nPeriod,
                        "learning_time", &nLearn,
                        "racket_step", &rStep,
                        "racket_size", &rSize,
                        "movement_threshold", &rThreshold);
                    if (xn.child("action_period") && nPeriod < 4) {
                        cout << "action_period must be >= 4\n";
                        exit(-1);
                    }
                    if (nLearn < 0) {
                        cout << "learning_time must be >= 0\n";
                        exit(-1);
                    }
                    if (!(rStep > 0.F)) {
                        cout << "racket_step must be > 0\n";
                        exit(-1);
                    }
                    if (!(rSize > 0.F) || rSize >= 1.F) {
                        cout << "racket_size must be in (0, 1)\n";
                        exit(-1);
                    }
                    if (rThreshold < 0.F) {
                        cout << "movement_threshold must be >= 0\n";
                        exit(-1);
                    }
                    nActionCommandPeriod = nPeriod;
                    nLabelUntil = nLearn;
                    rRacketStep = rStep;
                    rRacketSize = rSize;
                    rMovementThreshold = rThreshold;
                }
                return new Actions;
        default: cout << "ping-pong -- Too many calls of SetParametersIn\n";
				exit(-1);
	}
}

DYNAMIC_LIBRARY_ENTRY_POINT IReceptors *LoadStatus(Serializer &ser)
{
	static int CallNo = 0;
	rec_ping_pong *prpp;
	Evaluator *peva;
	switch (CallNo++) {
		case 0: prpp = new rec_ping_pong;
				prpp->LoadStatus(ser);
				return prpp;
		case 1: peva = new Evaluator(Evaluator::punishment);
				peva->LoadStatus(ser);
				return peva;
		case 2: peva = new Evaluator(Evaluator::reward);
				peva->LoadStatus(ser);
				return peva;
//		case 3: papG = new AdaptivePoisson(true);
//				papG->LoadStatus(ser);
//				return papG;
		default: cout << "Too many calls of LoadStatus\n";
				exit(-1);
	}
}
