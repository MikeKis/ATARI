# -*- coding: utf-8 -*-
"""
Created on Fri Jan 30 15:38:02 2026

@author: Mike
"""

import csv

wd = "/home/mikhail/E/DyALA/Workplace/WorldDynamicsModel/"
filein = "ping_pong_state_discrete.csv"
fileout = "ping_pong_state_complete.csv"
nPeriods = 4
minPeriod = 10
print("Reading " + filein + "...")
nStates = [30, 30, 9, 9, 30]

tactcnt = [0 for i in nStates]
LastState = [-1 for i in nStates]
with open(wd + filein, newline = '') as filin, open(wd + fileout, "wt") as filout:
    csr = csv.reader(filin)
    for row in csr:
        tact = int(row[0])
        CurrentState = [int(i) for i in row[1:]]   # row[0] - tact
        for i in range(len(nStates)):
           tactcnt[i] = 1 if CurrentState[i] != LastState[i] else tactcnt[i] + 1
        CurrentCompleteState = [[0, 0] for i in range(len(nStates))]   
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
            CurrentCompleteState[i][0] = CurrentState[i]
            CurrentCompleteState[i][1] = j
        LastState = CurrentState
        filout.write(row[0])
        for i in CurrentCompleteState:
            filout.write(f",{i[0]},{i[1]}")
        filout.write('\n')
        