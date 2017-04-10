#!/bin/bash
# Set up a local cluster via docker on a single machine.
export K8S_VERSION=v1.3.0
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
curl -Lo kubectl https://storage.googleapis.com/kubernetes-release/release/${K8S_VERSION}/bin/darwin/amd64/kubectl && chmod +x kubectl && sudo mv kubectl /usr/local/bin/
minikube start
echo "Exiting... Wait until 'kubectl get pod' does not give error. Fix permission errors if exist. Then run 'start_services.sh'."
echo "Meanwhile, you can 'watch docker ps' to see how Kubernetes sets up the cluster via Docker containers."
echo "Use 'brew install watch' (without sudo) to install watch."
