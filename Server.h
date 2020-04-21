#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <thread> 
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <future>
#include <atomic>  
#include <climits>
#include <numeric>
#include <unistd.h>
#include <future>

#include "Raft.h"
#include "AppendEntries.h"
#include "RequestVote.h"
#include "ClientRequest.h"

using namespace std;

#ifndef SERVER
#define SERVER

class Raft;

class Server {
    private:
    //Own our variables
    //Use a boolean variable to mimic server crashes. If it is false, then the server should be not responding.
    bool online;
    // Interval to sleep between checking for requests, also factors into checking the election timeout at every execution
    int interval;

    Raft* raft;

    //Persistent state on all servers
    int currentTerm;
    int votedFor;
    vector<pair<int, pair<string, int>>> log;

    //Volatile state on all servers
    //We can reset these variables when a server restarts to mimic their volatile nature
    int commitIndex;
    int lastApplied;

    //Volatile state on leaders
    vector<int> nextIndex;
    vector<int> matchIndex;

    void onServerStart();

    public:
    int serverId;
    Server(int serverId, Raft* raft):serverId(serverId), raft(raft) {
        onServerStart();
    };

    void crash();

    void restart();

    void eventLoop();

    void append(AppendEntries, std::promise<AppendEntriesResponse> && p);

    void vote(RequestVote, std::promise<RequestVoteResponse> && p);

    void clientRequest(ClientRequest, std::promise<ClientRequestResponse> && p);
};

#endif