# EstTools
Standalone scripts for background estimation general methods to make plots of kinematically interesting variables. Includes methods to merge JSON files for training, validation, and testing using LundNet and ParticleNet

## General estimation methods locally:
```
root -l
.L HiggsEstimator.C+
plotHtoTaus()
```

## How to run all plots on Condor
```
./process.py -p {run dir} -m {macro.C} -o {where to send files} -l {location of macros} -b {variables.conf}
Example: ./process.py -p . -m HiggsEstimator.C -o Tau_training_21Dec21_LundVars -l . -b plotVars.conf
```

## Merge JSON files together for LundNet
```
. ./mergeAll.sh {directory name on EOS} {output dir}
Example: . ./mergeAll.sh nanoaod_2018_diHiggs_20Dec21_json diHiggsRegion_26Jan22_newQCD
```


