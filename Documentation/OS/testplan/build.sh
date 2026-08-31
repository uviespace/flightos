#!/bin/bash


xelatex testplan.tex

makeglossaries testplan

biber testplan

xelatex testplan.tex

xelatex testplan.tex

mv testplan.pdf FLIGHTOS-UVIE-TP-001-Issue_1_1.pdf
