# Clean up script

rm -rf caffe
rm -rf opencv-*
rm -rf protobuf-*
rm -rf thrift-*
rm -rf fbthrift

for tar in *.tar.gz;
do
  if [ -f $tar ]; then
    rm $tar
  fi
done
