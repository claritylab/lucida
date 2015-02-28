## Sirius-web

Included is a web-frontend for the [sirius-application](../sirius-application). 

### Prerequisites

Run `get-web-deps.sh` to install the required dependencies.

### Interacting with Sirius

Use the scripts included in the `demo/` directory to make requests to Sirius
servers from a CLI. Change the IP addresses and ports at the top of the
scripts. These are similar to the test scripts in
`sirius-application/run-scripts`.

Use `$ ./web-sirius.py <ip> <port>` to run a browser based frontend. Also make
sure to change Sirius' server addresses and ports at the top of the script.

Have fun!
