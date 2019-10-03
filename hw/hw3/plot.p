set terminal png
set output "output.png"
// set yrange [0:1]
set xlabel "RTT"
set ylabel "estimated RTT"
set title "EWMA for RTT"
plot "awk_output.txt"
