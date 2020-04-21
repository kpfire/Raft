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

#include "utilities.h"
#include "Server.h"
#include "Raft.h"
#include "ClientRequest.h"

using namespace std;

Raft* raft = NULL;

int main(int argc, char * argv[]) {
    string cmd;
    while (std::getline(std::cin, cmd))
    {
        //cout << cmd << endl;
        if (cmd.rfind("StartRaft", 0) == 0) {
            assert(raft == NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            raft = new Raft(stoi(parts[1]));
        } else if (cmd.rfind("CrashServer", 0) == 0) {
            assert(raft != NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            // stop server stoi(parts[1])
            raft->crashServer(stoi(parts[1]));
        } else if (cmd.rfind("RestartServer", 0) == 0) {
            assert(raft != NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            // start server stoi(parts[1])
            raft->restartServer(stoi(parts[1]));
        } else if (cmd.rfind("Request", 0) == 0) {
            assert(raft != NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 3);
            int requestedServer = stoi(parts[1]);
            string stationMachineCommand = parts[2];
            // client ask requestedServer to perform stationMachineCommand
            ClientRequestResponse response = raft->clientRequest(requestedServer, stationMachineCommand);
            cout << "Server " << requestedServer << " responded: " << response.message << endl;
        } 
        // else {
            // cout << "Invalid command" << endl;
        // }
    }
}
