// .specs/2026-10-13_ping-pong-imitation-learning.md
//
// CoLaNET genes only. action_period, racket step, racket size, ball speed,
// jitter amplitude and jitter period are fixed: they change the task, not the net.
//
// Full GA is meant to run on Linux. Comment out SMOKE_GA, point
// ArNI/ArNI_OPT.inc at this file (symlink, do not copy), rebuild SuperArNI,
// then from ping-pong/Workplace:
//   ArNI_GA 1 1 -p24 -e0.08 -M30 -oImitationPingPong.ga.log
// Do not use the default population of 300. One ArNIGPU per machine.
//
// Local smoke (SMOKE_GA defined): phases 3000/1000/1000 intervals of P,
// one network copy. Launch:
//   ArNI_GA 1 1 -p8 -M2 -oImitationPingPong.ga.smoke.log
// Changing this file's EVALUATOR requires rebuilding SuperArNI.

//#define USE_CPU
#define EMULATOR "ArNIGPU"
#define RECOGNIZER ""

#define SMOKE_GA

#define ACTION_PERIOD 15
#define DOPAMINE_PLASTICITY_TIME (ACTION_PERIOD + 3)
#define MAX_TSS_ISI (ACTION_PERIOD - 1)
#define RACKET_JITTER 0.25
#define JITTER_EVERY 10
#define NTARGETCLASSES 2
#define LEARNINGGROUPSIZE 1
#ifdef SMOKE_GA
#define NLEARNINGEXAMPLES 3000
#define NTESTEXAMPLES 1000
#define NAGENTEXAMPLES 1000
#define NNETWORKCOPIES 1
#else
#define NLEARNINGEXAMPLES 60000
#define NTESTEXAMPLES 10000
#define NAGENTEXAMPLES 10000
#define NNETWORKCOPIES 15
#endif
#define LEARNING_TIME (ACTION_PERIOD * NLEARNINGEXAMPLES)
#define AGENT_START (LEARNING_TIME + ACTION_PERIOD * NTESTEXAMPLES)
#define MAXEMULATIONTIME (AGENT_START + ACTION_PERIOD * NAGENTEXAMPLES)
#define SECOND_TEST_THRESHOLD 0x7fffffff

NetworkTestingParams ntpG{EMULATOR, RECOGNIZER, SECOND_TEST_THRESHOLD};

OPTIMIZATION_SPACE_DEFINITION
{
    OPTIMIZED_VARIABLE(DWPOS, 0.003, 0.3),
    OPTIMIZED_VARIABLE(RELDWNEG, 0.3, 1.3),
    OPTIMIZED_VARIABLE(STABILITYRESOURCECHANGERATIO, 0.001, 0.3),
    OPTIMIZED_VARIABLE_INT(LINEARIZED_PLASTICITY, 0, 1),
    OPTIMIZED_VARIABLE(MAXWEIGHT, 0.1, 3.),
    OPTIMIZED_VARIABLE(MINWEIGHT, 0.003, 3.),
    OPTIMIZED_VARIABLE(B, 0.03, 30.),
    OPTIMIZED_VARIABLE(STEPS_TO_HINGE, 3., 300.),
    OPTIMIZED_VARIABLE_INT(NLEARNINGGROUPS, 1, 30),
    OPTIMIZED_VARIABLE(WTHREXCESS, 0.001, 0.3),
    OPTIMIZED_VARIABLE(STOCHASTICSTIMULATION, 0.001, 1.),
    OPTIMIZED_VARIABLE_INT(NSILENTSYNAPSES, 1, 3000),
    OPTIMIZED_VARIABLE(SPARSITY, 0.001, 1.)
};

GENE_DEPENDENCIES
{
    GENE_DEPENDENCY(LINEARIZED_PLASTICITY, B),
    GENE_DEPENDENCY(LINEARIZED_PLASTICITY, STEPS_TO_HINGE)
};

