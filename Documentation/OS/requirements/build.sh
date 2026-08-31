#!/bin/bash


xelatex software_requirements.tex

makeglossaries software_requirements

biber software_requirements

xelatex software_requirements.tex

xelatex software_requirements.tex

mv software_requirements.pdf FLIGHTOS-UVIE-SRS-001_Issue_1_1.pdf
