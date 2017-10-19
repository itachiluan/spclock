#include <iostream>
#include "spclock.h"
#include "parse.h"

namespace spclock {

local_time::local_time() {
    using std::chrono::system_clock;
    zoned_tp_ = date::make_zoned(
             date::current_zone(), date::floor<seconds>(system_clock::now()));
};

local_time::local_time(const std::chrono::seconds& sec) {
    using std::chrono::system_clock;
    auto zone = date::current_zone();
    auto ltp = date::make_zoned(zone, system_clock::now());
    auto lday = date::floor<date::days>(ltp.get_local_time());
    zoned_tp_ = make_zoned(zone, lday + sec);
};

const date::time_zone* local_time::zone() const {
    return zoned_tp_.get_time_zone();
};

std::string local_time::format(const std::string& fmt) const {
    return date::format(fmt, zoned_tp_);
};

seconds operator-(const local_time& lt1, const local_time& lt2) {
    return date::floor<seconds>(lt1.zoned_tp_.get_sys_time() -
                                    lt2.zoned_tp_.get_sys_time());
};

local_time operator+(const local_time& lt1, const seconds& dur) {
    local_time ret;
    ret.zoned_tp_ = make_zoned(lt1.zone(),
           date::floor<seconds>(lt1.zoned_tp_.get_sys_time() + dur));
    return ret;
};

bool operator>(const local_time& lt1, const local_time& lt2) {
    return lt1.zoned_tp_.get_sys_time() > lt2.zoned_tp_.get_sys_time();
}
bool operator<(const local_time& lt1, const local_time& lt2) {
    return lt1.zoned_tp_.get_sys_time() < lt2.zoned_tp_.get_sys_time();
}

std::ostream& operator<<(std::ostream& os, const local_time& lt) {
    std::ostringstream oss;
    oss << lt.zoned_tp_;
    return os << oss.str();
};

local_time now() {
    return local_time();
};

buzzer::buzzer(seconds sec, const std::string& type, std::string message) {
    if (type == "alarm") {
        this->buzzer_type = b_type::alarm;
        this->end_time = local_time(sec);
    } else if (type == "timer") {
        this->buzzer_type = b_type::timer;
        this->end_time = now() + sec;
    }
    this->message = message;
    state = b_state::running;
};

buzzer::buzzer(buzzer&& other)
      :end_time{other.end_time}, message{std::move(other.message)},
       buzzer_type{std::move(other.buzzer_type)}, state{std::move(other.state)},
       stop{std::move(other.stop)} { }

void make_sound() {
    std::cout << '\7' << std::flush;
};

} //namespace spclock

// output duration in a verbose format
std::ostream& operator<<(std::ostream& os, const std::chrono::seconds& dur) {
    auto t = date::make_time(dur);
    std::ostringstream oss;
    std::string sign = (dur < std::chrono::seconds{0}) ? "- ": "";
    oss << sign << t.hours().count() << " hours " << t.minutes().count() 
                       << " minutes " << t.seconds().count() << " seconds";
    return os << oss.str();
};

