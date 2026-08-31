#!/bin/bash


cp ../requirements/software_requirements* .
cp ../requirements/FLIGHTOS-UVIE-SRS-001-Issue_1_1.pdf .
rename software_requirements FLIGHTOS-UVIE-SRS-001_Issue_1_1 *

xelatex architectural_design.tex

makeglossaries architectural_design

biber architectural_design

xelatex architectural_design.tex

xelatex architectural_design.tex

mv architectural_design.pdf FLIGHTOS-UVIE-ADD-001-Issue_1_1.pdf
