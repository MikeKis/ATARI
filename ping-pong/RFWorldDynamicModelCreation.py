# -*- coding: utf-8 -*-
"""
Created on Fri Jan 30 15:38:02 2026

@author: Mike
"""

from sklearn.ensemble import RandomForestClassifier
import joblib
from ReadStateChanges import ReadStateChanges

filein = "ping_pong_state_discrete.csv"
fileout = "ping_pong_state_changes.csv"
filemodel = 'RFpingpongdynamics.joblib'
RFmaxdepth = 10
RacketMovementPeriod = 17.74622

NewCompleteStates, nTargets = ReadStateChanges(filein)

ntactsperCompleteState = NewCompleteStates[-1][0] / len(NewCompleteStates)
RacketMovementPeriod = int(RacketMovementPeriod / ntactsperCompleteState)
        
with open(fileout, "w") as filout:
    for i in NewCompleteStates:
        filout.write(str(i[0]))
        for j in i[1]:
            filout.write(f",{j}")
        filout.write('\n')

OnlyCompleteStates = [i[1] for i in NewCompleteStates]   # without tacts

def getRF(ind):
    target = [i[ind] for i in OnlyCompleteStates[1:]]
    if target == [0] * len(target):
        return None
    ret = RandomForestClassifier(max_depth = RFmaxdepth)
    ret.fit(OnlyCompleteStates[:-1], target)
    return ret

print("Creating predictors...")
Predictors = []
for i in range(nTargets):
    Predictors.append(getRF(i))
    print(f"Predictor #{i} created")
    
joblib.dump(Predictors, filemodel)    
print("RF world dynamics model saved.")