all:
	cd server ; \
	make all ; \
	cd ../test; \
	make all ; \
	cd ..;

start_server:
	@if [ "$(port)" != "" ]; then \
		cd server; \
		./template_server $(port); \
	fi

start_test: 
	@if [ "$(port)" != "" ]; then \
		cd test; \
		./template_test $(port); \
	fi

clean:
	cd server ; \
	make clean ; \
	cd ../test; \
	make clean ; \
	cd ..;

%:
	@:
