#!/bin/bash

set -e

description='(none)'
copyright='(none)'

while getopts n:c:h: option
do
case "${option}"
in
n) description=${OPTARG};;
c) copyright=${OPTARG};;
h)  echo "Usage: $ ./dcamprof-ssf.sh [-n cameraname] [-c copyright] <jsonfile>.json"
    exit
    ;;
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
echo "dcamprof make-profile -c $@ _target.ti3 _profile.json"
dcamprof make-profile -c $@ _target.ti3 _profile.json
echo
echo "dcamprof make-icc -n "\"$description\"" -c "\"$copyright\"" -p xyzlut _profile.json $fbname.icc"
dcamprof make-icc -n "\"$description\"" -c "\"$copyright\"" -p xyzlut _profile.json $fbname.icc
rm _*
