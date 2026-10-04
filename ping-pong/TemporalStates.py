# -*- coding: utf-8 -*-
"""
Created on Fri Jan 30 15:38:02 2026

@author: Mike
"""

import csv
import random
from sklearn.ensemble import RandomForestClassifier
import numpy as np


filein = "ping_pong_state_discrete.csv"
fileout = "ping_pong_state_changes.csv"
minPeriod = 10
nPeriods = 4
nStates = [30, 30, 9, 9, 30]
RacketMovementPeriod = 17.74622

print("Reading " + filein + "...")
offset = [0]
for i in range(len(nStates)):
    offset.append(offset[-1] + nStates[i] * nPeriods)
tactcnt = [0 for i in nStates]
LastState = [-1 for i in nStates]
LastCompleteState = [0 for i in range(offset[-1])]
NewCompleteStates = []
nTargets = sum(nStates[:-1]) * nPeriods
with open(filein, newline = '') as filin:
    csr = csv.reader(filin)
    for row in csr:
        tact = int(row[0])
        CurrentState = [int(i) for i in row[1:]]
        for i in range(len(nStates)):
           tactcnt[i] = 1 if CurrentState[i] != LastState[i] else tactcnt[i] + 1
        CurrentCompleteState = [0 for i in range(offset[-1])]
        for i in range(len(nStates)):
            t = minPeriod
            T = tactcnt[i]
            LongestPeriod = True
            for j in range(nPeriods - 1):
                T -= t
                if T < 0:
                    LongestPeriod = False
                    break
                t *= 2
            if LongestPeriod:
                j = nPeriods - 1
            CurrentCompleteState[offset[i] + CurrentState[i] * nPeriods + j] = 1
        if CurrentCompleteState[:nTargets] != LastCompleteState[:nTargets]:
            LastCompleteState = CurrentCompleteState
            NewCompleteStates.append([tact, CurrentCompleteState])
        LastState = CurrentState
    
ntactsperCompleteState = NewCompleteStates[-1][0] / len(NewCompleteStates)
RacketMovementPeriod = int(RacketMovementPeriod / ntactsperCompleteState)
        
with open(fileout, "w") as filout:
    for i in NewCompleteStates:
        filout.write(str(i[0]))
        for j in i[1]:
            filout.write(f",{j}")
        filout.write('\n')

OnlyCompleteStates = [i[1] for i in NewCompleteStates]

def getRF(ind):
    target = [i[ind] for i in OnlyCompleteStates[1:]]
    if target == [0] * len(target):
        return None
    ret = RandomForestClassifier(max_depth = 100)
    ret.fit(OnlyCompleteStates[:-1], target)
    return ret

print("Creating predictors...")
Predictors = []
for i in range(nTargets):
    Predictors.append(getRF(i))
    print(f"Predictor #{i} created")
indstate = int(random.random() * len(OnlyCompleteStates))

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

def PrintState(SymSta):
    return f"x{SymSta[0][0]}" + \
           '{' + \
           f"{SymSta[0][1]}" + \
           "}," + \
           f"y{SymSta[1][0]}" + \
           '{' + \
           f"{SymSta[1][1]}" + \
           "}," + \
           f"vx{SymSta[2][0] - 4}" + \
           '{' + \
           f"{SymSta[2][1]}" + \
           "}," + \
           f"vy{SymSta[3][0] - 4}" + \
           '{' + \
           f"{SymSta[3][1]}" + \
           "}," + \
           f"ry{SymSta[4][0]}" + \
           '{' + \
           f"{SymSta[4][1]}" + \
           "}"

StartingState = OnlyCompleteStates[indstate]
SymSta = SymbolicState(StartingState)
print(PrintState(SymSta))

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

tactcnt = [0 for i in nStates]
tactcntRacket = 0
for i in range(100):
    StartingState, SymSta = NextCompleteState(Predictors, StartingState, SymSta)
    print(PrintState(SymSta))
    
    


