CC = gcc

all : hessian 2dconv nonmax

# Main target
hessian : hessian.c
	$(CC) -o hessian hessian.c -O1

2dconv : 2dconv.c
	$(CC) -o 2dconv 2dconv.c -O1

2dconv : nonmax.c
	$(CC) -o nonmax nonmax.c -O1

# To remove generated files
clean:
	rm -f hessian 2dconv nonmax
