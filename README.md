# ssftool And tiff2specdata
command line tool to transform spectral measurements into data for use in creating camera profiles.

## Introduction

ssftool contains a set of operations that are useful in taking measurements from recorded spectrum and transforming it into data suitable for creating custom camera profiles.  Measuring camera's spectral sensitivity for this purpose allows a profile to provide finer-grained information to the first colorspace transform, which can retain tonality in extreme colors, as well as provide better color separation from the camera's color filter array.

The first incarnation of ssftool is designed specifically to support single-image whole-spectrum captures from a spectrometer, vice successive images of single wavelength light from a monocromator.  It is also designed to support data exported from rawproc, my hack raw processor.  The general workflow for this method is:

1. Point the camera at a 30-degree offset to a diffraction grating illuminated by a broadband light source shined through a slit.
2. Take two images: 1) the broadband specturm, and 2) a calibration spectrum from a light source with wavelength-known spikes (a standard CFL bulb suffices).
3. Develop the the spectrum image to 1)demosaic, 2)flip horizontal, 3) crop the best part of the spectrum, and save to a data file (rawproc has .csv, comma-separated value, as an export image format).  
4. Develop the calibration image with the same operations, to include the exact same crop as the spectrum image.  This is important to get correct alignment of the wavelengths to the spectrum.
5. Use ssftool to do the following:
   - Format the data to R,G,B columns.
   - Assign the correct wavelengths to each row.
   - "Intervalize" the measurements to a range with an interval, e.g., 380nm to 730mn, 5nm increments.
   - Compensate each wavelength's measurements to the measured illumination power at that wavelength
   - Normalize the measurements to the range 0.0 - 1.0
   
tiff2specdata is a separate program from ssftool, designed to extract spectra from 16-bit linear TIFF images.  If the spectrum is the only thing in the TIFF, tiff2specdata uses the max value in the green channel of the entire image to find it, figures out a horizontal band of 100 pixels centered on the max green pixel, then reads that 
band right-to-left, computes the channel averages, and prints them to stdout along with the x pixel coordinate.  This output can be piled directly into ssftool at the command line

## Using ssftool and tiff2specdata

ssftool and tiff2specdata are command line tools designed to work with Unix shell piping.  The following single command line does all of the above to produce a normalized SSF dataset:

```bash
$ tiff2specdata spectrum.tiff | ssftool wavelengthcalibrate <(tiff2specdata calibration.tif ) blue=437,green=546,red=611 | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize
```
If you're using rawproc to extract the spectra, this is the command line:

```bash
$ ssftool extract DSG_4583-spectrum.csv | ssftool transpose | ssftool wavelengthcalibrate <(ssftool extract DSG_4582-calibration.csv | ssftool transpose) blue=437,green=546,red=611 | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize
```

The general format of a ssftool command is:

```bash
$ ssftool \<operator> [datafile] [parameters...]
```
Each invocation expects for input either a file or standard input (stdin), and outputs to stdout. Using ssftool can be organized as separate invocations where output is piped to intermediate files, or a single command line can be strung together piping the output from one to the input of the next as in the above example.  The expected data format is comma-separated values; piping to a .csv file makes it convenient to open the file with a spreadsheet program like LibreCalc.

The following operators are supported:

- ssftool list [\<datafile\>] ['wavelengths']  - prints the data file. 'wavelengths' prints just the wavelenghts as a comma-separated list
- ssftool extract [\<datafile\>] - extracts data from a rawproc data file.
- ssftool transpose [\<datafile\>] - turns a row-major file into column-major.
- ssftool channelmaxes [\<datafile\>] - calculates the pixel locations of each of the channel maximum values.
- ssftool wavelengthcalibrate [\<datafile>] markerstring [\<calibrationfile\>] -  calibrate either using a markerstring of \"red=www,green=www,blue=www\" to a calibration file or \"position=wavelength...\"
- ssftool powercalibrate [\<datafile>] [\<calibrationfile\>] - divide each value in the datafile by the corresponding value from the calibration file.
- ssftool normalize [\<datafile\>] - normalizes the data to the range 0.0-1.0 based on the largest channel maximum.
- ssftool intervalize [\<datafile\>] \<lowerbound\>,<upperbound\>,<interval\> [\<datafile\>] - collapses the data to the range specified by lowerbound, upperbound, and interval.
- ssftool averagechannels [\<datafile\>] - averages the r, g, and b values of each line to produce a single value for the line.
- ssftool averagefiles [\<datafile\>][...] - averages the r, g, and b values from each file to form a single r, g, and b for each line. 
- ssftool format [\<datafile>] \<precision\> - formats the w,r,g,b file to integer-ize the w, and round each r, g, and b to the specified precision. 
- ssftool smooth [\<datafile>] - applies a moving average smoothing to the data.
- ssftool linearpower \<lower,upper,interval,lowvalue,highvalue\> - builds a dataset that starts with lowvalue, then proceeds to the highvalue over the lower-to-upper interval in the specified interval.
- ssftool multiply [\<datafile>] \<number\> - multiplies each data value by the specified number.
- ssftool dcamprofjson [\<datafile\>] - produces a JSON format from the w,r,g,b data that can be ingested by dcamprof.

## Building ssftool

ssftool and tiff2specdata are C++ programs, any recent gcc will compile them.  ssftool has no particular library dependencies, tiff2specdata depends on libtiff.  A Makefile is included, `make all` will build both executables.  `sudo make install` will copy ssftool and tiff2specdata to /usr/local/bin.  If you're using Windows, MSYS2 should compile, install, and run ssftool and tiff2specdata per these instructions; any other environment is left as an exercise for the student... :D

libtiff note: Most Linux distros require the separate installation of the development headers when compiling programs that link with libtiff; in Debian distros, that can usually be done with `sudo apt-get install libtiff4-dev`

