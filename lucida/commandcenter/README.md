# Command Center
- Maintains a list of registered services.
- Forwards a client's request to the appropriate registered services.
- Returns requested information to the client.

## Using Lucida services
You can use the supplied Lucida services in the lucida ecosystem in conjunction
with the command center. These services (asr, qa, imm) are in each service's
`lucida/` subdirectory. The command center must be compiled before you can compile
the lucida versions of these services.

## Starting the command center:
- Start the command center
```
./ccserver <port>
```
- Run the tests. Sample test files can be found in inputs/.
```
# Test image matching, speech recognition, and question-answering
./ccclient --asr <AUDIO_FILE> --imm <IMAGE_FILE> (PORT)
# Test speech recognition, and question-answering
./ccclient --asr <AUDIO_FILE> (PORT)
# Test question-answering
./ccclient --qa <QUESTION> (PORT)
```

## Running with LucidaMobile v0.1
(DEPRECATED) The current implementation of the mobile application requires a
node.js server as an intermediary between the mobile app and the command center.
To run this node server, you must have node installed on your machine as well as
npm.
1) Download the thrift node packages in the command-center/ folder using
```
sudo npm install thrift
```
You should see a new folder called node_modules

2) Run the server using
```
node filetransfer_svc.js <fts_port> <cmdcenter_port>
```
Note that depending on how you installed node, this command could be node or
nodejs
