# ssftool
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

## Using ssftool

ssftool is a command line tool designed to work with Unix shell piping.  The following single command line does all of the above:

```bash
$ ssftool extract DSG_4583-spectrum.csv | ssftool transpose | ssftool wavelengthcalibrate <(ssftool extract DSG_4582-calibration.csv | ssftool transpose) blue=437,green=546,red=611 | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize
```

The general format of a ssftool command is:

```bash
$ ssftool <operator> [datafile] [parameters...]
```
Each invocation expects for input either a file or standard input (stdin), and outputs to stdout. Using ssftool can be organized as separate invocations where output is piped to intermediate files, or a single command line can be strung together piping the output from one to the input of the next as in the above example.  The expected data format is comma-separated values; piping to a .csv file makes it convenient to open the file with a spreadsheet program like LibreCalc.

The following operators are supported:

- extract [file]: One of rawproc's data save format is as an average of the red, green, and blue values for each column of the image (output.data.parameters:outputmode=channelaverage).  This ssftool operator extracts each red, green, and blue line from the data file.
- transpose [file]: Turns the red-green-blue row data into columns, "row#,r,g,b"
- wavelengthcalibrate [file] calibrationfile [red=nm][,green=nm][,blue=nm]: This operator finds the red, green and blue peaks of the calibration file and uses the row indexes to map wavelengths to each row.  The user must inspect the calibration image to find the relevant peaks and assign their associated wavelengths. CFL bulbs have well-known spectrum, with a red peak at ~611nm, green at ~546nm, and blue at ~437nm.
- powercalibrate [file] calibrationfile: Applies per-wavelength power compensation from a file with this line format: wavelength,compensation.  The application is an arithmetic division.  Determining this data requires a calibrated spectrometer; data for common bulb types can be found on the internet.  For now, the wavelength lookup is "exact match"; a future enhancement will be to interpolate values where there's not an exact match.
- intervalize [file] lower,upper,interval: Turns the pixel-oriented measurements into a proper interval between a lower and upper bound.
- normalize [file]: Normalizes the r,g,b values to the range 0.0 - 1.0, with the maximum value among all three channels.
