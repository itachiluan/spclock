#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <future>
#include <vector>
#include "spclock.h"
#include "parse.h"

std::vector<spclock::buzzer> buzzers;
std::vector<std::thread> sleeps;
std::mutex buzz_m;
std::mutex vec_m;
int current_buzz_ID;

template <typename T>
void mut_push_back(std::vector<T>& vec, T&& item) {
    std::lock_guard<std::mutex> lck(vec_m);
    vec.push_back(std::forward<T>(item));
}

void print_info(const std::vector<spclock::buzzer>& buzzers) {
    using std::stringstream;
    using std::string;
    std::unique_lock<std::mutex> lck{vec_m};

    constexpr int ID_width{5};
    constexpr int time_width{12};
    constexpr int t_toFin_width{34};
    constexpr int message_width{40};
    
    string edge{'+' + string(ID_width + 1, '-') +
                '+' + string(time_width + 1, '-') + 
                '+' + string(t_toFin_width + 1, '-') + 
                '+' + string(message_width + 1, '-') + '+'};

    std::cout << '\n' << edge << '\n'
              << '|' << std::setw(ID_width) << "ID" << " |"
              << std::setw(time_width) << "Finish Time" <<" |"
              << std::setw(t_toFin_width) << "Time to finish" << " |"
              << std::setw(message_width) << "Message" << " |"
              << '\n' << edge << '\n';

    auto now = spclock::now();
    for (int i=0; i!=buzzers.size(); ++i) {
        if (buzzers[i].state == spclock::b_state::running) {
            string out_message = buzzers[i].message;
            if (out_message.size() > message_width) {
                out_message = out_message.substr(0, message_width - 3) + "...";
            }
            std::cout << "|" 
              << std::setw(ID_width) << i << " |"
              << std::setw(time_width) << buzzers[i].end_time.format("%H:%M:%S")
              << " |"
              << std::setw(t_toFin_width) << buzzers[i].end_time - now << " |"
              << std::setw(message_width) << out_message << " |"
              << '\n';
        }
    }
    
    std::cout << edge << std::endl;
}

void print_arg_err() {
    std::cout << "Argument error." 
    << "\nUsage: alarm|timer|calc|now|quit <time> [message] "
    << std::endl;
}

void calc_time(const std::string& str) {
    try {
        auto sec = spclock::parse_time(str);
        std::cout << spclock::now() + sec << std::endl;
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

std::vector<std::string> get_cmds() {
    using std::string;
    using std::stringstream;

    string line;
    getline(std::cin, line);
    stringstream ss;
    ss << line;
    std::vector<string> cmds;
    string cmd;
    while (ss >> cmd) {
        cmds.push_back(cmd);
    }

    return cmds;
}

void stop_buzzer(size_t buzzer_ID, spclock::b_state stop_state) {
    std::lock_guard<std::mutex> lck(vec_m);
    if (buzzer_ID > buzzers.size() - 1)
        return;
    if (buzzers[buzzer_ID].state == spclock::b_state::running) {
        buzzers[buzzer_ID].stop.set_value();
        buzzers[buzzer_ID].state = stop_state;
    }
}

void I_sleep(size_t buzzer_ID) {
    using namespace std::chrono;
    auto sec = buzzers[buzzer_ID].end_time - spclock::now();
    auto ft = buzzers[buzzer_ID].stop.get_future();
    auto status = ft.wait_for(sec);
    if (status != std::future_status::ready) {
        std::unique_lock<std::mutex> lck(buzz_m);
        current_buzz_ID = buzzer_ID;
        for (;;) {
            status = ft.wait_for(milliseconds{500});
            if (status != std::future_status::ready)
                spclock::make_sound();
            else {
                stop_buzzer(buzzer_ID, spclock::b_state::finished);
                current_buzz_ID = -1;
                break;
            }
        }
    } else {
        stop_buzzer(buzzer_ID, spclock::b_state::cancelled);
        std::lock_guard<std::mutex> lck{buzz_m};
        std::cout << "\nID " << buzzer_ID
             << " is cancelled!\n\n"; // ...this has to be done better..
    }
}

void add_buzzer(const std::vector<std::string>& cmds) {
    spclock::seconds sec;
    try {
        sec = spclock::parse_time(cmds[1]);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return ;
    }
    std::string msg;
    if (cmds.size() < 3) {
        msg = "";
    } else {
        msg = cmds[2];
    }
    spclock::buzzer b(sec, cmds[0], msg);
    if (b.end_time > spclock::now()) {
        mut_push_back(buzzers, std::move(b));
        std::thread t{I_sleep, buzzers.size() - 1};
        mut_push_back(sleeps, std::move(t));
        std::cout << "\n" << cmds[0] << " is set.\n\n";
    } else {
        std::cout << "We cannot go back in time right?" << std::endl;
    }
}

void stop_all() {
    for (size_t i = 0; i < buzzers.size(); ++i) {
        stop_buzzer(i, spclock::b_state::cancelled);
    }

    for (auto& t:sleeps) {
        if (t.joinable())
            t.join();
    }
}

template <typename Container>
std::vector<std::string> modify_cmds(const Container& cmds) {
    using namespace std;
    if (cmds.size() <= 2) {
        //arguments passed in doesn't involve message
        return cmds;
    }
    
    // args with message info.
    if ((cmds[0] == "alarm") || (cmds[0] == "timer")) {
        vector<string> ret;
        for (int i=0; i<2; ++i) {
            ret.push_back(cmds[i]);
        }
        string msg;
        for (int i=2; i<cmds.size(); ++i) {
            msg += " " + cmds[i];
        }
        ret.push_back(msg);

        return ret;
    } else {
        return cmds;
    }
}

template <typename Container>
void exec_cmds(const Container& cmds_in) {
    auto cmds = modify_cmds(cmds_in);
    if (cmds[0] == "now") {
        std::cout << spclock::now() << std::endl;

    } else if (cmds[0] == "calc") {
        calc_time(cmds[1]);

    } else if ((cmds[0] == "alarm") || (cmds[0] == "timer")) {
        add_buzzer(cmds);
    
    } else if (cmds[0] == "quit") {
        stop_all();
        std::cout << "Bye.\n";

    } else if (cmds[0] == "stop") {
        if (cmds.size() > 1) {
            std::stringstream ss;
            ss << cmds[1];
            size_t idx;
            ss >> idx;
            stop_buzzer(idx, spclock::b_state::cancelled);
        } else {
            if (current_buzz_ID >= 0) 
                stop_buzzer(current_buzz_ID, spclock::b_state::cancelled);
        }
    } else if (cmds[0] == "list") {
        print_info(buzzers);

    } else {
        print_arg_err();
    }
}

int main(int argc, char **argv){
    if (argc < 2) {
        print_arg_err();
        return 1;
    }
    
    std::vector<std::string> cmds(argv + 1, argv + argc);
    current_buzz_ID = -1;
    exec_cmds(cmds);
    if ((cmds[0] == "alarm") || (cmds[0] == "timer")) {
        while (true) {
            std::cout << ">> ";
            cmds = get_cmds();
            if (cmds.size() > 0) {
                exec_cmds(cmds);
                if (cmds[0] == "quit")
                    break;
            }
        }
    }
}
