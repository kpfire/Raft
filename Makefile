all: clean build

clean: 
	rm -f ./raft.o

build: Server.cpp Server.h Raft.cpp main.cpp utilities.h
	g++ Server.cpp Raft.cpp main.cpp -pthread -o raft.o

debug: Server.cpp Server.h Raft.cpp main.cpp utilities.h
	g++ Server.cpp Raft.cpp main.cpp -pthread -o raft.o

test: clean build
	echo "testing given examples"
	./raft.o < test/test_example.txt

