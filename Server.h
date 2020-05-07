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
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include "utilities.h"
#include "Raft.h"
#include "AppendEntries.h"
#include "RequestVote.h"
#include "ClientRequest.h"
#include "Semaphore.h"

using namespace std;

#ifndef SERVER
#define SERVER

class Raft;

class Server {
    private:
    //Own our variables
    // lock
    std::mutex myLock;
    // Interval to sleep between checking for requests, also factors into checking the election timeout at every execution
    int interval;
    // Election timeout limit in seconds
    double timeout;
    // Time at last execution loop, used for checking timeout;
    double last_time;
    // use a map to represent the state machine
    unordered_map<string, int> stateMachine;
    // server state
    ServerState state;
    // the current id of leader
    int leaderId;
    // barriers to replicate each log entry
    unordered_map<int, Semaphore*> barriers;

    Raft* raft;

    //Persistent state on all servers
    vector<pair<int, string>> log;

    //Volatile state on all servers
    //We can reset these variables when a server restarts to mimic their volatile nature
    int configIndex;
    int currentTerm;
    int votedFor;
    int commitIndex;
    int lastApplied;

    //Volatile state on leaders
    vector<int> nextIndex;
    vector<int> matchIndex;

    void onServerStart();

    void convertToFollowerIfNecessary(int, int);

    // after leader appens one entry to its log, call this to replicate this entry to all folloers
    void replicateLogEntry(int, int);

    public:
    // this server's identifier
    int serverId;
    //Use a boolean variable to mimic server crashes. If it is false, then the server should be not responding.
    bool online;
    
    Server(int serverId, Raft* raft):serverId(serverId), raft(raft) {
        onServerStart();
    };

    void crash();

    void restart();

    void eventLoop();    

    // functions that run on the callee
    void appendEntries(AppendEntries, std::promise<AppendEntriesResponse> && p);

    void requestVote(RequestVote, std::promise<RequestVoteResponse> && p);

    void clientRequest(ClientRequest, std::promise<ClientRequestResponse> && p);

    // RPC functions run on the caller
    RequestVoteResponse requestVoteRPC(RequestVote, int);

    AppendEntriesResponse appendEntriesRPC(int, int);

    AppendEntriesResponse repeatedlyAppendEntries(int, int);

    // Config functions
    bool change_config(string config_string);
    void get_config(int c_idx, vector<vector<int>> &config_groups);
};

#endif