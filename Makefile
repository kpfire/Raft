all: clean build

clean: 
	rm -f ./raft.o

build: main.cpp Server.cpp Raft.cpp
	g++ -pthread -o raft.o main.cpp Server.cpp Raft.cpp 

debug: main.cpp Server.cpp Raft.cpp
	g++ -g -pthread -o raft.o main.cpp Server.cpp Raft.cpp

test: clean build
	echo "testing given example"
	./raft.o < test/test_example.txt