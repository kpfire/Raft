all: clean build

clean: 
	rm -f ./raft.o

build: main.cpp Server.cpp Raft.cpp
	g++ -pthread -o raft.o main.cpp Server.cpp Raft.cpp 

debug: main.cpp Server.cpp Server.h Raft.cpp main.cpp utilities.h
	g++ -g -pthread -o raft.o main.cpp Server.cpp Raft.cpp

test: clean build
	echo "testing given examples"
	./raft.o < test/test_example.txt

