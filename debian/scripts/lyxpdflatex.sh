#!/bin/sh

I=$1
B=$(basename $I .tex)
O=$B.pdf
T=$B.tpt

pdflatex $I

newer $T $I && exit 0

bibtex $B

thumbpdf $O

pdflatex $I
