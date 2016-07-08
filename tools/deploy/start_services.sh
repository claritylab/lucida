# Before going on, make sure the cluster is set up.
kubectl cluster-info

kubectl describe node

filelist=$(ls *-controller.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl delete -f ${d}
    done
fi

filelist=$(ls *-service.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl delete -f ${d}
    done
fi

filelist=$(ls *-service.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl create -f ${d}
    done
fi

filelist=$(ls *-controller.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl create -f ${d}
    done
fi

kubectl get services

watch "kubectl get pod"
