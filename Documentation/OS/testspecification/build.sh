#!/bin/bash


cp ../testplan/testplan* .
cp ../testplan/FLIGHTOS-UVIE-TP-001-Issue_1_1.pdf .
rename testplan FLIGHTOS-UVIE-TP-001-Issue_1_1 *

xelatex testspecification.tex

makeglossaries testspecification

biber testspecification

xelatex testspecification.tex

xelatex testspecification.tex

mv testspecification.pdf FLIGHTOS-UVIE-TS-001_Issue_1_1.pdf
