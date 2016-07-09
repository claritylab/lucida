# Deploy Lucida using Kubernetes

1. Run `sudo ./cluster_up.sh` to create a Kubernetes cluster on a single machine via Docker.
It assumes that Docker is already installed.
If you want to create a cluster with more than one machines,
please refer to [the official documentation](http://kubernetes.io/docs/).

2. Modify the `hostPath` fields of `mongo-controller.yaml` and `qa-controller.yaml`
to point to the directories where you want to store the data for MongoDB and OpenEphyra.
The default paths probably won't work on your machine.
Make sure you have write access to the directories you specify.
For example, modify the last section of `qa-controller.yaml` to be:

  ```
        volumes:
          - hostPath:
              path: /home/<your_username>/Documents/lucida_data_for_Kuebrnetes
            name: openephyra-persistent-storage
  ```

  If you prefer to build the Docker image from [the top level Dockerfile](../../Dockerfile)
  rather than pulling from our Dockerhub, you need to modify
  the `image` fields of all `*-controllers.yaml`s and set up a local Kubernetes container registry.

3. Run `sudo ./start_services.sh` to launch all Kubernetes services and pods.
It assumes that a local cluster is set up.
To debug, you can run `kubectl get service` to check the services,
`kubectl get pod` and `kubectl describe pod` to check the pods,
`docker ps | grep <controller_name>` followed by `docker exec -it <running_container_id> bash` to check the running containers.
Also, if MongoDB container is constantly being created without making progress, 
run `sudo netstat -tulpn | grep 27017` and kill the currently running MongoDB instance which also uses the port 27017.
This also applies to other containers whose ports are already used and thus cannot be started.

4. Open your browser and visit `http://localhost:30000`.
It may take up to several minutes for the Apache server to start working,
but if it seems to take forever for the index page to show up, please debug as described in step 3.

5. To destroy the cluster, run `docker ps`, then `stop` and `rm` all the containers related to Kubernetes.

6. If you have SSL certificates and want to set up https, please modify the following files according to their inline comments:

  ```
  web-controller-https.yaml
  asrmaster-controller-https.yaml
  ```
  
  then recreate the following service and pods by running:
  
  ```
  kubectl delete -f asrworker-controller.yaml
  kubectl delete -f asrmaster-controller.yaml
  kubectl delete -f web-controller.yaml
  kubectl delete -f web-service.yaml
  kubectl create -f web-service-https.yaml
  kubectl create -f web-controller-https.yaml
  kubectl create -f asrmaster-controller-https.yaml
  kubectl create -f asrworker-controller-https.yaml
```
