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
    std::mutex* outputLock;
    // Threads for each server. Currently not being used.
    vector<std::thread> handles;

    unordered_map<int, int> serverPartition;
    double dropoutProbability = .0;
    
    public:
    // List of servers
    vector<Server*> servers;
    int num_servers;
    // 0 for deterministic timeout (for testing), any other int for random timeout
    int timeoutType;

    // Assume after trying for retry_times times, communication between two ONLINE servers will eventualy succeed
    int retry_times = 1;
    
    Raft(int totalServers, int t_type, std::mutex* outputLock);

    ~Raft() { }

    void crashServer(int serverId);

    void restartServer(int serverId);

    void partition(vector<vector<int>> partitions);

    bool belongToSamePartition(int server1, int server2);

    void setDropoutProbability(double p);

    bool dropoutHappens();

    // RPC functions run on the caller
    ClientRequestResponse clientRequestRPC(int requestedServer, string stationMachineCommand) ;

    // Synchronous cout
    void syncCout(string);

};

#endif