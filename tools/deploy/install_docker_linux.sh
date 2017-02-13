#!/bin/bash

# Install dependencies
sudo apt install -y curl linux-image-extra-$(uname -r) linux-image-extra-virtual apt-transport-https ca-certificates

# Import keys
curl -fsSL https://yum.dockerproject.org/gpg | sudo apt-key add -

# Add Docker repository
sudo add-apt-repository \
       "deb https://apt.dockerproject.org/repo/ \
       ubuntu-$(lsb_release -cs) \
       main"

# Install Docker
sudo apt update
sudo apt install -y docker-engine

# Add current user to docker group to allow executing docker commands with user privileges
sudo groupadd docker
sudo usermod -aG docker $USER

# On 16.04 we need to activate Docker via systemd
if [[ `lsb_release -rs` == "16.04" ]]
then
sudo systemctl start docker
sudo systemctl enable docker
fi

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/download/1.10.1/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
