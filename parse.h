#ifndef PARSE_H
#define PARSE_H

#include <chrono>
#include <string>
#include <algorithm>
#include <cmath>

namespace spclock {

using seconds = std::chrono::seconds;
using string = std::string;

inline bool strTime_valid(const string& str) {
    // checking the string passed in has only numbers and ':'

    if (str.length() > 15) {
        // arbitrary limit
        return false;
    }
    if (std::count(str.begin(), str.end(), ':') > 2) {
        return false;
    }

    auto firstItr = str.begin();
    
    if ((*firstItr) == '-')
        ++firstItr;

    auto itr = std::find_if(firstItr, str.end(), 
                               [](char c){ return !isdigit(c) && (c!=':');});
    return (itr == str.end());
};

inline seconds parse_time(const string& str) {
    if (!strTime_valid(str)) {
        throw std::runtime_error("Time format not valid");
    }
    int64_t ret{0};
    int colon_count = 0;
    int dist_toColon = 0; 
    int sign = 1;
    for(auto itr = str.crbegin(); itr != str.crend(); ++itr) {
        if (*itr == ':') {
            ++colon_count;
            dist_toColon = 0;
        } else if (*itr == '-') {
            sign = -1;
        } else {
            int64_t bit_multi = pow(10, dist_toColon);
            int64_t clock_multi = pow(60, colon_count);
            ret += (*itr - '0') * bit_multi * clock_multi;
            ++dist_toColon;
        }
    }

    return seconds{ret*sign};
};

} // namespace spclock
#endif
