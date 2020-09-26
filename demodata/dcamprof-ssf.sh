#!/bin/bash

if [[ $# == 0 ]]; then
	echo "dcamprof-ssf.sh [-options] <ssf.json>"
	echo "Options:"
	echo "  -c <copyright>   - inserts copyright statement in the icc file"
	echo "  -n <description> - inserts description in the icc file"
	echo "  -r               - enables report generation for dcamprof make-profile"
	echo "  -s <spectra>     - uses the specified spectra, choose from cc24," 
	echo "                     kuopio-natural, munsell, or munsell-bright"
	echo "                     (default: cc24)"
	echo "  -f <filespec>    - specifies a filespec to use for the product "
	echo "                     filenames instead of the input filespec"
	echo "  -l               - log dcamprof output to filespec.log"
	echo "  -t xyzlut|matrix    - make lut or matrix profile, default=lut"
	exit
fi

set -e

description='(none)'
copyright='(none)'
spectra='cc24'
profiletype='-p xyzlut'

#load configuration file, if present:
if test -f "dcamprof-ssf.conf"; then
        source dcamprof-ssf.conf
fi

while getopts n:c:f:t:rl option
do
case "${option}"
in
n) description=${OPTARG};;
c) copyright=${OPTARG};;
r) reportflag='-r';;
s) spectra=${OPTARG};;
f) fbaltname=${OPTARG};;
l) logredirect='-l';;
t) profiletype="-p ${OPTARG}";;
esac
done
shift $(($OPTIND -1))

if ! [[ $@ =~ \.json$ ]]; then
	echo "$@ is an invalid filename (extension is not .json)"
	exit
fi

if [ -n "$fbaltname" ]; then
	fbname=$fbaltname
else
	fbname=$(basename "$@" '.json')
fi

if [ -n "$reportflag" ]; then
	reportflag="-r $fbname-reports"
fi

if [ -n "$logredirect" ]; then
	#logredirect="&>> $fbname-dcamprof.log" # 2>&1...
	exec 2> $fbname-dcamprof.log
fi

echo
echo "#Configuration variables:"
echo "description=\"$description\""
echo "copyright=\"$copyright\""
echo "spectra=$spectra"
echo "profiletype=$profiletype"
echo
echo "dcamprof make-target -c $@ -p $refspectra _target.ti3"
dcamprof make-target -c $@ -p $spectra _target.ti3
echo
echo "dcamprof make-profile $reportflag $fbname-reports  -c $@ _target.ti3 _profile.json"
dcamprof make-profile $reportflag -c $@ _target.ti3 _profile.json
echo
echo "dcamprof make-icc -n \"$description\" -c \"$copyright\" $profiletype _profile.json $fbname.icc"
dcamprof make-icc -n "$description" -c "$copyright" $profiletype _profile.json $fbname.icc
rm _*

