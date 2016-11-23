#!/bin/bash
echo "Assume that the local cluster is set up."

kubectl cluster-info

kubectl describe node

filelist=$(ls ./*-controller.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl delete -f "${d}"
    done
fi

filelist=$(ls ./*-service.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl delete -f "${d}"
    done
fi

filelist=$(ls ./*-service.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl create -f "${d}" --validate=false
    done
fi

filelist=$(ls ./*-controller.yaml)
if [ "${filelist}" != "" ]; then
    for d in ${filelist}; do
        echo "***** ${d}"
        kubectl create -f "${d}" --validate=false
    done
fi

kubectl get services

kubectl get pod

echo "Run 'watch kubectl get pod' to monitor the pods. Open your browser after all pods have the status 'running'."
