find . -name "Makefile" -exec sed -i '1,/CXX_FLAGS/a CXX_FLAGS+=-DPROFLING' "{}" \;
