#include "Server.h"

void Server::onServerStart() {
    online = true;
    // reset volatile variables because they are supposed to be lost
    commitIndex = 0;
    lastApplied = 0;
    if (nextIndex.size() > 0) {
        for (int i=0; i<nextIndex.size(); i++) nextIndex[i] = 0;
    } else {
        while (nextIndex.size() < raft->num_servers) nextIndex.push_back(0);
    }
    cout << "Server " << serverId << " is online" << endl;
}

void Server::crash() {
    online = false;
}

void Server::restart() {
    onServerStart();
}

void Server::eventLoop() {
    // Eventually check for election timeout here
    while (true) {
        if (online) {
            cout << "Server " << this->serverId << " is running..." << endl;
            for (int i=0; i<raft->num_servers; i++) {
                if (i == serverId) continue;
                // key for locks is always the same
                if (i < serverId) pair<int, int> lock_key {i, serverId};
                else pair<int, int> lock_key {serverId, i};
                // Key for channels needs to stay ordered
                pair<int, int> key {i, serverId};
                raft->locks[lock_key].lock();
                string response;
                // check incoming messages from server i
                while (raft->channels[key].size() > 0) {
                    response = handleMessage(i, raft->channels[key].front());
                    raft->channels[key].pop();
                    // put the response onto the outgoing channel
                    pair<int, int> response_key {serverId, i};
                    raft->channels[response_key].push(response);
                }
                raft->locks[lock_key].unlock();
            }
        }
        sleep(100);
    }
}

string Server::handleMessage(int fromServerId, string message) {
    cout << fromServerId << " -> " << serverId << ": " << message << endl;

    if (message.rfind("RequestVote ", 0) == 0) {
        vector<string> parts;
        split1(message, parts);
        assert(parts.size() == 5);
        int term = stoi(parts[1]);
        int candidateId = stoi(parts[2]);
        int lastLogIndex = stoi(parts[3]);
        int lastLogTerm = stoi(parts[4]);
        // vote logic goes here...
        string response;

        return response;
    } else if (message.rfind("AppendEntries ", 0) == 0) {
        vector<string> parts;
        split1(message, parts);
        assert(parts.size() == 7);
        int term = stoi(parts[1]);
        int leaderId = stoi(parts[2]);
        int prevLogIndex = stoi(parts[3]);
        int prevLogTerm = stoi(parts[4]);
        string entry = parts[5]; // we can only store one log entry at a time. 
        int leaderCommit = stoi(parts[6]);
        // append entry logic goes here...
        string response;

        return response;
    } else if (message.rfind("RequestVoteResponse ", 0) == 0) {
        vector<string> parts;
        split1(message, parts);
    } else if (message.rfind("AppendEntriesResponse ", 0) == 0) {
        vector<string> parts;
        split1(message, parts);
    } else {
        assert(false);
    }
}