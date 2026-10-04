/*
Copyright (C) 2022 Mikhail Kiselev

Emulates ping-pong game.

*/

#include <iostream>
#include "../ping-pong-environment.h"
#include "EnvironmentState.hpp"

using namespace std;

extern int tactStart;
extern float rStateFiringFrequency;
extern int ntact;
extern EnvironmentState es;

extern int nRewards;
extern int nPunishments;
extern int nRewardsTot;
extern int nPunishmentsTot;

extern int LastRegistration;
extern int PostRewardCounter;
extern int nRecognitions;
extern int nCorr;

int nNeuronsperAction = 0;

READOUT_SET_PARAMETERS(ExperimentId, tactTermination, nOutputNeurons, xn)
{
	nNeuronsperAction = (int)nOutputNeurons / 2;
	tactStart = atoi_s(xn.child("start_time").child_value());
}

const float rAction = 1.F / nSpatialZones;

READOUT_OBTAIN_SPIKES(v_Firing)
{
    using namespace std::placeholders;

    static int NoMoveTacts = 0;
    int nCommandsDown = (int)count_if(v_Firing.begin(), v_Firing.end(), bind(less<int>{}, _1, nNeuronsperAction));
	auto r = rAction * ((int)v_Firing.size() - 2 * nCommandsDown);
	auto rsav = *es.prRacket;
	*es.prRacket += r;
	if (*es.prRacket > 0.5F - RACKET_SIZE / 2)
		*es.prRacket = 0.5F - RACKET_SIZE / 2;
	else if (*es.prRacket < -0.5F + RACKET_SIZE / 2)
		*es.prRacket = -0.5F + RACKET_SIZE / 2;

	if (*es.prRacket == rsav) {
		if (++NoMoveTacts == 100000)
			return false;
	} else NoMoveTacts = 0;

	if (!(ntact % 200000)) {
		cout << "rew " << nRewards << " pun " << nPunishments << endl;
		nRewards = nPunishments = 0;
	}

	return true;
}

READOUT_FINALIZE(OriginalTerminationCode, filGenLog)
{
	cout << "rew " << nRewards << " pun " << nPunishments << endl;

    //double dmean, dstderr;
    //avgdis(&v_err.front(), v_err.size(), dmean, dstderr);
    //cout << "err: mean " << dmean << " stderr " << dstderr << endl;

	return nRewardsTot * 10000 / (nPunishmentsTot + nRewardsTot);
}

DYNAMIC_LIBRARY_ENTRY_POINT void Serialize(Serializer &ser, bool bSave) {}
