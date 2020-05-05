#include <string>

using namespace std;

#ifndef APPENDENTRIES
#define APPENDENTRIES

struct AppendEntries {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;
    string entry;
    int leaderCommit;
};

struct AppendEntriesResponse {
    bool responded;
    int term;
    bool success;
};

#endif