#include "Raft.h"

Raft::Raft(int num_servers): CommunicationChannels(num_servers) {
    // start the specified amount of servers(threads)
    for (int i=0; i<num_servers; i++) {
        Server svr(i, this);
        servers.push_back(svr);
    }

    for (int i=0; i<num_servers; i++) {
        handles.push_back(std::thread(&Server::eventLoop, &servers[i]));
    }
}

void Raft::crashServer(int serverId) {
    if (serverId >= servers.size()) {
        cout << "Invalid server id";
        return;
    }

    servers[serverId].crash();
}

void Raft::restartServer(int serverId) {
    if (serverId >= servers.size()) {
        cout << "Invalid server id";
        return;
    }

    servers[serverId].restart();
}