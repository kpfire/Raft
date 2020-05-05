#include <string>

using namespace std;

#ifndef CLIENTREQUEST
#define CLIENTREQUEST

struct ClientRequest {
    string key;
    int valueDelta;
};

struct ClientRequestResponse {
    bool responded;
    bool succeed;
    string message;
};

#endif