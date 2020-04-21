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
    // Servers and their corresponding threads.
    vector<Server> servers;
    vector<std::thread> handles;
    
    public:
    int num_servers;
    
    Raft(int totalServers);

    ~Raft() {
        
    }

    void crashServer(int serverId);

    void restartServer(int serverId);

    ClientRequestResponse clientRequest(int requestedServer, string stationMachineCommand) ;

};

#endif