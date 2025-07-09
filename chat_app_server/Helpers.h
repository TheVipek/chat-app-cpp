#pragma once
#include <iostream>
#include <string>
#include <stdexcept>

namespace Helpers
{
    inline bool isInteger(const std::string& s) {
        try {
            size_t pos;
            std::stoi(s, &pos);
            return pos == s.length();
        }
        catch (const std::invalid_argument& e) {
            return false;
        }
        catch (const std::out_of_range& e) {
            return false;
        }
    }
}