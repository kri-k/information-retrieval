#!/bin/bash

g++ --std=c++11 -O3 make_dictionary.cpp -o make_dictionary.out

time ./make_dictionary.out ./dict.txt ~/wiki_tokens/*

# time python3 mylemm.py ./dict.txt lemms_dict.txt
