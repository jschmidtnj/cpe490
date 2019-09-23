#!/bin/bash
echo "hello world"
url=www.government.kz
hours=6
sleeptime=5 # minutes
rm -rf traceroute_data.txt awk_output.txt output.png
for ((i=0;i<60/sleeptime*hours;i++))
do
echo "doing traceroute $((i + 1))"
traceroute $url >> traceroute_data.txt
sleep $((sleeptime*60))
done

awk -f read.awk traceroute_data.txt >> awk_output.txt
gnuplot plot.p
display output.png

# -rw-r--r--
# first char - d = directory, - = file
# -(user)(group)(others)
# read-write-execute
# 111 = 7, 110 = 6
# chmod 774 <file.sh> = -rwx-rwx-r--
