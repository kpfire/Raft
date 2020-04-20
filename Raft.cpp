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
    assert(serverId < num_servers);

    servers[serverId].crash();
}

void Raft::restartServer(int serverId) {
    assert(serverId < num_servers);

    servers[serverId].restart();
}

string Raft::clientRequest(int serverId, string stationMachineCommand) {
    assert(serverId < num_servers);
    return servers[serverId].onClientRequest(stationMachineCommand);
}