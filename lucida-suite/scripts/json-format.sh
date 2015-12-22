file=$1

# test if first json bracket there
grep -q "\[" $file
out=$?

if [ $out -eq 1 ];
then
    sed -i -e '1s/^/\[/' $file
    sed -i -e 's/}/},/' $file
    sed -i -e '$s/$/\]/' $file
    sed -i -e '$s/},]/}\]/' $file
fi
