#!/bin/bash

name=$1
CMSSW=${CMSSW_BASE##*/}
DIR=$(pwd)

cd $CMSSW_BASE
cd ..
eosrm /store/user/${USER}/${CMSSW}_${name}.tgz
tar --exclude-caches-all -zcf ${CMSSW}_${name}.tgz $CMSSW --exclude="$CMSSW/src/AnalysisMethods/*" --exclude="$CMSSW/src/AnalysisBase/*" --exclude="$CMSSW/src/.git/*" --exclude="*.root" --exclude="*.pdf" --exclude="*.png" --exclude="*.log" --exclude="*.err" --exclude="*.out" --exclude="*.txt" --exclude="*.tex" --exclude="*.sh" --exclude="AnalysisTools/*" --exclude="PhysicsTools/*.root" --exclude="AnalysisMethods/EstTools/datacard_moriond17/*" --exclude="$CMSSW/src/AnalysisMethods/EstTools/SUSYNano19/*" --exclude="$CMSSW/src/AnalysisMethods/EstTools/HGCStudies/*" --exclude="$CMSSW/src/TopTagger*" --exclude="$CMSSW/src/HiggsAnalysis/*" --exclude="$CMSSW/src/CombineHarvester/*" --exclude="$CMSSW/src/Limits/*"
xrdcp -f ${CMSSW}_${name}.tgz root://cmseos.fnal.gov//store/user/${USER}/${CMSSW}_${name}.tgz
rm -f ${CMSSW}_${name}.tgz
cd $DIR
