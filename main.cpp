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

std::mutex outputLock;
Raft* raft = NULL;

int main(int argc, char * argv[]) {
    string cmd;
    while (std::getline(std::cin, cmd))
    {
        outputLock.lock();
        cout << "$> " << cmd << endl;
        outputLock.unlock();
        if (cmd.rfind("Sleep", 0) == 0) {
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            sleep(stoi(parts[1]));
        } else if (cmd.rfind("StartRaft", 0) == 0) {
            assert(raft == NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            raft = new Raft(stoi(parts[1]), &outputLock);
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
            assert(parts.size() == 3 || parts.size() == 4);
            int requestedServer = stoi(parts[1]);
            string stationMachineCommand = parts[2];
            if (parts.size() == 4) {
                // this is an update
                stationMachineCommand += " " + parts[3];
            }
            // client ask requestedServer to perform stationMachineCommand
            ClientRequestResponse response = raft->clientRequestRPC(requestedServer, stationMachineCommand);
            if (!response.responded) {
                outputLock.lock();
                cout << "The server did not respond"<< endl;
                outputLock.unlock();
            } else if (response.message.size() > 0) {
                outputLock.lock();
                cout << "Response: " << response.message << endl;
                outputLock.unlock();
            }
        } else if (cmd.rfind("Partition", 0) == 0) {
            assert(raft != NULL);
            vector<string> parts;
            split1(cmd, parts);
            assert(parts.size() == 2);
            string partitions = parts[1];
            //raft->partition(parts[1]);
        } 
        // else {
            // cout << "Invalid command" << endl;
        // }
    }
}
