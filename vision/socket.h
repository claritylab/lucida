#ifndef SOCKET_H
#define SOCKET_H

// returns socket to tx data
int CLIENT_init(char *hostname, int portno, bool debug);

// returns socket where to rx data
int SERVER_init(int portno);

// tx len of data
void SOCKET_txsize(int socket, int len);

// receive len of data
int SOCKET_rxsize(int socket);

// send data over socket
int SOCKET_send(int socket, char* data, int size, bool debug);

// receive data over socket
int SOCKET_receive(int socket, char* data, int size, bool debug);

// close the socket
int SOCKET_close(int socket, bool debug);

#endif
