CC = g++
CFLAGS = -g -Wall
DEPS = sha1.h
OBJ = client.o sha1.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto
 
clean:
	rm -rf *o client
