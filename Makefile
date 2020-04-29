all: clean build

clean: 
	rm -f ./raft.o

build: main.cpp Server.cpp Server.h Raft.cpp Raft.h utilities.h AppendEntries.h ClientRequest.h RequestVote.h
	g++ -pthread -o raft.o main.cpp Server.cpp Server.h Raft.cpp Raft.h utilities.h AppendEntries.h ClientRequest.h RequestVote.h

debug: main.cpp Server.cpp Server.h Raft.cpp main.cpp utilities.h
	g++ -g -pthread -o raft.o main.cpp Server.cpp Server.h Raft.cpp Raft.h utilities.h AppendEntries.h ClientRequest.h RequestVote.h

test: clean build
	echo "testing given examples"
	./raft.o < test/test_example.txt

