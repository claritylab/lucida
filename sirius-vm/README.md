# Sirius Vagrant VM
This configuration creates a basic virtual machine to test or to
contribute to **Sirius**.

### Dependencies
* [Virtual Box](https://www.virtualbox.org/wiki/Downloads)
* [Vagrant](http://www.vagrantup.com/downloads.html)

### Initial Setup
* Download or checkout the Sirius git repository.
* Navigate in your commandline / terminal into the *sirius-vm*
  directory.
* adapt the Vagrantfile to your environment
    * to compile you need more than 1024 MB
	* to run the ASR with Kaldi you need 2048 MB
	* to run ASR and QA you need 3072 MB or more
	* to let the machine respond when the applications are running you
      need 2 CPU cores
* To create the virtual machine, just execute the command `vagrant
  up`. This will start the installation process of the virtual machine
  and compile the sirius-applications. Afterwards it will show up in
  Virtual Box.

### SSH Connection
You can connect to the vagrant machine with the command `vagrant ssh`,
right out of this directory.

Inside the box you have prepare the servers like described on the
website: http://sirius.clarity-lab.org/index.html%3Fp=9.html

(Hint: `wiki_indri_index.tar.gz` is part of the
`sirius-application.tar.gz` that is part of the `sirius-1.0.tar.gz`
and it's 11 GB in size)

There you can start the servers and then access them from outside the
box.

Since vagrant configures the public network on the interface eth1, you
can get the public ip addresse with this command:

```
IP=$(ip addr show eth1|grep "inet[^6]"|awk '{print $2}' | sed 's#/.*##')
echo IP=$IP
```

Then start one or all of the servers (image matching is untested):
```
./start-asr-server.sh kaldi $IP
./start-qa-server.sh $IP
./start-imm-server.sh $IP
```

Then from outside the VM you can start the demo applications:

```
export ASR=http://${IP}:8081
export QA=http://${IP}:8080
export IMM=http://${IP}:8082

./sirius-asr-test.sh ../inputs/questions/what.is.the.speed.of.light.wav
./sirius-qa-test.sh "what is the speed of light"
./sirius-asr-qa-test.sh ../inputs/questions/what.is.the.speed.of.light.wav
./sirius-imm-test.sh ../image-matching/matching/landmarks/query/query.jpg
```

### Contribute
Feel free to change or extend this basic configuration.
