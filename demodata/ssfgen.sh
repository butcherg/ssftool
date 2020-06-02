#!/bin/bash

set -e

#defaults:
ssftool=../ssftool
calibmarkers="blue=437,green=546,red=611"
interval="400,725,5"
outputfilespec="bar.csv"

#override defaults with ssfgen.conf
if test -f "ssfgen.conf"; then
	source ssfgen.conf
fi

#override defaults/ssfgen.conf with command line parameters:
while getopts s:c:w:i:p:o:h: option
do
case "${option}"
in
s) spectrumfile=${OPTARG};;
c) calibrationfile=${OPTARG};;
m) calibmarkers=${OPTARG};;
i) interval=${OPTARG};;
p) powerfile=${OPTARG};;
o) outputfilespec=${OPTARG};;
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
echo

echo "$ssftool extract $spectrumfile | ssftool transpose..."
$ssftool extract $spectrumfile | ssftool transpose > _spectrum.csv
echo "$ssftool extract $calibrationfile | ssftool transpose..."
$ssftool extract $calibrationfile | ssftool transpose > _calibration.csv
echo "$ssftool wavelengthcalibrate _spectrum.csv _calibration.csv $calibmarkers..."
$ssftool wavelengthcalibrate _spectrum.csv _calibration.csv $calibmarkers > _wavelength.csv
echo "$ssftool intervalize _wavelength.csv $interval..."
$ssftool intervalize _wavelength.csv $interval > _interval.csv
echo "$ssftool powercalibrate _interval.csv $powerfile..."
$ssftool powercalibrate _interval.csv $powerfile > _power.csv
echo "$ssftool normalize _power.csv > $outputfilespec..."
$ssftool normalize _power.csv > $outputfilespec
echo "removing temp files..."
rm _*


