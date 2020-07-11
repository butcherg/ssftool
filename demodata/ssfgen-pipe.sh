#!/bin/bash

# Example using ssftool pipe from one op to the next...

ssftool extract DSG_4583-spectrum.csv | ssftool transpose | ssftool wavelengthcalibrate blue=437,green=546,red=611 <(ssftool extract DSG_4582-calibration.csv | ssftool transpose)  | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize 
