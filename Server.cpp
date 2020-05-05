#include "Server.h"

void Server::onServerStart() {
    online = true;
    state = Follower;
    leaderId = -1;
    currentTerm = -1;
    interval = 1; // seconds between checking for requests
    // randomized election timeout
    //timeout = 5 + rand() % 5;
    // debug
    timeout = 2 + (3 * serverId + 1);
    last_time = time_now();
    votedFor = -1; // instead of NULL
    // reset volatile variables because they are supposed to be lost
    commitIndex = 0;
    lastApplied = -1;
    if (nextIndex.size() > 0) {
        for (int i=0; i<nextIndex.size(); i++) nextIndex[i] = 0;
    } else {
        while (nextIndex.size() < raft->num_servers) nextIndex.push_back(0);
    }
    raft->syncCout("Server " + to_string(serverId) + " is online with timeout "+ to_string(timeout));
}

void Server::crash() {
    myLock.lock();
    online = false;
    raft->syncCout("Server " + to_string(serverId) + " crashed");
    myLock.unlock();
}

void Server::restart() {
    myLock.lock();
    onServerStart();
    myLock.unlock();
}

void Server::eventLoop() {
    while (true) {
        myLock.lock();
        if (online) {
            //cout << "Server " << this->serverId << " is running..." << endl;
            if (state != Leader){
                // Check election timeout value
                double passed = time_passed(last_time);
                //raft->syncCout("Server " + to_string(serverId) + " passed " + to_string(passed) + " seconds since last reset");
                if (passed > timeout && votedFor == -1) {
                    raft->syncCout("Server " + to_string(serverId) + " initiates an election with term = " + to_string(currentTerm+1));
                    // Hold an election
                    auto election_start = time_now();
                    state = Candidate;
                    currentTerm += 1;
                    int collected_votes = 1; // votes for self
                    // Request votes from everyone but self
                    int lastLogIndex;
                    int lastTerm;
                    if (log.size() == 0) {
                        // Initial election
                        lastLogIndex = 0;
                        lastTerm = 0;
                    }
                    else {
                        lastLogIndex = log.size() - 1;
                        lastTerm = log[lastLogIndex].first;
                    }
                    RequestVote req = {currentTerm, serverId, lastLogIndex, lastTerm};
                    vector<std::future<RequestVoteResponse>> responses;
                    for (int ids = 0; ids < raft->num_servers; ids++) {
                        if (ids == serverId) continue;
                        responses.push_back( std::async(&Server::requestVoteRPC, raft->servers[ids], req, ids));
                    }
                    //use the .get() method on each future to get the response
                    int majority = (int)floor((double)(raft->num_servers)/2.) + 1;
                    bool won_election = false;
                    // Collect votes asynchronously
                    while(!won_election) {
                        if (time_passed(election_start) > timeout) {
                            raft->syncCout("Election on server " + to_string(serverId) + " timed out!");
                            break;
                        }
                        auto it = responses.begin();
                        while(it != responses.end() && !won_election) {
                            std::future<RequestVoteResponse>& f = *it;
                            if (f.wait_for(0ms) == std::future_status::ready) { // This thread is done running
                                RequestVoteResponse r = f.get();
                                if (r.responded && r.voteGranted) {
                                    raft->syncCout("Server " + to_string(serverId) + " received a vote!");
                                    collected_votes++;
                                    if (collected_votes >= majority) {
                                        won_election = true;
                                    }
                                }
                                it = responses.erase(it);
                            }
                            else ++it;
                        }
                    }
                    // Also recheck if state was reset by a new leader before winning election
                    if (won_election && state != Follower) {
                        state = Leader;
                        raft->syncCout("Server " + to_string(serverId) + " became the leader");
                    }
                    last_time = time_now();                 
                }
            }
            if (state == Leader) {
                // send out heartbeat
                for (int ids = 0; ids < raft->num_servers; ids++) {
                    if (ids == serverId) continue;
                    repeatedlyAppendEntries(-1, ids);
                }
            }            
        }
        myLock.unlock();
        sleep(interval);
    }
}


void Server::convertToFollowerIfNecessary(int requestTerm, int requestLeaderId) {
    // If RPC request or response contains term T > currentTerm: set currentTerm = T, convert to follower (§5.1)
    // However, after checking our code, there is no way to have this: response.term > currentTerm
    if (requestTerm > currentTerm) {
        //cout << "Server " << serverId << " converted" << endl;
        currentTerm = requestTerm;
        leaderId = requestLeaderId;
        state = Follower;
        // Reset votedFor only if it's a new leader
        votedFor = -1;
    }
    // If we called this, it was from an RPC and we can reset the election timer
    last_time = time_now();
}

