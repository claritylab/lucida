sed -i '/__itt/i #ifdef PROFILING' $1
sed -i '/__itt/a #endif' $1
