#!/bin/bash

if [ "$#" -eq 1 ]; then

name=$(basename "$1" .csv)
read -r -d '' script<<-EOFMarker
set datafile separator ','
set terminal png size 640,480
set output '$1.png'
set title '$1' noenhanced
unset key
set xlabel 'wavelength (nm)'
set linetype 1 lw 2 lc rgb 'red'
set linetype 2 lw 2 lc rgb 'green'
set linetype 3 lw 2 lc rgb 'blue'
plot '$1' using 1:2 with lines, '' using 1:3 with lines, '' using 1:4 with lines
EOFMarker

elif [ "$#" -eq 2 ]; then

name1=$(basename "$1" .csv)
name2=$(basename "$2" .csv)
read -r -d '' script<<-EOFMarker
set datafile separator ','
set terminal png size 640,480
set output '$name1-vs-$name2.png'
set title '$name1 vs $name2' noenhanced
unset key
set xlabel 'wavelength (nm)'
set linetype 1 lw 2 lc rgb 'red'
set linetype 2 lw 2 lc rgb 'green'
set linetype 3 lw 2 lc rgb 'blue'
set linetype 4 lw 1 lc rgb 'red'
set linetype 5 lw 1 lc rgb 'green'
set linetype 6 lw 1 lc rgb 'blue'
plot '$1' using 1:2 with lines, '' using 1:3 with lines, '' using 1:4 with lines, '$2'  using 1:2 with lines, '' using 1:3 with lines, '' using 1:4 with lines
EOFMarker

fi

echo "$script" > who.gnu
gnuplot who.gnu
rm who.gnu
