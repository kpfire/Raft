#include "Raft.h"

Raft::Raft(int num_servers, std::mutex* outputLock): num_servers(num_servers), outputLock(outputLock) {
    // start the specified amount of servers(threads)
    for (int i=0; i<num_servers; i++) {
        Server* svr = new Server(i, this);
        servers.push_back(svr);
    }

    for (int i=0; i<num_servers; i++) {
        handles.push_back(std::thread(&Server::eventLoop, servers[i]));
    }

    srand(time(NULL));
}

void Raft::crashServer(int serverId) {
    assert(serverId < num_servers);

    servers[serverId]->crash();
}

void Raft::restartServer(int serverId) {
    assert(serverId < num_servers);

    servers[serverId]->restart();
}

void Raft::partition(vector<vector<int>> partitions) {
    for (int i=0; i<partitions.size(); i++) {
        for (int server: partitions[i]) {
            serverPartition[server] = i;
        }
    }
}

bool Raft::belongToSamePartition(int server1, int server2) {
    return serverPartition[server1] == serverPartition[server2];
}

void Raft::setDropoutProbability(double p){
    dropoutProbability = p;
}

bool Raft::dropoutHappens() {
    double r = (rand() % 100) / 100.;
    //syncCout(to_string(r) + ", " + to_string(dropoutProbability));
    return r < dropoutProbability;
}

ClientRequestResponse Raft::clientRequestRPC(int serverId, string stateMachineCommand) {
    if (dropoutHappens()) {
        //syncCout("Dropout clientRequest to " + to_string(serverId));
        ClientRequestResponse response;
        response.responded = false;
        return response;
    }

    assert(serverId < num_servers);
    vector<string> parts;
    split1(stateMachineCommand, parts);
    //cout << "stateMachineCommand=" << stateMachineCommand << endl;
    ClientRequest request;
    request.key = parts[0];
    if (parts.size() > 1) {
        request.valueDelta = stoi(parts[1]);
    } else {
        request.valueDelta = 0;
    }

    std::promise<ClientRequestResponse> p;
    auto f = p.get_future();
    std::thread t(&Server::clientRequest, servers[serverId], request, std::move(p));
    t.join();

    if (dropoutHappens()) {
        //syncCout("Dropout clientRequest response from " + to_string(serverId));
        ClientRequestResponse response;
        response.responded = false;
        return response;
    }

    return f.get();
}

void Raft::syncCout(string msg) {
    outputLock->lock();
    cout << msg << endl;
    outputLock->unlock();
}