bool bNoFatalDefect(const vector<float> &vr_) {return nint(V(NLEARNINGGROUPS)) >= 1 && V(SPARSITY) <= 1. && V(STOCHASTICSTIMULATION) < 1.;}

int StructuralCriterion(int)
{
    throw std::logic_error("StructuralCriterion used!!!");
    return 0;
}

EVALUATOR
{
    int a, SelectedDevice;
    if (!bNoFatalDefect(vr_))
            return pair<double, double>();

    SETNNP(".." FOLDER_SEPARATOR "Experiments" FOLDER_SEPARATOR "ImitationPingPong.nnp");

    EXCLUDESECTION("ONE_FACTOR_PLASTICITY");
    SUBMACRO(NNETWORKCOPIES);
    SUBMACRO(ACTION_PERIOD);
    SUBMACRO(DOPAMINE_PLASTICITY_TIME);
    SUBMACRO(MAX_TSS_ISI);
    SUBMACRO(LEARNING_TIME);
    SUBMACRO(AGENT_START);
    SUBMACRO(NTARGETCLASSES);
    SUBVALUE("RACKET_JITTER", RACKET_JITTER);
    SUBVALUE("JITTER_EVERY", JITTER_EVERY);
    SUBVALUE("PLASTICITY_MODEL", V(LINEARIZED_PLASTICITY) ? "linearized" : "smooth");
    SUBVALUE("OUTPUT", "OUT");
    SUBVALUE("INPUT", "R");
    SUBVALUE("REWARD", "Actions");
    SUBVALUE("NLEARNERS", NTARGETCLASSES * nint(V(NLEARNINGGROUPS)) * LEARNINGGROUPSIZE);
    SUBV(DWPOS);
    SUBVALUE("DWNEG", V(DWPOS) * V(RELDWNEG));
    SUBV(STABILITYRESOURCECHANGERATIO);
    SUBV(STOCHASTICSTIMULATION);
    SUBV(MAXWEIGHT);
    SUBV(MINWEIGHT);
    if (V(LINEARIZED_PLASTICITY)) {
        SUBV(B);
        SUBVALUE("HINGE_POSITION", V(DWPOS) * V(STEPS_TO_HINGE));
        SUBVALUE("INIRESOURCE0", 0.);
    } else {
        EXCLUDESECTION("LINEARIZED_PLASTICITY");
        SUBVALUE("INIRESOURCE0", rWeight0Resource(-V(MINWEIGHT), V(MAXWEIGHT)));
    }
    SUBV(WTHREXCESS);
    SUBV(SPARSITY);
    SUBVALUE("INPUT_DELAY", 1);
    SUBVINT(NSILENTSYNAPSES);
    SUBVINT(NLEARNINGGROUPS);

    unsigned CurrentExperiment = STARTING_EXPERIMENT_ID + indinPack;

    char buf[3000];
    sprintf(buf, "%d.nnc", CurrentExperiment);
    ofstream ofs(buf);
    SAVENNC(ofs);
    ofs.close();
    mutLog.lock();
    ofsTaskLog << "Testing network " << hex << idUnique << dec << " (local experimrnt id " << CurrentExperiment << ")" << endl;
    ofsTaskLog << "-------------------------------\n";
    SAVENNC(ofsTaskLog);
    ofsTaskLog << "-------------------------------\n";
    mutLog.unlock();
    stringstream ssNNC;
    SAVENNC(ssNNC);
    NNCDONE;

    sprintf(buf, "monitoring.%d.csv", CurrentExperiment);
    string strMonitoringFile(buf);
    sprintf(buf, "-e%d", CurrentExperiment);
    int ntrials = 0;
    boost::this_thread::sleep(boost::posix_time::milliseconds((CurrentExperiment % 101) * 300));
    IncrementalMean_Dispersion imd;
    vector<string> vstr_argsEmulator;
    do {
        do {
            mutsoc.lock();
            SelectedDevice = (int)(min_element(vpn_ComputingDeviceStatus.begin(), vpn_ComputingDeviceStatus.end()) - vpn_ComputingDeviceStatus.begin());
            ++vpn_ComputingDeviceStatus[SelectedDevice].first;
            mutsoc.unlock();
            mutLog.lock();
            ofsTaskLog << "Experiment " << CurrentExperiment << " - " << EMULATOR << " started at " << boost::posix_time::second_clock::local_time() << std::endl;
            mutLog.unlock();
            sprintf(buf + 300, "-T%d", MAXEMULATIONTIME);
            vstr_argsEmulator.resize(3);
            vstr_argsEmulator[0] = ".";
            vstr_argsEmulator[1] = buf;
            vstr_argsEmulator[2] = "-R";
            vstr_argsEmulator.push_back("-f" + str(LEARNING_TIME));
#ifndef USE_CPU
            sprintf(buf + 100, "-C%d", SelectedDevice);
            vstr_argsEmulator.push_back(buf + 100);
#endif
            imd = IncrementalMean_Dispersion();
            FORI(nTests) {
                a = ExecuteProcess(EMULATOR, vstr_argsEmulator, ofsTaskLog);
                if (a < 0) {
                    printf("\a\a\a Abnormal " EMULATOR " termination -- trying to restart few minutes later!!!\n");
                    mutLog.lock();
                    ofsTaskLog << "Experiment " << CurrentExperiment << " abnormal " EMULATOR " termination -- trying to restart (" << boost::posix_time::second_clock::local_time() << ")\n";
                    mutLog.unlock();
                    break;
                }
                if (a >= SHOULD_BE_LAUNCHED_ON_ANOTHER_NODE)
                    break;
                imd.AddMonovalue(a);
            }
            mutsoc.lock();
            --vpn_ComputingDeviceStatus[SelectedDevice].first;
            mutsoc.unlock();
            mutLog.lock();
            ofsTaskLog << "Experiment " << CurrentExperiment << " - " << EMULATOR << " finished at " << boost::posix_time::second_clock::local_time() <<  " returned  " << a << std::endl;
            mutLog.unlock();
            if (a >= 0)
                break;
            boost::this_thread::sleep(boost::posix_time::seconds(200));
            ntrials++;
        } while (ntrials < 30);
        if (ntrials == 30)
            do {
                printf("CANNOT SPAWN!\n");
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            } while (true);
        if (a < SHOULD_BE_LAUNCHED_ON_ANOTHER_NODE) {
            remove(strMonitoringFile.c_str());
            sprintf(buf, "%d.finsave.nns", CurrentExperiment);
            remove(buf);
            return make_pair(imd.dmean(), imd.dstandard_deviation());
        }
        mutsoc.lock();
        ++vpn_ComputingDeviceStatus[SelectedDevice].first;
        mutsoc.unlock();
        string strLog;
        time_t tim = time(NULL);
        ctime_s(buf + 200, 100, &tim);
        mutLog.lock();
        ofsTaskLog << "Experiment " << CurrentExperiment << " - task sent to another cluster node at " << boost::posix_time::second_clock::local_time() << " (device " << SelectedDevice << ")" << endl;
        mutLog.unlock();
        auto pdd_ret = pdd_CoordinateCluster(a - SHOULD_BE_LAUNCHED_ON_ANOTHER_NODE, SelectedDevice, vstr_argsEmulator, nTests, ssNNC.str(), strLog);
        if (strLog.length()) {
            mutsoc.lock();
            --vpn_ComputingDeviceStatus[SelectedDevice].first;
            mutsoc.unlock();
            mutLog.lock();
            ofsTaskLog << "Experiment " << CurrentExperiment << " result: " << pdd_ret.first << '/' << pdd_ret.second << endl;
            ofsTaskLog << strLog << "\n-------------------\n";
            mutLog.unlock();
            return pdd_ret;
        }
        mutLog.lock();
        ofsTaskLog << "Experiment " << CurrentExperiment << " -- too long waiting for result -- restarting on another device\n";
        mutLog.unlock();
    } while (true);
}
