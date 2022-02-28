# EstTools
Standalone scripts for background estimation

## General estimation methods:
```
root -l
.L LLBEstimator.C+
plot1LepCRAllEras()
LLBPredSeparate()
```

## How to run all systematics
Exemple: ./process.py -p . -c ll_systematics.conf -o uncertainties_sb_021820 -l SR_syst
```
./process.py -p {run dir} -c {conf file with list of syst macros} -o {where to send files} -l {location of macros}
```
