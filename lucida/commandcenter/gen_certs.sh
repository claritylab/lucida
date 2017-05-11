#!/bin/bash

if [ ! -e certs/server.key ] || [ ! -e certs/server.crt ]; then

    mkdir certs
    cd certs/

    # Generate a private key
    openssl genrsa -des3 -out server.key 1024

    # Generate a CSR
    openssl req -new -key server.key -out server.csr

    # Remove passphrase from key
    cp server.key server.key.org
    openssl rsa -in server.key.org -out server.key
    rm server.key.org

    # Generate a self signed certificate
    openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
    rm server.csr

    cd ..
fi
