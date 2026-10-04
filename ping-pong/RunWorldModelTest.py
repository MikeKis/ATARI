#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Apr 13 13:14:12 2026

@author: mike
"""

import pandas as pd
import random
import subprocess

filestates = "ping_pong_state_complete.csv"
fileres = "WorldDynamicsModel.SNN.res"

nTests = 3000

df = pd.read_csv(filestates, index_col=False, header=None)

with open(fileres, "w") as filres:
    filres.write("tactstart,tactfinish,yreal,ypred\n")
    test = 0
    while test < nTests:
        nr = df.shape[0]
        indstart = random.randint(0, nr - 1)
        if df.iat[indstart, 1] > 0 and df.iat[indstart, 5] < 4:
            indfinish = indstart + 1
            while indfinish < nr and df.iat[indfinish, 1] > 0:
                indfinish += 1
            if indfinish < nr:
                ground_truth = df.iat[indfinish, 3]
                try:
                    p = subprocess.Popen(["./ArNIGPU", "../Experiments", "-e20001"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, text=True)
                    outs, errs = p.communicate(str(indstart) + '\n')
                    if p.returncode != 0 or len(errs) == 0 or errs[:4] != 'ArNI':
                        print("Abnormal ArNIGPU termination!")
                        exit(-1)
                    try:
                        res = int(errs[4:])
                        if res < 0:
                            print("Negative ArNIGPU result!")
                            exit(-1)
                    except:
                        print("Unexpected ArNIGPU result!")
                        exit(-1)
                except:
                    print("Cannot run ArNIGPU!")
                    exit(-1)
                filres.write(f"{df.iat[indstart, 0]},{df.iat[indfinish, 0]},{ground_truth},{res}\n")
                test += 1
                print(test, " tests...")
                filres.flush()
        

