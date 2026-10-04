#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Apr  3 17:04:20 2024

@author: mikhail
"""
import csv

# period_duration = 300
state_file = "ping_pong_state.csv"
label_file = "rewstastates.txt"
state_duration = 10

rewards = []
punishments = []
labels = []
with open(state_file, newline = '') as filsta:
    csr = csv.reader(filsta)
    last_bounce = -1000000
    bounced = False
    for row in csr:
        tact = int(row[0])
        x = float(row[1])
        if bounced:
            fill = 1 if x == 0. else 2
            i = len(labels) - 1
            while i >= max(0, last_bounce + state_duration * 2):
                labels[i] = fill
                i -= 1
            last_bounce = len(labels)
        labels.append(0)  
        bounced = x <= -0.5

with open(label_file, "wt") as filout:
    js = 0
    jr = 0
    current_state = -1
    for i in range(0, len(labels), state_duration):
        label = max(labels[i:i+state_duration])
        filout.write('-\n' if label == 0 else '0\n' if label == 1 else '1\n')
                
    
    
    