#!/bin/bash
num_samples=10000
alpha=0.8
url=google.com
rm -rf raw_data.txt awk_output.txt rtt_output.txt
ping -c $num_samples $url >> raw_data.txt
awk -f getdata.awk raw_data.txt >> awk_output.txt
awk -f getrtt.awk raw_data.txt >> rtt_output.txt
gnuplot plot.p
display output.png
