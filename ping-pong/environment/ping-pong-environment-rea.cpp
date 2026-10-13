/*
Copyright (C) 2022 Mikhail Kiselev

Emulates ping-pong game.

*/
// .specs/2026-10-13_ping-pong-imitation-learning.md

#include <fstream>
#include <iostream>
#include <string>

#include "../ping-pong-environment.h"
#include "EnvironmentState.hpp"
#include "ImitationScorer.hpp"

using namespace std;

extern int tactStart;
extern int ntact;
extern EnvironmentState es;

extern int nRewards;
extern int nPunishments;

extern int nActionCommandPeriod;
extern bool bGridTact;
extern bool bFlyingLeft;
extern RacketCommand cmdTeacher;
extern int nTeacherDown;
extern int nTeacherUp;
extern int nLeftDecisions;

int nNeuronsperAction = 0;
int nAgentStartTime = 0x7fffffff;
int nRewardsAgent = 0;
int nPunishmentsAgent = 0;

static ImitationScorer scorer;
static string strFitness = "f1";
static string strReportFile;
static int nLogPeriod = 200000;
static int nWindowDecisions = 2000;
static float rRacketJitter = 0.F;
static int nJitterEvery = 10;
static int nJitterShifts = 0;
static ofstream ofsReport;

static void Refuse(const char *pchParameter, const char *pchWhy)
{
    cout << pchParameter << " " << pchWhy << endl;
    exit(-1);
}

static void WriteF1(const char *pchPhase)
{
    double dDown = scorer.dWindowF1(0);
    double dUp = scorer.dWindowF1(1);
    double dMean = 0.5 * (dDown + dUp);
    cout << "f1 tact " << ntact << " phase " << pchPhase
         << " down " << dDown << " up " << dUp << " mean " << dMean << endl;
    if (ofsReport.is_open())
        ofsReport << ntact << ',' << pchPhase << ',' << dDown << ',' << dUp << ',' << dMean << endl;
}

READOUT_SET_PARAMETERS(ExperimentId, tactTermination, nOutputNeurons, xn)
{
    if (nOutputNeurons < 0 || nOutputNeurons % 2)
        Refuse("nOutputNeurons", "must be divisible by 2");
    nNeuronsperAction = (int)nOutputNeurons / 2;
    int nStart = 0;
    int nAgent = -1;
    int nLog = 200000;
    int nWindow = 2000;
    float rJitter = 0.F;
    int nEvery = 10;
    string strFit = "f1";
    string strReport;
    GetAllParameters(xn,
        "start_time", &nStart,
        "agent_start_time", &nAgent,
        "fitness", &strFit,
        "log_period", &nLog,
        "report_file", &strReport,
        "window", &nWindow,
        "racket_jitter", &rJitter,
        "jitter_every", &nEvery);
    if (strFit != "f1" && strFit != "hits")
        Refuse("fitness", "must be f1 or hits");
    if (nLog < 1)
        Refuse("log_period", "must be >= 1");
    if (xn.child("window") && nWindow < 1)
        Refuse("window", "must be >= 1");
    if (rJitter < 0.F)
        Refuse("racket_jitter", "must be >= 0");
    if (xn.child("jitter_every") && nEvery < 1)
        Refuse("jitter_every", "must be >= 1");
    if (nStart < 0)
        Refuse("start_time", "must be >= 0");
    tactStart = nStart;
    if (xn.child("agent_start_time")) {
        if (nAgent < 0)
            Refuse("agent_start_time", "must be >= 0");
        nAgentStartTime = nAgent;
    } else nAgentStartTime = tactTermination > 0 ? tactTermination : 0x7fffffff;
    strFitness = strFit;
    nLogPeriod = nLog;
    nWindowDecisions = nWindow;
    rRacketJitter = rJitter;
    nJitterEvery = nEvery;
    strReportFile = strReport;
    nJitterShifts = 0;
    nRewardsAgent = 0;
    nPunishmentsAgent = 0;
    scorer.Configure(nOutputNeurons, nWindowDecisions);
    if (strReportFile.length()) {
        ofsReport.open(strReportFile);
        if (!ofsReport.is_open())
            Refuse("report_file", "cannot be opened");
        ofsReport << "tact,phase,f1_down,f1_up,f1_mean\n";
    }
}

