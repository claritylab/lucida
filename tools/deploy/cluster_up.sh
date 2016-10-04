# Set up a local cluster via docker on a single machine.
export K8S_VERSION=v1.3.0
export ARCH=amd64
docker run -d \
--volume=/:/rootfs:ro \
--volume=/sys:/sys:rw \
--volume=/var/lib/docker/:/var/lib/docker:rw \
--volume=/var/lib/kubelet/:/var/lib/kubelet:rw \
--volume=/var/run:/var/run:rw \
--net=host \
--pid=host \
--privileged \
gcr.io/google_containers/hyperkube-${ARCH}:${K8S_VERSION} \
/hyperkube kubelet \
    --containerized \
    --hostname-override=127.0.0.1 \
    --api-servers=http://localhost:8080 \
    --config=/etc/kubernetes/manifests \
    --cluster-dns=10.0.0.10 \
    --cluster-domain=cluster.local \
    --allow-privileged --v=2
curl -sSL "http://storage.googleapis.com/kubernetes-release/release/${K8S_VERSION}/bin/linux/amd64/kubectl" > /usr/bin/kubectl
chmod +x /usr/bin/kubectl
kubectl config set-cluster test-doc --server=http://localhost:8080
kubectl config set-context test-doc --cluster=test-doc
kubectl config use-context test-doc
echo "Exiting... Wait until 'kubectl get pod' does not give error. Fix permission errors if exist. Then run 'start_services.sh'."
echo "Meanwhile, you can 'watch docker ps' to see how Kubernetes sets up the cluster via Docker containers."
