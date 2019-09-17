#!/bin/bash
echo "hello world"
url=www.government.kz
hours=6
sleeptime=5 # minutes
rm traceroute_data.txt
for ((i=0;i<60/sleeptime*hours;i++))
do
echo "doing traceroute $((i + 1))"
traceroute $url >> traceroute_data.txt
sleep $((sleeptime*60))
done

rm awkoutput.txt
awk -f read.awk traceroute_data.txt >> awk_output.txt
gnuplot -p -e 'plot "awk_output.txt" u 1:(1) smooth cumulative w lp'

# -rw-r--r--
# first char - d = directory, - = file
# -(user)(group)(others)
# read-write-execute
# 111 = 7, 110 = 6
# chmod 774 <file.sh> = -rwx-rwx-r--
