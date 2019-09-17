# awk reads data line by line, saves in $1, $2, etc.
BEGIN {
}

# this happens every line
{
  if ($1 == 19)
    print $4
}

# happens when at the end of scanning through files
END {
}
