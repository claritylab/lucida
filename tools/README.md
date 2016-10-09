# Tools

This directory contains common tools that are used by Lucida micro-services, 
so please install all of them before compiling any micro-service.

## Install

Type `make` to install all dependencies necessary for Lucida in the following order:

- `apt_deps.sh`: various packages installed using `apt-get`. 

- `install_python.sh`: Python 2.7.9 virtual environment
and various packages installed via `pip`.

- `install_java.sh`: Java 8

- `install_opencv.sh`: [OpenCV](http://opencv.org/)

- `install_thrift.sh`: [Apache Thrift 0.9.3](https://thrift.apache.org/)

- `install_fbthrift.sh`: [Facebook Thrift](https://github.com/facebook/fbthrift)

- `install_mongodb.sh`: [MongoDB](https://www.mongodb.com/)
and [C++ legacy driver](https://github.com/mongodb/mongo-cxx-driver/tree/legacy)

Other scripts:

- `download_wiki_index.sh`: if you want to use Wikipedia as an additional data source for [OpenEphyra](../lucida/questionanswering/OpenEphyra), please run `./download_wiki_index.sh` and export `wiki_indri_index`. When deploying, you need to mount the Wikipedia database to the Kubernetes cluster, so please move the database to the host volume directory where other user databases are stored.
(refer to [`deploy/qa-controller.yaml`](deploy/qa-controller.yaml) for details).

- `start_all.sh`: if you want to use a subset of services, please modify this file which is called
by [`the top-level Makefile`](../Makefile) to start all the services.

## Notes

1. This setup has been tested for Ubuntu 14.04 (64 bit), gcc 4.8, and Python 2.7.9, but
you are welcome to improve the build system.
If you use Ubuntu 16.04, modify `python_requirements.txt` so that `pyOpenSSL==0.14`,
and be aware that you may not be able to compile OpenCV.
Make sure to use `gcc 4.8.4`, because MongoDB C++ driver fails to compile on newer version of gcc.

2. Each script performs a simple check on whether the package is
installed. If for some reason the installation failed, or the simple check
is not sufficient and you want to force reinstallation,
please either ```sudo ./install_xxx.sh```, 
or open the script and run the commands manually to make sure each command succeeds.

3. Both Apache Thrift and Facebook are necessary, and you must install Apache Thrift first,
and then install Facebook Thrift. The `Makefile` guarantees that, but
if for some reason you reverse the order (both compiled and installed),
simply go back to Facebook Thrift: `cd fbthrift/thrift/`
and `sudo make install` which should not take too long because it is already compiled.
