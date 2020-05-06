#include <stdlib.h>
#include <string>
#include <iterator>
#include <ctime>
#include <ratio>
#include <chrono>

#ifndef UTILITIES
#define UTILITIES

const int OBSERVER_NODE_ID = 1000001;

enum ServerState {Leader, Candidate, Follower};

// split string utility function
template <class Container>
void split1(const std::string& str, Container& cont)
{
    std::istringstream iss(str);
    std::copy(std::istream_iterator<std::string>(iss),
         std::istream_iterator<std::string>(),
         std::back_inserter(cont));
}

template <class Container>
void split2(const std::string& str, const std::string& delim, Container& cont)
{
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) cont.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
}

// this struct allows using pairs as keys in hash tables
struct pair_hash
{
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2> &pair) const
	{
		return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
	}
};

// Timing functions
using namespace std::chrono;

// Get current epoch time in seconds
static double time_now() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / 1000.;
}

// Return time passed since t in seconds
static double time_passed(double t) {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / 1000. - t;
}

using namespace std;

// Read config into vector of ints
static vector<int> read_config(string s) {
    string to_delete = "config=";
    s.erase(0, to_delete.length());
    vector<int> ids;
    stringstream ss(s);
    while(ss.good()) {
        string substr;
        getline(ss, substr, ',');
        ids.push_back(stoi(substr));
    }
    return ids;
}

// Find last config entry
static int find_config_index(vector<pair<int, string>> l) {
    auto result = find_if(l.rbegin(), l.rend(), [](pair<int, string> p) {return p.second.find("config") != string::npos;});
    return distance(result, l.rend()) - 1;
}

#endif