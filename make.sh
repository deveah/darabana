#!/bin/bash

gcc synth.c gui.c darabana.c -o darabana `pkg-config --libs --cflags gtk+-2.0` -lm -lportaudio -lsndfile -Wall -Wextra -g

