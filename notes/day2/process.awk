# awk reads data line by line, saves in $1, $2, etc.
BEGIN {
  delay
  data[3]
}

# this happens every line
{
  delay = $8
  split(delay, data, "=")
  if ($1 == 64)
    print data[2]
}

# happens when at the end of scanning through files
END {
}
