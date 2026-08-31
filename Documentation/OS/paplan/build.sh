#!/bin/bash


xelatex paplan.tex

makeglossaries paplan

biber paplan

xelatex paplan.tex

xelatex paplan.tex

mv paplan.pdf FLIGHTOS-UVIE-PAQ-001_Issue_1_1.pdf
