#!/bin/bash

# Example using ssftool pipe from one op to the next...

ssftool extract D7000_spectrum.csv | ssftool transpose | ssftool wavelengthcalibrate blue=437,green=546,red=611 <(ssftool extract D7000_calibration.csv | ssftool transpose)  | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize 
