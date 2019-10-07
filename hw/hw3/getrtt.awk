BEGIN {
delay
data[3]
RTT_old=0
alpha=0.8
}

{
 delay = $8 
 split($8, data, "=")
 if ($1 == 64)
	RTT_new=alpha*RTT_old+(1 - alpha)*data[2]
	print RTT_new
	RTT_old=RTT_new
}

END {
}
