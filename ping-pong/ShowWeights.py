# -*- coding: utf-8 -*-
"""
Created on Wed Aug 27 14:34:22 2025

@author: Kiselev_Mi
"""

import numpy as np
from sklearn.mixture import BayesianGaussianMixture
import matplotlib as mpl
import matplotlib.pyplot as plt

nnsfile = "WorldDynamics.nns.csv"

with open(nnsfile) as filnns:
    lstr = filnns.readlines()
    lstr = [s.strip() for s in lstr]

def w(res, wmin, wmax):
    return wmin + (wmax - wmin) * max(res, 0) / (wmax - wmin + max(res, 0))

def get_weights(population, nstates):
    ind = lstr.index("$PopulationProperties")
    indbeg = ind + 3
    ind = indbeg
    TargetPopulations = []
    while lstr[ind][0] != '$':
        if lstr[ind].split('#')[0] == population:
            TargetPopulations.append(ind - indbeg)
        ind += 1
    indNeurons = [i for i, val in enumerate(lstr) if val == "$Neurons"]
    indLinks = [i for i, val in enumerate(lstr) if val == "$Links"]
    ret = np.array([])
    totsrc = []
    for i in TargetPopulations:
        Neurons = [s.split(',') for s in lstr[indNeurons[i] + 3: indLinks[i]]]
        Neurons.sort(key = lambda l: int(l[0]))    # probably, it is sorted already
        Links = [s.split(',') for s in (lstr[indLinks[i] + 3: indNeurons[i + 1]] if i + 1 < len(indNeurons) else lstr[indLinks[i] + 3:])]
        Links.sort(key = lambda l: int(l[0]) + (0 if l[1] == "plastic" else 1000000))
        allres = []
        ind = 0
        for s in Neurons:
            neu = s[0]
            wmin = int(s[-2]) * 0.001
            wmax = int(s[-1]) * 0.001
            res = []
            src = []
            while Links[ind][0] == neu:
                res.append(w(int(Links[ind][-1]) * 0.001, wmin, wmax))
                src.append(int(Links[ind][-2]))   # assumed that sources are identical for all these neurons
                ind += 1
            allres.append(res)
            if i == TargetPopulations[0]:
                totsrc.append(src)
        allres = np.array(allres)
        allres = allres.reshape((nstates, allres.shape[0] // nstates, allres.shape[1]))
        if ret.ndim == 1:
            ret = allres
            maxnclusters = allres.shape[1]
        else:
            ret = np.concatenate((ret, allres), axis = 1)
    totsrc = [totsrc[i] for i in range(0, len(totsrc), len(totsrc) // nstates)]
    return ret, maxnclusters, totsrc, wmin, wmax

indMemoryT = np.array(range(217, 217 + 108 * 4)).reshape((108, 4)).transpose()
wplaces = [np.array([-257, -258]), indMemoryT[:,:30], indMemoryT[:,30:60], indMemoryT[:,60:69], indMemoryT[:,69:78], indMemoryT[:,78:]]
maxnweightcolumns = 30

def show_weights(aw, asrc, wmin, wmax):
    wshowed = []
    for i in wplaces:
        wshowed.append(i.astype(np.float64))
        for j in range(len(wshowed[-1].flat)):
            wshowed[-1].flat[j] = aw[asrc.index(int(wshowed[-1].flat[j]))] if int(wshowed[-1].flat[j]) in asrc else 0
    wshowedaligned = []
    for i in wshowed:
        if i.ndim == 1:
            wshowedaligned.append([i[j] if j < len(i) else 0. for j in range(maxnweightcolumns)])
        else:
            for k in i:
                wshowedaligned.append([k[j] if j < len(k) else 0. for j in range(maxnweightcolumns)])
    norm = mpl.colors.TwoSlopeNorm(vmin = wmin, vcenter = 0, vmax = wmax)
    fig, ax = plt.subplots()
#    ax.axis('off')
    ax.imshow(wshowedaligned, cmap = 'seismic', norm = norm)
    plt.show()

weights_enter, ncluenter, srcenter, wminenter, wmaxenter = get_weights("Lenter", 108)
weights_cont, nclucont, srccont, wmincont, wmaxcont = get_weights("Lcont", 108 * 3)

def show_typical_weights(state, nclusters = 0):
    
    weights_to_show = weights_enter[state]
    
    bgm = BayesianGaussianMixture(n_components = ncluenter if nclusters == 0 else nclusters).fit(weights_to_show)
    
    for i in bgm.means_:
        show_weights(i, srcenter[state], wminenter, wmaxenter)

def show_typical_weights_cont(state, period, nclusters = 0):
    
    per = period - 1
    weights_to_show = weights_cont[state * 3 + per]
    
    bgm = BayesianGaussianMixture(n_components = nclucont if nclusters == 0 else nclusters).fit(weights_to_show)
    
    for i in bgm.means_:
        show_weights(i, srccont[state * 3 + per], wmincont, wmaxcont)

show_typical_weights_cont(0, 2, 3)
        
        
        
        