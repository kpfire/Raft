#include "Server.h"

void Server::onServerStart() {
    online = true;
    state = Follower;
    leaderId = -1;
    interval = 1; // seconds between checking for requests
    // randomized election timeout
    timeout = 5 + rand() % 5;
    last_time = clock();
    votedFor = -1; // instead of NULL
    // reset volatile variables because they are supposed to be lost
    commitIndex = 0;
    lastApplied = -1;
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
    while (true) {
        if (online) {
            cout << "Server " << this->serverId << " is running..." << endl;
            if (state != Leader){
                // Check election timeout value
                float time_passed = (clock() - last_time)/CLOCKS_PER_SEC;
                if (time_passed > timeout && votedFor != -1) {
                    state = Candidate
                    currentTerm += 1;
                    int collected_votes = 1; // votes for self
                    // Request votes from everyone but self
                    RequestVote req = {currentTerm, serverId, commitIndex, log[commitIndex].first};
                    for (int ids = 0; ids < raft->num_servers; ids++) {
                        if (ids == serverId) continue;
                        if (requestVoteRPC(req, ids).voteGranted == true) collected_votes++;
                    }
                    int majority = (int)floor((double)(raft->num_servers)/2.) + 1;
                    // Also recheck if state was reset before winning election
                    if (collected_votes < majority || state == Follower) continue;
                    else {
                        state = Leader;
                    }
                    last_time = clock();                 
                }
            }
            if (state == Leader) {
                // send out heartbeat
                for (int ids = 0; ids < raft->num_servers; ids++) {
                    if (ids == serverId) continue;
                    appendEntriesRPC(-1, ids);
                }
            }
        }
        sleep(interval);
    }
}


void Server::convertToFollowerIfNecessary(int requestTerm, int responseTerm) {
    // If RPC request or response contains term T > currentTerm: set currentTerm = T, convert to follower (§5.1)
    int maxTerm = max(requestTerm, responseTerm);
    if (maxTerm > currentTerm) {
        currentTerm = maxTerm;
        state = Follower;
        // Reset votedFor only if it's a new leader
        votedFor = -1;
    }
    // If we called this, it was from an RPC and we can reset the election timer
    last_time = clock();
}

// the caller of all below methods should invoke these rpc calls in a separate thread
// see Raft::clientRequest for an example

void Server::appendEntries(AppendEntries request, std::promise<AppendEntriesResponse> && p) {
    AppendEntriesResponse response;
    
    if (request.leaderCommit == -1) {
        // This is just an empty heartbeat
        response.success = true;
        response.term = -1;
    }
    else if (request.term < currentTerm) {
        // Reply false if term < currentTerm (§5.1)
        response.success = false;
        response.term = currentTerm;
    } else if (request.prevLogIndex >= log.size() || log[request.prevLogIndex].first != request.prevLogTerm) {
        //Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm (§5.3)
        response.success = false;
        response.term = currentTerm;
    } else {
        // If an existing entry conflicts with a new one (same index but different terms), 
        // delete the existing entry and all that follow it (§5.3)
        int logIndex = request.prevLogIndex + 1;
        if (logIndex < log.size() && log[logIndex].first != currentTerm) {
            log.erase(log.begin() + logIndex, log.end());
        }
        // Append any new entries not already in the log
        if (logIndex >= log.size()) {
            log.push_back({request.term, request.entry});
        }
        // If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
        if (request.leaderCommit > commitIndex) {
            commitIndex = min(request.leaderCommit, (int)(log.size() - 1));
        }
        response.success = true;
        response.term = currentTerm;

        //If commitIndex > lastApplied: increment lastApplied, apply log[lastApplied] to state machine (§5.3)
        while (commitIndex > lastApplied) {
            lastApplied++;
            stateMachine[log[lastApplied].second.first] += log[lastApplied].second.second;
        }
        // infered logic (not in paper)
        leaderId = request.leaderId;
    }
    p.set_value(response);

    convertToFollowerIfNecessary(request.term, response.term);
}

