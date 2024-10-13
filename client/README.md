## Client
- We first create user using credentials and then login.
- Client is more or less the front end of this project where it simply enters the command and wait for tracker to complete it and gets a reply using socket programming.
- We also apply fault tolerance, i.e., While transfering Info, we see basic things like whether the user exists or logged in, whether the group exists or not, whether the file exists or not, whether the user is a member of group or someone contains file as a seeder.

- Upload and Download functionality works on Client Side.
- We upload data in chunks of size 512 kb and calculate SHA1 hash of these chunks before sending for end to end encryption. 
- Piece Selection Algorithm: We took data in chunks, further break that chunk into 16kb, so that it will fit into an array of size 16 kb or 32 kb, even it 32 bit system, read byte by byte and write it in another file, join all the chunks and download it.
- We also find SHA1 Hash of each downloaded chunk and compare it with uploaded chunk. If all the chunks are perfect, then our download is perfect.

- Client works both as a server as well as client.