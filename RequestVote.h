#include <string>

using namespace std;

#ifndef REQUESTVOTE
#define REQUESTVOTE

struct RequestVote {
    int term;// candidate’s term
    int candidateId;// candidate requesting vote
    int lastLogIndex;// index of candidate’s last log entry (§5.4)
    int lastLogTerm;// term of candidate’s last log entry (§5.4)
};

struct RequestVoteResponse {
    int term;
    bool voteGranted;
};

#endif