set terminal png
set output "output.png"
set xlabel "Sample"
set ylabel "RTT (ms)"
set title "RTT for www.google.com"
set xrange [0:10000]
set yrange [0:200]
plot "awk_output.txt" using 1 title 'Raw RTT' with lines lt rgb "red", \
  "rtt_output.txt" using 1 title 'Estimated RTT' with lines lt rgb "black"
