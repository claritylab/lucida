find . -name "Makefile" -exec sed -i '/CXX_FLAGS+=-DPROFILING/d' "{}" \;
