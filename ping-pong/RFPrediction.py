# -*- coding: utf-8 -*-
"""
Created on Fri Jan 30 15:38:02 2026

@author: Mike
"""

import joblib
import random
import numpy as np
import csv
import pandas as pd
import os
from ReadStateChanges import ReadStateChanges

wd = "/home/mike/DyALA/Workplace/"
filemodel = "RFpingpongdynamics.joblib"
fileori = "ping_pong_state.csv"
filein = "ping_pong_state_discrete.csv"
fileout = "RFPredictonResults.csv"
fileSNNres = "WorldDynamicsModel.res"

dfSNNres = pd.read_csv(wd + fileSNNres, index_col=False)

nPeriods = 4
minPeriod = 10
RacketMovementPeriod = 17.74622
nStates = [30, 30, 9, 9, 30]
offset = [0]
for i in range(len(nStates)):
    offset.append(offset[-1] + nStates[i] * nPeriods)

print(f"reading {fileori}...")
x = []
y = []
vx = []
vy = []
ry = []
with open(wd + fileori, newline = '') as fil:
    csr = csv.reader(fil)
    for row in csr:
        x.append(float(row[1]))
        y.append(float(row[2]))
        vx.append(float(row[3]))
        vy.append(float(row[4]))
        ry.append(float(row[5]))

print("Loading RF model...")
Predictors = joblib.load(wd + filemodel)    

NewCompleteStates, nTargets = ReadStateChanges(wd + filein)
ntactsperCompleteState = NewCompleteStates[-1][0] / len(NewCompleteStates)
RacketMovementPeriod = int(RacketMovementPeriod / ntactsperCompleteState)
OnlyCompleteStates = [i[1] for i in NewCompleteStates]
    
def SymbolicState(CompleteState):
    ret = []
    for i in offset[:-1]:
        ind = i
        while CompleteState[ind] == 0:
            ind += 1
        v = (ind - i) // nPeriods
        t = ind - i - v * nPeriods
        ret.append([v, t])
    return ret

def NextCompleteState(Predictors, CompleteState, SymSta):
    global tactcnt, tactcntRacket
    ExpiredStates = [i for i in range(len(SymSta) - 1) if tactcnt[i] > minPeriod * 2 ** SymSta[i][1]]
    AllowedTransfer = [True for i in CompleteState[:nTargets]]
    if len(ExpiredStates) > 0:
        for i in range(len(nStates) - 1):
            if not i in ExpiredStates:
                AllowedTransfer[offset[i]:offset[i + 1]] =[False] * (offset[i + 1] - offset[i])
    pre = [0 if i == None else i.predict_proba([CompleteState])[0][np.where(i.classes_ == 1)[0][0]] for i in Predictors]
    pmax = 0
    for i in range(nTargets):
        if CompleteState[i] == 0 and AllowedTransfer[i] and pre[i] > pmax:
            pmax = pre[i]
            Changedto = i
    if pmax == 0:
        print("No next state!")
        exit(0)
    for i in tactcnt[:-1]:
        i += ntactsperCompleteState
    dim = 0
    while offset[dim] <= Changedto:
        dim += 1
    dim -= 1
    tactcnt[dim] = 0
    NewCompleteState = CompleteState[:offset[dim]] + [0] * (nStates[dim] * nPeriods) + CompleteState[offset[dim + 1]:offset[-2]] + [0] * (nStates[-1] * nPeriods)
    NewCompleteState[Changedto] = 1
    OldRacketPosition = SymSta[-1][0]
    if tactcntRacket > RacketMovementPeriod:
        tactcntRacket -= RacketMovementPeriod
        SymSta[-1][0] += -1 if random.random() < 0.5 else 0
        SymSta[-1][0] = max(0, min(nStates[-1] - 1, SymSta[-1][0]))
    tactcntRacket += ntactsperCompleteState
    if SymSta[-1][0] != OldRacketPosition:
       tactcnt[-1] = 0 
    else:
       tactcnt[-1] += ntactsperCompleteState
    t = minPeriod
    T = tactcnt[-1]
    LongestPeriod = True
    for j in range(nPeriods - 1):
        T -= t
        if T < 0:
            LongestPeriod = False
            break
        t *= 2
    if LongestPeriod:
        j = nPeriods - 1
    NewCompleteState[offset[-2] + SymSta[-1][0] * nPeriods + j] = 1
    return NewCompleteState, SymbolicState(NewCompleteState)

fileout = wd + fileout
bAppend = os.path.exists(fileout)

with open(fileout, "r+t" if bAppend else "wt", buffering=1) as filout:
    if not bAppend:
        filout.write("xstart,ystart,vxstart,vystart,rystart,xstartdis,ystartdis,vxstartdis,vystartdis,rystartdis,realdis,pred\n");    
    else: 
        filout.readline()
    test = 0
    for tactstart in dfSNNres["tactstart"]:
        if bAppend:
            s = filout.readline()
            if len(s) == 0:
                bAppend = False
        if not bAppend:
            start = next(i for i, x in enumerate(NewCompleteStates) if x[0] > tactstart) - 1
            if start < 0:
                print(tactstart, " - some problem!")
                exit(-1)
            StartingState = OnlyCompleteStates[start]
            SymSta = SymbolicState(StartingState)
            StartingStateDis = [i[0] for i in SymSta]
            j = start + 1
            while j < len(OnlyCompleteStates) and SymbolicState(OnlyCompleteStates[j])[0][0] != 0:
                j += 1
            if j < len(OnlyCompleteStates):
                yrealdis = SymbolicState(OnlyCompleteStates[j])[1][0]
                tactcnt = [0 for i in nStates]
                tactcntRacket = 0
                k = 0
                Visited = set()
                Cycled = False
                while SymSta[0][0] != 0 and k < 3000:
                    print(f"test {test} step {k}")
                    StartingState, SymSta = NextCompleteState(Predictors, StartingState, SymSta)
                    t = tuple(tuple(item) for item in SymSta)
                    if t in Visited:
                        Cycled = True
                        break
                    k += 1
                    Visited.add(t)
                if k == 3000 or Cycled:
                    print("no prediction")
                    ypred = -1
                else: 
                    print(f"steps to prediction: {k}")
                    ypred = SymSta[1][0]
                filout.write(f"{x[tactstart]},{y[tactstart]},{vx[tactstart]},{vy[tactstart]},{ry[tactstart]},{StartingStateDis[0]},{StartingStateDis[1]},{StartingStateDis[2]},{StartingStateDis[3]},{StartingStateDis[4]},{yrealdis},{ypred}\n");    
            else:
                print(tactstart, " - another problem!")
                exit(-1)
        test += 1
            