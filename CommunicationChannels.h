#include <vector>
#include <queue>
#include <unordered_map>
#include "utilities.h"

using namespace std;

#ifndef COMMU_CHNL
#define COMMU_CHNL

//CommunicationChannels is written as an abstract class so there is no circular dependencies between Node and Master
class CommunicationChannels {
    public:
    // locks to access communication channels
    unordered_map<pair<int, int>, std::mutex, pair_hash> locks;
    // communication channels
    unordered_map<pair<int, int>, queue<string>, pair_hash> channels;

    // total number of servers
    int num_servers;

    CommunicationChannels(int num_servers) : num_servers(num_servers) {}
};

#endif