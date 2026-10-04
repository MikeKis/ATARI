#include <iostream>
#include "WorldModelTest.hpp"

using namespace std;

WorldModelTest *pwmtG = NULL;

const int aindReceptorSection[] = {0, 30, 60, 69, 78, 108};
const int nPeriods = 4;

WorldModelTest::WorldModelTest(const pugi::xml_node &xn, int nRec)
{
    SetNReceptors(aindReceptorSection[5] * nPeriods);
    ifstream ifsCompleteState("ping_pong_state_complete.csv");
    while (!ifsCompleteState.eof()) {
        string s;
        getline(ifsCompleteState, s);
        PAIR<int,VECTOR<PAIR<int, int> > > ptactvp_{0, VECTOR<PAIR<int, int> >(5)};
        sscanf(
               s.c_str(), 
               "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &ptactvp_.first, 
               &ptactvp_.second[0].first, 
               &ptactvp_.second[0].second, 
               &ptactvp_.second[1].first, 
               &ptactvp_.second[1].second, 
               &ptactvp_.second[2].first, 
               &ptactvp_.second[2].second, 
               &ptactvp_.second[3].first, 
               &ptactvp_.second[3].second,
               &ptactvp_.second[4].first, 
               &ptactvp_.second[4].second
              );
        vptactvp_CompleteStates.push_back(ptactvp_);
    }

    const char *apchReceptorSection[] = { "x", "y", "vx", "vy", "ry" };
    int i = 0;
    int ReceptorSection = 0;
    vstr_MyMeanings.resize(GetNReceptors());
    while (ReceptorSection < sizeof(apchReceptorSection) / sizeof(apchReceptorSection[0])) {
        int section_size = aindReceptorSection[ReceptorSection + 1] - aindReceptorSection[ReceptorSection];
        for (int offset_in_section = 0; offset_in_section < section_size; ++offset_in_section)
            for (int TimePeriod = 0; TimePeriod < 4; ++TimePeriod) {
               std::stringstream ss;
               ss << apchReceptorSection[ReceptorSection];
               ss << (ReceptorSection != 2 && ReceptorSection != 3 ? offset_in_section : offset_in_section - 4) << '/' << TimePeriod;
               vstr_MyMeanings[i++] = ss.str();
           }
        ++ReceptorSection;
    }
}

RECEPTORS_SET_PARAMETERS(pchMyReceptorSectionName, nReceptors, xn)
{
	pwmtG = new WorldModelTest(xn, nReceptors);
	return pwmtG;
}

DYNAMIC_LIBRARY_ENTRY_POINT IReceptors *LoadStatus(Serializer &ser)
{
	return nullptr;
}

bool WorldModelTest::bGenerateSignals(unsigned *pfl, int bitoffset)
{
    switch (ntact++) {
        case 0: {
                    vector<unsigned> vfl_InputSignal(AfferentSpikeBufferSizeDW(GetNReceptors()), 0);
                    auto pflMy = &vfl_InputSignal.front();
                    int indstart = 1148539;
#ifndef EXPERIMENTAL
                    cin >> indstart;
#else
                    cout << "ATTETION!! Experimental regime - indstart is " << indstart << '\n';
#endif
                    auto vp_CurrentCompleteState = vptactvp_CompleteStates[indstart].second;
                    FORI(vp_CurrentCompleteState.size())
                        pflMy |= BitMaskAccess((aindReceptorSection[_i] + vp_CurrentCompleteState[_i].first) * nPeriods + vp_CurrentCompleteState[_i].second);
                    copy(vfl_InputSignal.begin(), vfl_InputSignal.end(), pfl);
                    CurrentY = vp_CurrentCompleteState[1].first;
                }
                break;
        case 1: fill(pfl, pfl + AfferentSpikeBufferSizeDW(GetNReceptors()), 0);   // mo more writes there!
                break;
        default: break;
    }
    return ntact < 3000 && !bLeftWallReached;
}

void WorldModelTest::change_to(int indrec)
{
    if (!indrec)   // = x==0
        bLeftWallReached = true;
    int ReceptorSection = (int)(upper_bound(aindReceptorSection, aindReceptorSection + sizeof(aindReceptorSection) / sizeof(aindReceptorSection[0]), indrec / nPeriods) - aindReceptorSection) - 1;
    if (ReceptorSection == 1) {   // Y coordinate
        int offset = indrec - aindReceptorSection[1] * nPeriods;
        CurrentY = offset / nPeriods;
    }
    ++nhops;
}
