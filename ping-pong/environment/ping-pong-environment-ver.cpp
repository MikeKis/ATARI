/*
Copyright (C) 2022 Mikhail Kiselev

Emulates ping-pong game.

*/

#include "../ping-pong-environment.h"

using namespace std;

extern int ntact;

vector<pair<string, unsigned> > vpstrn_sec;
int nNeurons;
pair<int, int> p_LREWRange;
int indREWNORM;

DYNAMIC_LIBRARY_ENTRY_POINT void GetSections(const vector<pair<string, unsigned> > &vpstrn_Sections)
{
	vpstrn_sec = vpstrn_Sections;
	nNeurons = 0;
	for (const auto &i: vpstrn_Sections) {
		if (i.first == "LREW") {
			p_LREWRange.first = nNeurons;
			p_LREWRange.second = p_LREWRange.first + i.second;
		} else if (i.first == "REWNORM")
			indREWNORM = nNeurons;
		nNeurons += i.second;
	}
}

int PostRewardCounter = 0;
int nRecognitions = 0;
int LastRegistration = -1000000;
bool b_forVerifier_Reward = false;
int nCorr = 0;

DYNAMIC_LIBRARY_ENTRY_POINT void ObtainSpikes(const vector<int> &v_Firing, string &strFatal)
{
	if (!PostRewardCounter && find(v_Firing.begin(), v_Firing.end(), indREWNORM) != v_Firing.end())
		++nRecognitions;
	if (!PostRewardCounter && any_of(v_Firing.begin(), v_Firing.end(), [=](int indneu) {return p_LREWRange.first <= indneu && indneu < p_LREWRange.second; }))
		LastRegistration = ntact;
	if (b_forVerifier_Reward) {
		if (ntact - LastRegistration < 100)
			++nCorr;
		PostRewardCounter = RewardTrainLength * RewardTrainPeriod;
		b_forVerifier_Reward = false;
	}
	if (PostRewardCounter)
		--PostRewardCounter;
}
