#!/bin/bash
gcc -Wall -o strebupitecha main.c circularbuffers.c `pkg-config --cflags --libs gtk+-2.0 jack rubberband` -export-dynamic

