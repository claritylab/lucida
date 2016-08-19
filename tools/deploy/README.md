# Deploy Lucida using Kubernetes

1. Run `sudo ./cluster_up.sh` to create a Kubernetes cluster on a single machine via Docker.
  It assumes that Docker is already installed, port 8080 is not in use,
  and you have at least 16 GB of disk space for the Docker image(s) and containers.
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
  
  Also modify the number of replicas in `*-controller`s if the default parameter does not suffice.

3. If you prefer to build the Docker image from [the top level Dockerfile](../../Dockerfile)
  rather than pulling from our Dockerhub, you need to modify
  the `image` fields of all `*-controller`s and set up a local Kubernetes container registry.

4. If you have SSL certificates and want to set up https, please modify the following files according to their inline comments:

  ```
  web-controller-https.yaml
  asrmaster-controller-https.yaml
  ```
  
  , and then rename the following files:
  
  ```
  mv asrworker-controller-https.yaml asrworker-controller.yaml
  mv asrmaster-controller-https.yaml asrmaster-controller.yaml
  mv web-controller-https.yaml web-controller.yaml
  mv web-service-https.yaml web-service.yaml
  ```

5. Run `sudo ./start_services.sh` to launch all Kubernetes services and pods.
  It assumes that a local cluster is set up.
  To debug, you can run `kubectl get service` to check the services,
  `kubectl get pod` and `kubectl describe pod` to check the pods,
  `docker ps | grep <controller_name>` followed by `docker exec -it <running_container_id> bash` to check the running containers.
  For example, if you see "Internal Server Error", you should check the web container,
  and see the error logs in `/usr/local/lucida/lucida/commandcenter/apache/logs/`.
  Also, if MongoDB container is constantly being created without making progress, 
  run `sudo netstat -tulpn | grep 27017` and kill the currently running MongoDB instance which also uses the port 27017.
  This also applies to other containers whose ports are already used and thus cannot be started.

6. Open your browser and visit `http://localhost:30000` (or `https://<YOUR_DOMAIN_NAME>:30000` if you set up https in step 4).
  It may take up to several minutes for the Apache server to start working,
  but if it seems to take forever for the index page to show up, please debug as described in step 5.

7. To destroy the cluster, run `docker ps`, then `stop` and `rm` all the containers related to Kubernetes.
   The following function may be helpful if you want to stop and remove all Docker containers.

  ```
  function docker-flush(){
    dockerlist=$(docker ps -a -q)
      if [ "${dockerlist}" != "" ]; then
        for d in ${dockerlist}; do
          echo "***** ${d}"
          docker stop ${d} 2>&1 > /dev/null
          docker rm ${d} 2>&1 > /dev/null
          done
        fi
  }
  ```
