#pragma once
#include <vector>
#include <map>
#include "common.h"

NAMESPACE_START(cm)

struct CmdStream {
    std::vector<std::string> commands;

    CmdStream () {
        commands.reserve(128);
    }

    template <class... FmtArgs>
    CmdStream& add(std::format_string<FmtArgs...> command_fmt, FmtArgs&&... fmt_args) {
        commands.push_back(std::format(command_fmt, std::forward<FmtArgs>(fmt_args)...));
        return *this;
    }

    void clear() {
        commands.clear();
    }

    void run() {
        for (auto& cmd : commands) {
            std::system(cmd.data());
        }
    }
};



// prompt user via std::istream
template <bool read_line=true, class T>
inline T& prompt(T& lval) {
    if constexpr (read_line) {
        std::getline(std::cin, lval);
    }
    else { // read_line == false
        std::cin >> lval;
    }
    return lval;
}

// print to stdout via std::ostream
template <bool _flush=true, class... Args>
inline void print(Args&&... args) {
    (std::cout << ... << std::forward<Args>(args));
    if(_flush) std::flush(std::cout);
}

// terminate the application
template <int _errc=1, class... Args>
[[noreturn]] inline void terminate(Args&&... args) {
    (std::cerr << ... << std::forward<Args>(args)) << std::endl;
    std::exit(_errc);
}



const char* concats(const char* base, const char* append) {
    static const char* res = std::string(base).append(append).data();
    return res;
}

template <class T>
inline bool vec_contains(std::vector<T> vector, T element) {
    return std::find(vector.begin(), vector.end(), element) != vector.end();
}

inline bool prefix_strip(const std::string& str, const std::string& prefix, std::string* out) {
    if (str.substr(0, prefix.size()) != prefix) return false;
    if (out) *out = str.substr(prefix.size());
    return true;
}

// create a new file in the file system
inline bool newFile(fs::path path) {
    std::fstream new_file(path);
    if (!new_file) return false;
    return true;
}

//
inline const char* userHomePath(bool terminate_on_fail = false, const char* fail_msg="") {
    static const char* home_path = getenv("HOME");
    if (home_path == NULL) {
        if (terminate_on_fail) terminate(fail_msg);
        else return nullptr;
    }
    return home_path;
}

// parse file path by converting tilde('~') to $HOME variable
inline fs::path parsePathTilde(std::string path) {
    if (!(path[0] == '~')) return path;
    path.erase(0, 1);
    const char* user_home = userHomePath(true, "User does not have $HOME set");
    path.insert(0, user_home);
    return path;
}

NAMESPACE_END(cm)