// the caller of all below methods should invoke these rpc calls in a separate thread
// see Raft::clientRequest for an example

void Server::appendEntries(AppendEntries request, std::promise<AppendEntriesResponse> && p) {
    myLock.lock();
    AppendEntriesResponse response;
    if (!online) {
        response.responded = false;
        p.set_value(response);
        myLock.unlock();
        return;
    }

    response.responded = true;
    if (request.leaderCommit == -1) {
        // This is just an empty heartbeat
        //raft->syncCout("Server " + to_string(serverId) + " received heartbeat from Server " + to_string(request.leaderId));
        if (currentTerm == -1) {
            // We were initialized and need to know the current leader
            leaderId = request.leaderId;
            currentTerm = request.term;
        }
        response.success = true;
        response.term = -1;
    }
    else if (request.term < currentTerm) {
        // Reply false if term < currentTerm (§5.1)
        response.success = false;
        response.term = currentTerm;
    } else if (request.prevLogIndex >=0 && (request.prevLogIndex >= log.size() || log[request.prevLogIndex].first != request.prevLogTerm)) {
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
            vector<string> parts;
            if (log[lastApplied].second.find("+=") != string::npos) {
                split2(log[lastApplied].second, "+=", parts);
            } else if (log[lastApplied].second.find("-=") != string::npos) {
                split2(log[lastApplied].second, "-=", parts);
            } else {
                //config change logic here
                assert(false);
            }
            
            stateMachine[parts[0]] += stoi(parts[1]);
        }
        // infered logic (not in paper)
        leaderId = request.leaderId;
    }
    convertToFollowerIfNecessary(request.term, request.leaderId);
    p.set_value(response);
    myLock.unlock();
}

void Server::requestVote(RequestVote request, std::promise<RequestVoteResponse> && p) {
    myLock.lock();
    RequestVoteResponse response;
    if (!online) {
        response.responded = false;
        p.set_value(response);
        myLock.unlock();
        return;
    }
    response.responded = true;
    response.term = currentTerm;
    response.voteGranted = false;
    if (votedFor == -1 || votedFor == request.candidateId) {
        if (request.term > currentTerm) { // Candidate's term is more up to date
            response.voteGranted = true;
        }
        else if (request.term == currentTerm) { // Candidate's term is same but log is longer
            if (log.size() <= request.lastLogIndex) {
                response.voteGranted = true;
            }
        }
    }
    if (response.voteGranted == true) {
        votedFor = request.candidateId;
    }
    convertToFollowerIfNecessary(request.term, request.candidateId);
    p.set_value(response);
    myLock.unlock();
}

void Server::clientRequest(ClientRequest request, std::promise<ClientRequestResponse> && p) {
    // if this is not the leader, reject it and tell who the leader it
    // otherwise handle the message in a blocking manner (add to local log, send out replicate message to
    // other servers, and monitor incoming channels from other servers to see if it is done)
    //raft->syncCout("server " + to_string(serverId) + " handles request " + request.key + (request.valueDelta == 0 ? "" : "+=" + to_string(request.valueDelta)));
    myLock.lock();
    ClientRequestResponse response;
    if (!online) {
        response.responded = false;
    }
    else{
        response.responded = true;
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
            } else {
                // delta != 0. We consider it as an update
                // If command received from client: append entry to local log, respond after entry applied to state machine (§5.3)
                string operation = request.key + "+=" + to_string(request.valueDelta);
                log.push_back({currentTerm, operation});
                int replicateIndex = log.size() - 1;
                // the min limit to the barrier is 1(current thread) + half of the threads that replicates to other servers
                // if num_servers = 5, then only TWO other servers needs to reply.
                barriers[replicateIndex] = new Semaphore(1 + raft->num_servers / 2);
                for (int i=0; i<raft->num_servers; i++) {
                    if (i==serverId) continue;
                    std::async(&Server::replicateLogEntry, raft->servers[serverId], replicateIndex, i);
                }
                barriers[replicateIndex]->notify(serverId);
                barriers[replicateIndex]->wait(serverId);
                raft->syncCout("Log entry " + to_string(replicateIndex) + " has been replicated");

                // ideally, below lines should execute atomically
                stateMachine[request.key] += request.valueDelta;
                lastApplied = log.size() - 1;
                commitIndex = lastApplied + 1;
            }
            response.succeed = true;
        }
    }
    p.set_value(response);
    myLock.unlock();

    //The leader needs to replicate the message to other servers. So it should create separate threads for each OTHER server
    //and call "append" for each server
}

