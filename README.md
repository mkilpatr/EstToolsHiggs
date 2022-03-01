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
. ./submitall.sh
```

## Merge JSON files together for LundNet
```
. ./mergeAll.sh {directory name on EOS} {output dir}
Example: . ./mergeAll.sh nanoaod_2018_diHiggs_20Dec21_json diHiggsRegion_26Jan22_newQCD
```

# Running with LundNet and ParticleNet
Follow installation directions in the repository below
```
git clone git@github.com:mkilpatr/LundNet.git
```

## Running methods locally

```
lundnet --model [lundnet5, lundnet4, lundnet3, lundnet2, particlenet] --dir {location-for-samples} --sig {signal file name}.json.gz --bkg {background file name}.json.gz --save {save directory} --device cpu --num-epochs {Number of epochs}
```

## Running methods on POD Cluster

```
./process.py -p {directory location of samples} -o {output directory} -e {Number of epochs} -m [lundnet5, lundnet4, lundnet3, lundnet2, particlenet]
. ./submitall.sh
```

## Plotting ROC Curves
Single plot example
```
Example: python rocPlot.py -n TEST_HIGGS -f "diHiggs_hh_dyll/test_ROC_data.pickle;diHiggs_hh_diboson/test_ROC_data.pickle;diHiggs_hh_wjets/test_ROC_data.pickle" -l "Dyll;Diboson;W+jets"
```


```
. ./manyROCPlots.sh {input dir} {output dir} ## all ROC curves
. ./manySignalROCPlots.sh {input dir} {output dir} ## group ROC curves by signal
. ./manyModelROCPlots.sh {input dir} {output dir} ## group ROC curves by model
```
