BEGIN {
delay
data[3]
}

{
 delay = $8 
 split($8, data, "=")
 if ($1 == 64)
	print data[2]
}

END {
}
