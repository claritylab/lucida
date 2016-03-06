# Resize images to fit sizes specified by CNN using imagemagick
# args: ./resize.sh <image> <size>
# ./resize.sh test.jpg 227
#
# 1 liner for a directory:
# for i in test-images/*.jpg; do ./resize.sh $i 227; done
#
# APP:  C x W x H
# IMC:  3x227x227
# FACE: 3x152x152
# DIG:  1x28x28

INPUT=$1
SIZE=$2
DIR=$(dirname $INPUT)
IN=${INPUT##*/}
IN=${IN%.jpg}
OUTPUT=$IN-$SIZE.jpg

convert $INPUT -resize ${SIZE}x${SIZE}^ -gravity center -crop ${SIZE}x${SIZE}+0+0 $DIR/$OUTPUT