// this models a thread running on the LEADER and it tries replicate certain log entry to one CERTAIN follower
void Server::replicateLogEntry(int replicateIndex, int replicateTo) {
    //raft->syncCout("Server " + to_string(serverId) + " runs replicateLogEntry(" + to_string(replicateIndex) + ", " + to_string(replicateTo) + ");");
    AppendEntriesResponse response = repeatedlyAppendEntries(replicateIndex, replicateTo);
    if (!response.responded) {
        return;
    }

    if (!response.success) {
        // try previous entry
        int rollbackTo = replicateIndex;
        while (!response.success) {
            --rollbackTo;
            //assert(rollbackTo >= 0);
            response = repeatedlyAppendEntries(rollbackTo, replicateTo);
        } 
        // now replicate again starting from rollbackTo+1
        rollbackTo++;
        while (rollbackTo < replicateIndex) {
            response = repeatedlyAppendEntries(replicateIndex, replicateTo);
            //assert(response.success);
            rollbackTo++;
        }
        // done
        raft->syncCout("Server " + to_string(replicateTo) + " synchronized its log from server " + to_string(serverId) + " (index of last log entry = " + to_string(replicateIndex) + ")");
    }
    barriers[replicateIndex]->notify(serverId);
}

// run appendEntriesRPC for at most 10 times to account for dropout
AppendEntriesResponse Server::repeatedlyAppendEntries(int replicateIndex, int replicateTo) {
    AppendEntriesResponse response;
    for (int i=0; i<10; i++) {
        response = appendEntriesRPC(replicateIndex, replicateTo);
        if (response.responded) return response;
    }
    return response;
}

// RPC functions run on the caller
AppendEntriesResponse Server::appendEntriesRPC(int replicateIndex, int replicateTo) {
    if (raft->dropoutHappens()) {
        //raft->syncCout("Dropout appendEntries from " + to_string(serverId) + " to " + to_string(replicateTo));
        AppendEntriesResponse response;
        response.responded = false;
        return response;
    }
    
    if (!raft->belongToSamePartition(serverId, replicateTo)) {
        AppendEntriesResponse response;
        response.responded = false;
        return response;
    }

    AppendEntries request;

    if (replicateIndex == -1) { // Heartbeat message
        request.leaderCommit = -1;
        request.term = currentTerm;
    }
    else { // Normal message
        request.prevLogTerm = log[replicateIndex - 1].first;
        request.entry = log[replicateIndex].second;
        request.leaderCommit = commitIndex;
    }
    request.term = currentTerm;
    request.leaderId = serverId;
    request.prevLogIndex = replicateIndex - 1;

    std::promise<AppendEntriesResponse> p;
    auto f = p.get_future();
    std::thread t(&Server::appendEntries, raft->servers[replicateTo], request, std::move(p));
    t.join();

    if (raft->dropoutHappens()) {
        //raft->syncCout("Dropout appendEntries response from " + to_string(replicateTo) + " to " + to_string(serverId));
        AppendEntriesResponse response;
        response.responded = false;
        return response;
    }

    return f.get();
}

// RPC functions run on the caller
RequestVoteResponse Server::requestVoteRPC(RequestVote request, int sendTo) {
    if (raft->dropoutHappens()) {
        //raft->syncCout("Dropout requestVote from " + to_string(serverId) + " to " + to_string(sendTo));
        RequestVoteResponse response;
        response.responded = false;
        return response;
    }
    
    if (!raft->belongToSamePartition(serverId, sendTo)) {
        RequestVoteResponse response;
        response.responded = false;
        return response;
    }

    std::promise<RequestVoteResponse> p;
    auto f = p.get_future();
    std::thread t(&Server::requestVote, raft->servers[sendTo], request, std::move(p));
    t.join();

    if (raft->dropoutHappens()) {
        //raft->syncCout("Dropout requestVote response from " + to_string(sendTo) + " to " + to_string(serverId));
        RequestVoteResponse response;
        response.responded = false;
        return response;
    }

    return f.get();
}