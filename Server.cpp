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
            
            // if a server wants to start a vote, it should create separate threads for each OTHER server
            // and call "vote" for each server.
        }
        sleep(1);
    }
}

// the caller of all below methods should invoke these rpc calls in a separate thread
// see Raft::clientRequest for an example

void Server::append(AppendEntries appendEntries, std::promise<AppendEntriesResponse> && p) {
    AppendEntriesResponse response;
    response.success = true;
    response.term = 0;
    p.set_value(response);
}

void Server::vote(RequestVote requestVote, std::promise<RequestVoteResponse> && p) {
    RequestVoteResponse response;
    response.voteGranted = true;
    response.term = 0;
    p.set_value(response);
}

void Server::clientRequest(ClientRequest clientRequest, std::promise<ClientRequestResponse> && p) {
    // if this is not the leader, reject it and tell who the leader it
    // otherwise handle the message in a blocking manner (add to local log, send out replicate message to
    // other servers, and monitor incoming channels from other servers to see if it is done)
    ClientRequestResponse response;
    response.succeed = true;
    response.message = "stored";
    p.set_value(response);

    //The leader needs to replicate the message to other servers. So it should create separate threads for each OTHER server
    //and call "append" for each server
}