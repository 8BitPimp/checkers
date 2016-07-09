#include "checkers.h"

bool move_t::serialize(std::string &out) const
{
    // loop over all move coordinates
    for (const auto & pos : *this) {
        // validate coordinate
        if (pos.x<0 || pos.x>7 || pos.y<0 || pos.y>7) {
            return false;
        }
        // append as plain text
        const char coord[3] = {char('0' + pos.x),
                               char('0' + pos.y),
                               '\0'};
        out += coord;
    }
    return true;
}

bool move_t::pop(pos_t & out)
{
    if (empty()) {
        return false;
    }
    else {
        out = front();
        pop_front();
        return true;
    }
}
