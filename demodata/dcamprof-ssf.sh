#!/bin/bash

if [[ $# == 0 ]]; then
	echo "dcamprof-ssf.sh [-cnr] <ssf.json>
	echo "Options:"
	echo "	-c <copyright>   - inserts copyright statement in the icc file"
	echo "	-n <description> - inserts description in the icc file"
	echo "	-r               - adds the \"-r reports\" switch for dcamprof make-profile using the supplied report directory"
	exit
fi

set -e

description='(none)'
copyright='(none)'

while getopts n:c:h:r option
do
case "${option}"
in
n) description=${OPTARG};;
c) copyright=${OPTARG};;
h)  echo "Usage: $ ./dcamprof-ssf.sh [-n cameraname] [-c copyright] <jsonfile>.json"
    exit
    ;;
r) reports="-r reports";;
esac
done
shift $(($OPTIND -1))

if ! [[ $@ =~ \.json$ ]]; then
	echo "$@ is an invalid filename (extension is not .json)"
	exit
fi

fbname=$(basename "$@" '.json')

echo
echo "dcamprof make-target -c $@ -p cc24 _target.ti3"
dcamprof make-target -c $@ -p cc24 _target.ti3
echo
echo "dcamprof make-profile $reports -c $@ _target.ti3 _profile.json"
dcamprof make-profile $reports -c $@ _target.ti3 _profile.json
echo
echo "dcamprof make-icc -n \"$description\" -c \"$copyright\" -p xyzlut _profile.json $fbname.icc"
dcamprof make-icc -n "$description" -c "$copyright" -p xyzlut _profile.json $fbname.icc
rm _*
