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

#include "CommunicationChannels.h"

using namespace std;

#ifndef SERVER
#define SERVER

class Server {
    private:
    //Own our variables
    //Use a boolean variable to mimic server crashes. If it is false, then the server should be not responding.
    bool online;
    // Interval to sleep between checking for requests, also factors into checking the election timeout at every execution
    int interval;

    CommunicationChannels* raft;

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
    Server(int serverId, CommunicationChannels* raft):serverId(serverId), raft(raft) {
        onServerStart();
    };

    void crash();

    void restart();

    void eventLoop();

    void handleMessage(int fromServerId, string message);
};

#endif