READOUT_OBTAIN_SPIKES(v_Firing)
{
    if (!v_Firing.empty())
        scorer.Observe(&v_Firing[0], (int)v_Firing.size());
    float rBefore = es.prRacket ? *es.prRacket : 0.F;
    if (bGridTact && bFlyingLeft) {
        bool bTest = ntact >= tactStart && ntact < nAgentStartTime;
        bool bLearn = ntact < tactStart;
        if (bLearn || bTest)
            scorer.Score(cmdTeacher, bTest);
        RacketCommand cmd = ntact >= nAgentStartTime ? scorer.cmdTrigger(NULL) : cmdTeacher;
        ApplyRacketCommand(cmd);
        if (ntact < nAgentStartTime && rRacketJitter > 0.F && nJitterEvery > 0 && nLeftDecisions % nJitterEvery == 0) {
            JitterRacket(rRacketJitter);
            ++nJitterShifts;
        }
    }
    if (bGridTact)
        scorer.ClearWindow();
    if (ntact >= nAgentStartTime) {
        static int NoMoveTacts = 0;
        if (es.prRacket && *es.prRacket == rBefore) {
            if (++NoMoveTacts == 100000)
                return false;
        } else NoMoveTacts = 0;
    }
    if (nLogPeriod > 0 && ntact > 0 && ntact % nLogPeriod == 0 && ntact < nAgentStartTime)
        WriteF1(ntact < tactStart ? "learn" : "test");
    if (!(ntact % 200000)) {
        cout << "rew " << nRewards << " pun " << nPunishments << endl;
        nRewards = nPunishments = 0;
    }
    return true;
}

READOUT_FINALIZE(OriginalTerminationCode, filGenLog)
{
    cout << "rew " << nRewards << " pun " << nPunishments << endl;
    int nTP0 = scorer.NTest(0, 3), nFP0 = scorer.NTest(0, 1), nFN0 = scorer.NTest(0, 2), nTN0 = scorer.NTest(0, 0);
    int nTP1 = scorer.NTest(1, 3), nFP1 = scorer.NTest(1, 1), nFN1 = scorer.NTest(1, 2), nTN1 = scorer.NTest(1, 0);
    auto precision = [](int nTP, int nFP) {return nTP + nFP ? (double)nTP / (nTP + nFP) : 0.;};
    auto recall = [](int nTP, int nFN) {return nTP + nFN ? (double)nTP / (nTP + nFN) : 0.;};
    double dF1Down = scorer.dTestF1(0);
    double dF1Up = scorer.dTestF1(1);
    double dMean = scorer.dTestMeanF1();
    cout << "summary"
         << " tp_down " << nTP0 << " fp_down " << nFP0 << " fn_down " << nFN0 << " tn_down " << nTN0
         << " tp_up " << nTP1 << " fp_up " << nFP1 << " fn_up " << nFN1 << " tn_up " << nTN1
         << " precision_down " << precision(nTP0, nFP0) << " recall_down " << recall(nTP0, nFN0)
         << " f1_down " << dF1Down
         << " precision_up " << precision(nTP1, nFP1) << " recall_up " << recall(nTP1, nFN1)
         << " f1_up " << dF1Up << " f1_mean " << dMean
         << " both " << scorer.NBothSpiked() << endl;
    cout << "imitation_summary left=" << nLeftDecisions
         << " down=" << nTeacherDown << " up=" << nTeacherUp
         << " jitter=" << nJitterShifts << " both=" << scorer.NBothSpiked() << endl;
    if (ofsReport.is_open()) {
        ofsReport << "summary"
                  << " tp_down " << nTP0 << " fp_down " << nFP0 << " fn_down " << nFN0 << " tn_down " << nTN0
                  << " tp_up " << nTP1 << " fp_up " << nFP1 << " fn_up " << nFN1 << " tn_up " << nTN1
                  << " f1_down " << dF1Down << " f1_up " << dF1Up << " f1_mean " << dMean
                  << " both " << scorer.NBothSpiked() << "\n";
        ofsReport << "imitation_summary left=" << nLeftDecisions
                  << " down=" << nTeacherDown << " up=" << nTeacherUp
                  << " jitter=" << nJitterShifts << " both=" << scorer.NBothSpiked() << "\n";
        ofsReport.close();
    }
    if (filGenLog) {
        fprintf(filGenLog, "imitation_summary left=%d down=%d up=%d jitter=%d both=%d f1_mean %g\n",
            nLeftDecisions, nTeacherDown, nTeacherUp, nJitterShifts, scorer.NBothSpiked(), dMean);
    }
    if (strFitness == "hits") {
        int nDen = nRewardsAgent + nPunishmentsAgent;
        if (!nDen)
            return 1;
        return nRewardsAgent * 10000 / nDen;
    }
    if (!scorer.NTestDecisions())
        return 1;
    return (int)(10000. * dMean + 0.5);
}

DYNAMIC_LIBRARY_ENTRY_POINT void Serialize(Serializer &ser, bool bSave) {}
