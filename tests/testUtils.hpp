#pragma once
#include <iostream>
#include <sstream>

class StreamRedirect {
    std::ostream& stream;
    std::streambuf* original;
    std::ostringstream buffer;
public:
    StreamRedirect(std::ostream& s) : stream(s), original(s.rdbuf()) {
        stream.rdbuf(buffer.rdbuf());
    }
    ~StreamRedirect() { stream.rdbuf(original); }
    std::string str() const { return buffer.str(); }
};