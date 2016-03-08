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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

#define MAX_BUF_SIZE 65535
#define READ_MAX_BUF_SIZE 512

int SOCKET_txsize(int socket, int len) {
  return (write(socket, (void *)&len, sizeof(int)));
}

int SOCKET_send(int socket, char *data, int size, bool debug) {
  int total = 0;
  while (total < size) {
    int sent = send(socket, data + total, size - total, MSG_NOSIGNAL);
    if (sent <= 0) break;
    total += sent;
    if (debug)
      printf("Sent %d bytes of %d toal via socket %d\n", total, size, socket);
  }
  return total;
}

int SOCKET_rxsize(int socket) {
  int size = 0;
  int stat = read(socket, &size, sizeof(int));
  return (stat < 0) ? -1 : size;
}

int SOCKET_receive(int socket, char *data, int size, bool debug) {
  int rcvd = 0;
  while (rcvd < size) {
    int got = recv(socket, data + rcvd, size - rcvd, 0);
    if (got <= 0) break;
    rcvd += got;
    if (debug)
      printf("Received %d bytes of %d total via socket %d\n", rcvd, size,
             socket);
  }
  return rcvd;
}

int CLIENT_init(char *hostname, int portno, bool debug) {
  int sockfd;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("ERROR opening socket\n");
    exit(0);
  }
  server = gethostbyname(hostname);
  if (server == NULL) {
    printf("ERROR, no such host\n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("ERROR connecting\n");
    return -1;
  } else {
    if (debug) printf("Connected to %s:%d\n", hostname, portno);
    return sockfd;
  }
}

int SERVER_init(int portno) {
  int sockfd, newsockfd;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("ERROR opening socket");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("ERROR on binding\n");
    exit(0);
  }
  return sockfd;
}

int SOCKET_close(int socket, bool debug) {
  if (debug) printf("Closing socket %d\n", socket);
  close(socket);
}
