#!/bin/bash
gcc -Wall -o strebupitecha main.c `pkg-config --cflags --libs gtk+-2.0 jack` -export-dynamic

