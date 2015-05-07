## Sirius-web

Included is a web-frontend for the [sirius-application](../sirius-application). 

### Prerequisites

[get-dependencies.sh](../sirius-application/get-dependencies.sh) installs the necessary prerequisites for the web frontend.

### Interacting with Sirius

Use the scripts included in the `demo/` directory to make requests to Sirius
servers from a CLI. Change the IP addresses and ports at the top of the
scripts. These are similar to the test scripts in
`sirius-application/run-scripts`.

Alternatively, use `$ ./web-sirius.py <ip> <port>` to run a browser based frontend. Also make
sure to change Sirius' server addresses and ports at the top of the script.

Have fun!
