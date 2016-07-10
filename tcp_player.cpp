#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

#include "checkers.h"


struct tcp_player_t : public player_t
{
protected:
    int sock_;

public:
    tcp_player_t(colour_e colour, int socket_)
        : player_t(colour)
        , sock_(socket_)
    {
        assert(sock_ != -1);
    }

    virtual bool is_connected()
    {
        return sock_ != -1;
    }

    virtual bool poll_move(move_t & move)
    {
        return false;
    }

    virtual bool invalid_move(const move_t & move)
    {
        return false;
    }

    virtual bool send_move(const move_t & move)
    {
        return false;
    }

    virtual bool send_board(const board_t & board)
    {
        return false;
    }

    virtual bool send_colour(colour_e c)
    {
        return false;
    }

    virtual bool request_move()
    {
        return false;
    }

    virtual bool bad_input()
    {
        return true;
    }
};



player_t * new_tcp_player(colour_e)
{
    return nullptr;
}
