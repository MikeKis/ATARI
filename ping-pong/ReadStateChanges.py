# -*- coding: utf-8 -*-
"""
Created on Fri Jan 30 15:38:02 2026

@author: Mike
"""

import csv

def ReadStateChanges(filein):
    nPeriods = 4
    minPeriod = 10
    print("Reading " + filein + "...")
    nStates = [30, 30, 9, 9, 30]
    offset = [0]
    for i in range(len(nStates)):
        offset.append(offset[-1] + nStates[i] * nPeriods)
    tactcnt = [0 for i in nStates]
    LastState = [-1 for i in nStates]
    LastCompleteState = [0 for i in range(offset[-1])]
    NewCompleteStates = []   # it will be returned
    nTargets = sum(nStates[:-1]) * nPeriods
    with open(filein, newline = '') as filin:
        csr = csv.reader(filin)
        for row in csr:
            tact = int(row[0])
            CurrentState = [int(i) for i in row[1:]]   # row[0] - tact
            for i in range(len(nStates)):
               tactcnt[i] = 1 if CurrentState[i] != LastState[i] else tactcnt[i] + 1
            CurrentCompleteState = [0 for i in range(offset[-1])]   # without racket y
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
    return NewCompleteStates, nTargets
