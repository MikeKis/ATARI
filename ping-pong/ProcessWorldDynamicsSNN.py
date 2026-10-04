# -*- coding: utf-8 -*-
"""
Created on Mon Oct 20 11:52:26 2025

@author: Kiselev_Mi
"""

import snncsv

wd = "e:/DyALA/Workplace/"
fileori = "WorldDynamicsModel.snn.csv"
fileres = "WorldDynamicsModel.upper.snn.csv"

with open(wd + fileori, 'rt') as filin, open(wd + fileres, 'wt') as filout:
    lstr = filin.readlines()
    lstr = [s.strip() for s in lstr]
    snncsv.DeletePopulation(lstr, "CONSTGATET", True)
    snncsv.DeletePopulation(lstr, "STARTERT", True)
    snncsv.DeletePopulation(lstr, "MEMORYT", True, 0)
    snncsv.DeletePopulation(lstr, "ENDINDICATORT", True)
    snncsv.DeletePopulation(lstr, "BIASGATEenter", True)
    snncsv.DeletePopulation(lstr, "BIASGATEcont", True)
    snncsv.DeletePopulation(lstr, "N", True)
    snncsv.DeletePopulation(lstr, "Punishment", False)
    snncsv.DeletePopulation(lstr, "Reward", False)
    snncsv.DeletePopulation(lstr, "R", False)
    snncsv.DeletePopulation(lstr, "Actions", False)
    snncsv.DeletePopulation(lstr, "RandomActions", False)
    snncsv.DeleteLinks(lstr, type = "reward")
    lstr.append("R,0,432")
    for i in lstr:
        filout.write(i + '\n')
