# Parser

The parser program in `lucida/parser` belongs to a collection of services. It
can create an Indri repository based on the input urls and files, which can be
used in the question-answering service by modifying INDRI_INDEX in
lucida/qa/common/qa-runtime-config.inc.

## Basic Setup

1) Compile the server and the client: `./compile-parser-client-and-server.sh`

2) Start the server: `./start-parser-server.sh (PORT)`

3) Start the client: `./start-parser-client.sh (PORT) (pathToIndriRepository) (numberOfUrls) (numberOfFiles) (listOfUrls) (listOfFilePaths)`

## For example,

1) ./compile-parser-client-and-server.sh

2) ./start-parser-server.sh 8080

3) ./start-parser-client.sh 8080 repo/deep/deeper 2 3 https://en.wikipedia.org/wiki/Aloe http://www.google.com/ path/to/file1.txt path/to/file2.pdf path/to/file3.html

4) Once the database is created, edit the last line in the question-answer
system `qa/common/qa-runtime-config.inc` to point to the newly created database
(full path required). Now you can query the new data!

## Note: 

1) All parameters are required: 

(PORT): port number where the parser server is running at.

(pathToIndriRepository): location where the Indri repository will be stored. If
the folder does not exist, the parser will create it; if the folder already
exists, the files in the folder will be deleted.

(numberOfUrls): number of urls to be specified. Must be non-negative.

(numberOfFiles): number of files to be specified. Must be non-negative.

(listOfUrls): space-separated list of valid urls. If there is error fetching the
url, parsing will fail. The length of the list must match (numberOfUrls).

(listOfFilePaths): space-separated list of valid file paths. Files that do not
exist will be ignored. The length of the list must match (numberOfFiles).

2) The parser which uses Indri
(http://sourceforge.net/p/lemur/wiki/Indexer%20File%20Formats/) can parse files
of the following types:

* html (HTML formatted data, one document per file)
* xml (XML formatted data, one document per file - same as html, but without
  link processing)
* trecweb (TREC web collections, such as WT10G or GOV2, with many documents per
  file)
* trectext (TREC newswire collections, such as AP89, with many documents per
  file)
* warc WARC (Web ARChive) format, such as can be output by the heritrix
  webcrawler.
* warcchar WARC (Web ARChive) format, such as can be output by the heritrix
  webcrawler. Tokenizes individual characters, enabling indexing of unsgemented
  text.
* mbox (Unix mailbox files)
* doc (Microsoft Word documents - Windows only, requires Microsoft Office)
* ppt (Microsoft PowerPoint documents - Windows only, requires Microsoft Office)
* pdf (Adobe PDF)
* txt (Text documents)

3) Passing in an html or xml file is not recommended, because tags would also be
treated as part of the file content and be parsed. Instead, please specify the
web pages as urls in the url list.  

Last Modified: 01/05/16
