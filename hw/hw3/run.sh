#!/bin/bash
num_samples=10000
alpha=0.8
ping -c $num_samples google.com >> raw_data.txt
awk -f process.awk raw_data.txt >> awk_output.txt
