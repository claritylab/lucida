default="-O2 -msse"
sse="-O2 -ftree-vectorize -msse2 -msse3 -msse4 -ffast-math"
avx='-O2 -ftree-vectorize -mavx -mavx2 -mfma -ffast-math'
# avx2='-O2 -mavx2'
# native='-O2 -march=native'

# change here
from=$sse
to=$avx

find . -name "Makefile" -exec sed -i -e "s/$from/$to/" {} \;

# opencv
if [ "$to" == "-O2 -msse" ];then
    find . -name "*.cpp" -exec sed -i -e "s/cvUseOptimized(1)/cvUseOptimized(0)/" {} \;
else
    find . -name "*.cpp" -exec sed -i -e "s/cvUseOptimized(0)/cvUseOptimized(1)/" {} \;
fi

if [ "$to" == "-O2 -ftree-vectorize -mavx -mavx2 -mfma3 -mfma -ffast-math" ]; then
    find . -name "Makefile" -exec sed -i -e "s/opencv-multi\/sse/opencv-multi\/avx/" {} \;
else
    find . -name "Makefile" -exec sed -i -e "s/opencv-multi\/avx/opencv-multi\/sse/" {} \;
fi

#dnn
if [ "$to" == "-O2 -ftree-vectorize -mavx -mavx2 -mfma -ffast-math" ]; then
    find . -name "Makefile" -exec sed -i -e 's/libopenblas-.*/libopenblas-avx.a\ \\/' {} \;
elif [ "$to" == "-O2 -ftree-vectorize -msse2 -msse3 -msse4 -ffast-math" ]; then
    find . -name "Makefile" -exec sed -i -e 's/libopenblas-.*/libopenblas-sse.a\ \\/' {} \;
else
    find . -name "Makefile" -exec sed -i -e 's/libopenblas-.*/libopenblas-baseline.a\ \\/' {} \;
fi
