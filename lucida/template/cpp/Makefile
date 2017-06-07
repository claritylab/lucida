all:
	cd server ; \
	make all ; \
	cd ../test; \
	make all ; \
	cd ..;

start_server: 
	cd server; ./template_server 

start_test: 
	cd test; ./template_test 

clean:
	cd server ; \
	make clean ; \
	cd ../test; \
	make clean ; \
	cd ..;

%:
	@:
