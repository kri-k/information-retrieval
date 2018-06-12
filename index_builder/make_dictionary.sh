#!/bin/bash

g++ --std=c++17 -march=native -O3 -o make_dictionary.out make_dictionary.cpp
echo "collecting dictionary..."
time ./make_dictionary.out ./dict.txt ~/wiki_tokens/*
echo "lemmatization..."
time python3 mylemm.py ./dict.txt lemms_dict.txt
