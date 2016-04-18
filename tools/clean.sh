# Clean up script

rm -rf caffe
rm -rf opencv-*
rm -rf protobuf-*
rm -rf thrift-*

for tar in *.tar.gz;
do
  if [ -f $tar ]; then
    rm $tar
  fi
done
