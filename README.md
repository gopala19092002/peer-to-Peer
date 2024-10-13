# Peer-to-Peer Group Based File Sharing System

### My code is persistent, i.e., usernames, groups and users in the group are stored in a file and we don't need to remake it.

- Don't give plain path (e.g. abc.txt), instead give it like ./abc.txt

# How to implement it
- cd tracker
-    g++ tracker.cpp -o tracker
-   ./tracker tracker_info.txt <Tracker_number>
-   e.g. (./tracker tracker_info.txt 1/2)

- cd ../client
-    make or g++ client.cpp sha1.cpp -g -Wall -o client -lssl -lcrypto
-   ./client <IP>:<PORT> tracker_info.txt
-   e.g. (./client 127.0.0.1:54002 tracker_info.txt)

### tracker_info.txt - Contains IP, port details of all the trackers


