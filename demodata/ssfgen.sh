#!/bin/bash

set -e

#defaults:
ssftool=../ssftool
calibmarkers="blue=437,green=546,red=611"
interval="400,725,5"
outputfilespec="bar.csv"
precision=2

#override defaults with ssfgen.conf
if test -f "ssfgen.conf"; then
	source ssfgen.conf
fi

#override defaults/ssfgen.conf with command line parameters:
while getopts s:c:w:i:p:o:f:h: option
do
case "${option}"
in
s) spectrumfile=${OPTARG};;
c) calibrationfile=${OPTARG};;
m) calibmarkers=${OPTARG};;
i) interval=${OPTARG};;
p) powerfile=${OPTARG};;
o) outputfilespec=${OPTARG};;
f) precision=${OPTARG};;
h)  echo "Usage: $ ./ssfgen.sh [-s spectrumfile] [-c calbrationfile] [-m markers] [-i interval] [-p powerfile] [-o outputfilespec]]"
    exit
    ;;
esac
done

echo
echo "spectrumfile=$spectrumfile"
echo "calibrationfile=$calibrationfile"
echo "calibmarkers=$calibmarkers"
echo "interval=$interval"
echo "powerfile=$powerfile"
echo "outputfilespec=$outputfilespec"
echo "precision=$precision"
echo

echo "$ssftool extract $spectrumfile | ssftool transpose..."
$ssftool extract $spectrumfile | ssftool transpose > _spectrum.csv

echo "$ssftool extract $calibrationfile | ssftool transpose..."
$ssftool extract $calibrationfile | ssftool transpose > _calibration.csv

echo "$ssftool wavelengthcalibrate _spectrum.csv $calibmarkers _calibration.csv ..."
$ssftool wavelengthcalibrate _spectrum.csv $calibmarkers _calibration.csv > _wavelength.csv

echo "$ssftool intervalize _wavelength.csv $interval..."
$ssftool intervalize _wavelength.csv $interval > _interval.csv

echo "$ssftool powercalibrate _interval.csv $powerfile..."
$ssftool powercalibrate _interval.csv $powerfile > _power.csv

echo "$ssftool normalize _power.csv..."
$ssftool normalize _power.csv > _normalized.csv

echo "$ssftool format _normalized.csv $precision> $outputfilespec..."
$ssftool format _normalized.csv $precision > $outputfilespec


echo "removing temp files..."
rm _*


