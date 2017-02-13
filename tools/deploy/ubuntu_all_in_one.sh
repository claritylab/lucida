#!/bin/bash
RED='\033[0;31m'
DEFAULTCOLOR='\033[0m'

# Install Linux basic dependencies
sh install_docker_linux.sh

# Prepare directories
sudo mkdir -p /data/db
sudo chown -R $USER /data

echo -e "\n${RED}Docker installed. Please log out and back in for changed groups to take effect. Thank you.${DEFAULTCOLOR}"

# Setup Kubernetes cluster
sudo sh cluster_up_linux.sh

# Wait for cluster to be online
echo -e "\n${RED}Cluster setup succeeded, now waiting for it to boot up. You will see connection errors during this time - this is a result of polling, normal behavior and should not last longer than 3 minutes.${DEFAULTCOLOR}"
while ! sudo kubectl get pod; do
sleep 1
done
echo "\n${RED}Kubernetes cluster online.${DEFAULTCOLOR}"

# Start Lucida services
sudo sh start_services.sh

# Monitor containers coming up. This will take a while.
echo -e "\n${RED}Lucida services initiated. It will take a while (up to 15 minutes) until all services are running. You can then open http://localhost:30000/ and start using Lucida.${DEFAULTCOLOR}"
sudo watch kubectl get pod
