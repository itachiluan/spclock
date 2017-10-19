#ifndef SPTIME_H
#define SPTIME_H

#include <iostream>
#include <future>
#include "date/tz.h"

namespace spclock {
using std::chrono::seconds;

class local_time {
    date::zoned_time<seconds> zoned_tp_;

public:
    local_time(); // construct current time
    // constructing an arbitrary time of the current day
    local_time(const seconds&);

    const date::time_zone* zone() const;

    std::string format(const std::string&) const;

    // no + operation defined same as std::chrono::time_point
    friend seconds operator-(const local_time&, const local_time&);
    friend bool operator>(const local_time&, const local_time&);
    friend bool operator<(const local_time&, const local_time&);
    friend local_time operator+(const local_time&, const seconds&);
    friend std::ostream& operator<<(std::ostream&, const local_time&);
};

local_time now();

// classes related to alarm and timer constructs
enum class b_type {
    alarm,
    timer
};

enum class b_state {
    running,
    finished,
    cancelled
};

struct buzzer {
    local_time end_time;
    std::string message;
    b_type buzzer_type;
    b_state state;
    std::promise<void> stop;

    explicit buzzer(seconds,const std::string&, std::string);
    buzzer(const buzzer&) = delete; //no copy - it has promise
    buzzer(buzzer&&);
};

void make_sound();

} // namespace spclock

// outputs duration in a very verbose way
std::ostream& operator<<(std::ostream&, const std::chrono::seconds&);

#endif
