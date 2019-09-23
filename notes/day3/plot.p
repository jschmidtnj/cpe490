set terminal png
set output "output.png"
set yrange [0:1]
set xlabel "Delay (ms)"
set ylabel "Probability"
set title "CDF for www.government.kz (6hrs every 5 mins)"
plot "awk_output.txt" using 1:(.01) smooth cumul
