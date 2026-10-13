// .specs/2026-10-13_ping-pong-imitation-learning.md
#include <algorithm>

#include "ImitationScorer.hpp"

using namespace std;

enum { kindTN, kindFP, kindFN, kindTP };

static int nKind(bool bPositive, bool bTrigger)
{
    if (bPositive)
        return bTrigger ? kindTP : kindFN;
    return bTrigger ? kindFP : kindTN;
}

static double dF1(int nTP, int nFP, int nFN)
{
    if (nTP <= 0)
        return 0.;
    double dPrecision = (double)nTP / (nTP + nFP);
    double dRecall = (double)nTP / (nTP + nFN);
    return 2. * dPrecision * dRecall / (dPrecision + dRecall);
}

void ImitationScorer::Configure(int nOutputNeurons, int nWindowDecisions)
{
    nCopies = nOutputNeurons / 2;
    nWindow = nWindowDecisions < 1 ? 1 : nWindowDecisions;
    nFilled = 0;
    iOldest = 0;
    nTestDecisions = 0;
    nBothSpiked = 0;
    for (int a = 0; a < 2; ++a)
        for (int k = 0; k < 4; ++k)
            vnWindow[a][k] = vnTest[a][k] = 0;
    vuc_Fired.assign(nCopies * 2, 0);
    vuc_Ring.assign(nWindow * 2, 0);
}

void ImitationScorer::Observe(const int *pIds, int nIds)
{
    for (int i = 0; i < nIds; ++i) {
        int id = pIds[i];
        if (id >= 0 && id < (int)vuc_Fired.size())
            vuc_Fired[id] = 1;
    }
}

void ImitationScorer::ClearWindow(void)
{
    fill(vuc_Fired.begin(), vuc_Fired.end(), 0);
}

RacketCommand ImitationScorer::cmdTrigger(bool *pbBothSpiked)
{
    int nVotes[2] = {};
    for (int c = 0; c < nCopies; ++c) {
        if (vuc_Fired[c * 2])
            ++nVotes[0];
        if (vuc_Fired[c * 2 + 1])
            ++nVotes[1];
    }
    if (pbBothSpiked)
        *pbBothSpiked = nVotes[0] > 0 && nVotes[1] > 0;
    bool bDown = nVotes[0] * 2 > nCopies;
    bool bUp = nVotes[1] * 2 > nCopies;
    if (bDown && bUp)
        return nVotes[0] == nVotes[1] ? RacketCommand::Silent : nVotes[0] > nVotes[1] ? RacketCommand::Down : RacketCommand::Up;
    if (bDown)
        return RacketCommand::Down;
    if (bUp)
        return RacketCommand::Up;
    return RacketCommand::Silent;
}

void ImitationScorer::PushWindow(unsigned char ucDown, unsigned char ucUp)
{
    unsigned char uc_[2] = {ucDown, ucUp};
    int iWrite;
    if (nFilled < nWindow)
        iWrite = nFilled++;
    else {
        for (int a = 0; a < 2; ++a)
            --vnWindow[a][vuc_Ring[iOldest * 2 + a]];
        iWrite = iOldest;
        iOldest = (iOldest + 1) % nWindow;
    }
    for (int a = 0; a < 2; ++a) {
        vuc_Ring[iWrite * 2 + a] = uc_[a];
        ++vnWindow[a][uc_[a]];
    }
}

void ImitationScorer::Score(RacketCommand cmdTeacher, bool bTestPhase)
{
    bool bBoth = false;
    RacketCommand cmd = cmdTrigger(&bBoth);
    (void)cmd;
    if (bBoth)
        ++nBothSpiked;
    bool bPositive[2] = {cmdTeacher == RacketCommand::Down, cmdTeacher == RacketCommand::Up};
    int nVotes[2] = {};
    for (int c = 0; c < nCopies; ++c) {
        if (vuc_Fired[c * 2])
            ++nVotes[0];
        if (vuc_Fired[c * 2 + 1])
            ++nVotes[1];
    }
    unsigned char uc_[2];
    for (int a = 0; a < 2; ++a) {
        bool bTrig = nVotes[a] * 2 > nCopies;
        uc_[a] = (unsigned char)nKind(bPositive[a], bTrig);
        if (bTestPhase)
            ++vnTest[a][uc_[a]];
    }
    if (bTestPhase)
        ++nTestDecisions;
    PushWindow(uc_[0], uc_[1]);
}

double ImitationScorer::dWindowF1(int iAction) const
{
    return dF1(vnWindow[iAction][kindTP], vnWindow[iAction][kindFP], vnWindow[iAction][kindFN]);
}

double ImitationScorer::dTestF1(int iAction) const
{
    return dF1(vnTest[iAction][kindTP], vnTest[iAction][kindFP], vnTest[iAction][kindFN]);
}

double ImitationScorer::dTestMeanF1(void) const
{
    return 0.5 * (dTestF1(0) + dTestF1(1));
}
