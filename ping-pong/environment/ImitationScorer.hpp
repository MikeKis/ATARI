// .specs/2026-10-13_ping-pong-imitation-learning.md
#ifndef IMITATIONSCORER_HPP
#define IMITATIONSCORER_HPP

#include <vector>

#include "../ping-pong-environment.h"

// Binary triggers for the two racket actions. A copy votes if its OUT neuron
// spiked at least once in (t-P, t]. The trigger is a strict majority of copies.
class ImitationScorer
{
    int  nCopies = 0;
    int  nWindow = 1;
    int  nFilled = 0;
    int  iOldest = 0;
    int  vnWindow[2][4] = {};
    int  vnTest[2][4] = {};
    int  nTestDecisions = 0;
    int  nBothSpiked = 0;
    std::vector<unsigned char> vuc_Fired;
    std::vector<unsigned char> vuc_Ring;
    void PushWindow(unsigned char ucDown, unsigned char ucUp);
public:
    void Configure(int nOutputNeurons, int nWindowDecisions);
    void Observe(const int *pIds, int nIds);
    void ClearWindow(void);
    RacketCommand cmdTrigger(bool *pbBothSpiked);
    void Score(RacketCommand cmdTeacher, bool bTestPhase);
    double dWindowF1(int iAction) const;
    double dTestF1(int iAction) const;
    double dTestMeanF1(void) const;
    int NTestDecisions(void) const {return nTestDecisions;}
    int NBothSpiked(void) const {return nBothSpiked;}
    int NTest(int iAction, int nKind) const {return vnTest[iAction][nKind];}
};

#endif
