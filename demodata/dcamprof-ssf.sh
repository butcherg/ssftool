#!/bin/bash

if [[ $# == 0 ]]; then
	echo "dcamprof-ssf.sh [-cnr] <ssf.json>"
	echo "Options:"
	echo "	-c <copyright>   - inserts copyright statement in the icc file"
	echo "	-n <description> - inserts description in the icc file"
	echo "	-r               - enables report generation for dcamprof make-profile"
	echo "  -s <spectra>     - uses the specified spectra (default: cc24)"
	echo "  -f <filespec>	 - specifies a filespec to use for the product filenames instead of the input filespec"
	exit
fi

set -e

description='(none)'
copyright='(none)'
refspectra='cc24'

#load configuration file, if present:
if test -f "dcamprof-ssf.conf"; then
        source dcamprof-ssf.conf
fi

while getopts n:c:h:f:r option
do
case "${option}"
in
n) description=${OPTARG};;
c) copyright=${OPTARG};;
h)  echo "Usage: $ ./dcamprof-ssf.sh [-n cameraname] [-c copyright] <jsonfile>.json"
    exit
    ;;
r) reportflag="-r";;
s) refspectra=${OPTARG};;
f) fbaltname=${OPTARG};;
esac
done
shift $(($OPTIND -1))

if ! [[ $@ =~ \.json$ ]]; then
	echo "$@ is an invalid filename (extension is not .json)"
	exit
fi

if [ -z ${fbaltname+x} ]; then
	fbname=$(basename "$@" '.json')
else
	fbname=$fbaltname
fi

if [ -z ${reportflag+x} ]; then
	reportflag="-r $fbname-reports"
fi

echo
echo "dcamprof make-target -c $@ -p $refspectra _target.ti3"
dcamprof make-target -c $@ -p $refspectra _target.ti3
echo
echo "dcamprof make-profile $reportflag $fbname-reports  -c $@ _target.ti3 _profile.json"
dcamprof make-profile $reportflag -c $@ _target.ti3 _profile.json
echo
echo "dcamprof make-icc -n \"$description\" -c \"$copyright\" -p xyzlut _profile.json $fbname.icc"
dcamprof make-icc -n "$description" -c "$copyright" -p xyzlut _profile.json $fbname.icc
rm _*
