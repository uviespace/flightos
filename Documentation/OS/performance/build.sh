#!/bin/bash


xelatex performance.tex

makeglossaries performance

biber performance

xelatex performance.tex

xelatex performance.tex

mv performance.pdf FLIGHTOS-UVIE-PERF-001_Issue_0_2.pdf