void Server::requestVote(RequestVote request, std::promise<RequestVoteResponse> && p) {
    RequestVoteResponse response;
    response.term = currentTerm;
    response.voteGranted = false;
    if (request.term >= currentTerm) {
        if (votedFor == -1 || votedFor == request.candidateId) {
            if (request.lastLogIndex >= commitIndex && request.lastLogTerm >= log[commitIndex].first) {
                response.voteGranted = true;
            }
        }
    }
    votedFor = request.candidateId;
    p.set_value(response);
    convertToFollowerIfNecessary(request.term, response.term);
}

void Server::clientRequest(ClientRequest request, std::promise<ClientRequestResponse> && p) {
    // if this is not the leader, reject it and tell who the leader it
    // otherwise handle the message in a blocking manner (add to local log, send out replicate message to
    // other servers, and monitor incoming channels from other servers to see if it is done)
    ClientRequestResponse response;
    if (state != Leader) {
        response.succeed = false;
        if (leaderId >= 0) {
            response.message = "Please contact server " + to_string(leaderId);
        } else {
            response.message = "Please try again later";
        }
    } else {
        if (request.valueDelta == 0) {
            // since the delta is 0, we consider it as a query instead of an update
            response.message = request.key + "=" + to_string(stateMachine[request.key]);
        } else 
        {
            // delta != 0. We consider it as an update
            // If command received from client: append entry to local log, respond after entry applied to state machine (§5.3)
            log.push_back({currentTerm, {request.key, request.valueDelta}});
            int replicateIndex = log.size() - 1;
            pthread_barrier_t mybarrier;
            // the min limit to the barrier is 1(current thread) + half of the threads that replicates to other servers
            // if num_servers = 5, then only TWO other servers needs to reply.
            pthread_barrier_init(&mybarrier, NULL, 1 + raft->num_servers / 2);
            barriers[replicateIndex] = mybarrier;
            for (int i=0; i<raft->num_servers; i++) {
                if (i==serverId) continue;
                std::thread(&Server::replicateLogEntry, this, replicateIndex, i);
            }
            pthread_barrier_wait(&barriers[replicateIndex]);
        }
        response.succeed = true;
    }
    p.set_value(response);

    //The leader needs to replicate the message to other servers. So it should create separate threads for each OTHER server
    //and call "append" for each server
}

// this models a thread running on the LEADER and it tries replicate certain log entry to one CERTAIN follower
void Server::replicateLogEntry(int replicateIndex, int replicateTo) {
    AppendEntriesResponse response = appendEntriesRPC(replicateIndex, replicateTo);
    if (response.success) return;
    // try previous entry
    int rollbackTo = replicateIndex;
    while (!response.success) {
        --rollbackTo;
        assert(rollbackTo >= 0);
        response = appendEntriesRPC(rollbackTo, replicateTo);
    } 
    // now replicate again starting from rollbackTo+1
    rollbackTo++;
    while (rollbackTo < replicateIndex) {
        response = appendEntriesRPC(replicateIndex, replicateTo);
        assert(response.success);
        rollbackTo++;
    }
    // done
    cout << "Leader " << serverId << " has replicated log entry " << replicateIndex << " to " << replicateTo << endl;
    pthread_barrier_wait(&barriers[replicateIndex]);
}

// RPC functions run on the caller
AppendEntriesResponse Server::appendEntriesRPC(int replicateIndex, int replicateTo) {
    AppendEntries request;
    // Heartbeat message?
    if (replicateIndex == -1) {
        request.leaderCommit = -1;
        request.term = currentTerm;
    }
    else {
        request.term = currentTerm;
        request.leaderId = serverId;
        request.prevLogIndex = replicateIndex - 1;
        request.prevLogTerm = log[replicateIndex - 1].first;
        request.entry = log[replicateIndex].second;
        request.leaderCommit = commitIndex;
    }
    std::promise<AppendEntriesResponse> p;
    auto f = p.get_future();
    std::thread t(&Server::appendEntries, raft->servers[replicateTo], request, std::move(p));
    t.join();
    return f.get();
}

// RPC functions run on the caller
RequestVoteResponse Server::requestVoteRPC(RequestVote request, int sendTo) {
    std::promise<RequestVoteResponse> p;
    auto f = p.get_future();
    std::thread t(&Server::requestVote, raft->servers[sendTo], request, std::move(p));
    t.join();
    return f.get();
}