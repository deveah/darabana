#!/bin/bash

gcc darabana.c -o darabana `pkg-config --libs --cflags gtk+-2.0` -lm -lportaudio -lsndfile -Wall -Wextra -g

