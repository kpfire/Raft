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


#ifndef RAFT
#define RAFT

#include "Server.h"
#include "utilities.h"

// Raft is represents the overall infrastracture of the project. It manages servers and threads.
// It offers communication channels between threads.
class Raft {
    private:
    // output lock
    std::mutex outputLock;
    // Threads for each server. Currently not being used.
    vector<std::thread> handles;
    
    public:
    // List of ervers
    vector<Server*> servers;

    int num_servers;
    
    Raft(int totalServers);

    ~Raft() { }

    void crashServer(int serverId);

    void restartServer(int serverId);

    // RPC functions run on the caller
    ClientRequestResponse clientRequestRPC(int requestedServer, string stationMachineCommand) ;

    // Synchronous cout
    void syncCout(string);

};

#endif