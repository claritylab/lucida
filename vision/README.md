### Image Matching (IMM)

Functions included:  
image matching (described in this README), person detector, text extraction

Dependencies:
- libboost-all-dev
- opencv (can use ./get-opencv.sh to install)
- libprotobuf (2.5.0)

Python dependencies:  
see ./start-img-server.py

./detect --help  
For example:  
./detect --match matching/landmarks/query/query.jpg --database matching/landmarks/db

Use ./make-db.py db_name to store image database as descriptors in protobuf
format (faster).  
Example: ./make-db.py landmarks  
Then Re-run the application normally  
