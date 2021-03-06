#include "Raft.h"

Raft::Raft(int num_servers, int t_type, std::mutex* outputLock): num_servers(num_servers), outputLock(outputLock), timeoutType(t_type) {
    // start the specified amount of servers(threads)
    for (int i=0; i<num_servers; i++) {
        Server* svr = new Server(i, this);
        servers[i] = svr;
    }

    for (int i=0; i<num_servers; i++) {
        handles.push_back(std::thread(&Server::eventLoop, servers[i]));
    }

    srand(time(NULL));
}

void Raft::callConfigChange(int serverId, string change_to) {
    stringstream ss(change_to);
    vector<int> awoken_servers;
    while(ss.good()) {
        string substr;
        getline(ss, substr, ',');
        int new_server_id = stoi(substr);
        if (servers.find(new_server_id) == servers.end()) { // Start new servers
            Server* svr = new Server(new_server_id, this);
            servers[new_server_id] = svr;
            handles.push_back(std::thread(&Server::eventLoop, servers[new_server_id]));
            ++num_servers;
            awoken_servers.push_back(new_server_id);
        }
        else { // Wake up crashed servers
            if (!servers[new_server_id]->online) {
                servers[new_server_id]->restart();
                awoken_servers.push_back(new_server_id);
            }
        }
    }
    bool succeed = servers[serverId]->change_config(change_to);
    if (!succeed) {
        for (int k = 0; k < awoken_servers.size(); k++) {
            servers[awoken_servers[k]]->crash();
        }
    }
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
    // an RPC involves an incoming message and outgoing message (send out response). Dropout could happen in either (or both) stage.
    // if the dropout rate of a single message is p, then the probability than at least one message
    // is dropped is 1-(1-p)^2
    double dropoutProbabilityInTwoWayCommunication = 1.0-(1.0-p)*(1.0-p);
    // Be 99.99% confident that communication between 2 online servers will eventually succeed if we try retry_times times
    retry_times = max(1, (int)ceil(log(0.0001) / log(dropoutProbabilityInTwoWayCommunication)));
    //syncCout("Set retry_times=" + to_string(retry_times));
}

bool Raft::dropoutHappens() {
    double r = (rand() % 100) / 100.;
    //syncCout(to_string(r) + ", " + to_string(dropoutProbability));
    return r < dropoutProbability;
}

ClientRequestResponse Raft::clientRequestRPC(int serverId, string stateMachineCommand) {
    // assume no dropout when handling client request
    /*if (dropoutHappens()) {
        //syncCout("Dropout clientRequest to " + to_string(serverId));
        ClientRequestResponse response;
        response.responded = false;
        return response;
    }*/

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

    /*if (dropoutHappens()) {
        //syncCout("Dropout clientRequest response from " + to_string(serverId));
        ClientRequestResponse response;
        response.responded = false;
        return response;
    }*/

    return f.get();
}

void Raft::syncCout(string msg) {
    outputLock->lock();
    cout << msg << endl;
    outputLock->unlock();
}