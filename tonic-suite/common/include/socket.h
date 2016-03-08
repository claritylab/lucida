/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @author: Johann Hauswald, Yiping Kang
 * @contact: jahausw@umich.edu, ypkang@umich.edu
 */
#ifndef SOCKET_H
#define SOCKET_H

// returns socket to tx data
int CLIENT_init(char* hostname, int portno, bool debug);

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
