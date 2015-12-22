default="-O2 -msse"
sse3="-O2 -msse3"
sse4="-O2 -msse4"
avx='-O2 -mavx'
avx2='-O2 -mavx2'
native='-O2 -march=native'

option=$default
g++ $option -Q --help=target > default.txt
