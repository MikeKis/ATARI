#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jul  2 17:44:30 2025

@author: mikhail
"""

experiment = 10000
start_tact = 1499990
ntacts = 1000
state_inertia = 0

monitoring_file = f"monitoring.{experiment}.csv"
protocol_file = f"spikes.{experiment}.txt"

states = {}
predictions = {}
with open(monitoring_file) as filmon:
    while filmon.readline()[:10] != "neu,200000":
        continue
    while True:
        s = filmon.readline()
        if s[:3] != "neu" and s[:3] != "lin":
            break
        if s[:3] == "neu" and '{' in s:
            lstr = s.rstrip().split(',')
            neu = int(lstr[2])
            meaning = lstr[15][1:-1]
            if not "PRED_" in meaning:
                if len(meaning) <= 7:
                    states[neu] = [meaning, 1000]
            else:
                predictions[neu] = meaning[5:]
                
with open(protocol_file) as filpro, open("ping_pong_state.csv") as filsta, open("inf.txt", "wt") as filinf:
    if start_tact > 10:
        filpro.seek(len(filpro.readline()) * (start_tact - 10))
    for i in range(start_tact):
        filsta.readline()
    for i in range(min(10, start_tact)):
        spikes = filpro.readline()
        for j in states.keys():
            if spikes[j] == '.':
                states[j][1] += 1
            else:
                states[j][1] = 0
    for i in range(ntacts):
        spikes = filpro.readline()
        state = filsta.readline().rstrip()
        filinf.write(state + ' ')
        for j in states.keys():
            if spikes[j] == '.':
                states[j][1] += 1
            else:
                states[j][1] = 0
            if states[j][1] <= state_inertia:
                filinf.write(states[j][0] + ' ')
        filinf.write("-> ")
        for j in predictions.keys():
            if spikes[j] == '@':
                filinf.write(predictions[j] + ' ')
        filinf.write('\n')
        
                