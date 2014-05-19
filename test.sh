fs="$1"

echo "$2" > /$fs/calc/arg1
echo "$4" > /$fs/calc/arg2
echo "$3" > /$fs/calc/operation

cat /$fs/calc/result
