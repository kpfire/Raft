#include <string>

using namespace std;

#ifndef APPENDENTRIES
#define APPENDENTRIES

struct AppendEntries {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;
    pair<string, int> entry;
    int leaderCommit;
};

struct AppendEntriesResponse {
    int term;
    bool success;
};

#endif