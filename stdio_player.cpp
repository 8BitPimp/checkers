#include <cstdio>

#include "checkers.h"

namespace {
bool str_to_pos(const char *in, pos_t &out)
{
    out = pos_t{ in[0]-'0', in[1]-'0' };
    return (out.x>=0 && out.x<=7 &&
            out.y>=0 && out.y<=7);
}
} // namespace {}

struct stdio_player_t : public player_t
{
protected:
    static const size_t MAX_MOVE_SIZE = 256;
    std::array<char, MAX_MOVE_SIZE> input;

    bool poll_input()
    {
        // ask player to make a move
        printf("%s move: ", colour==WHITE ? "white" : "black");
        // read move from stdin
        fgets(input.data(), input.size(), stdin);
        // expect message start token
        return true;
    }

    bool draw_board(const std::string & input)
    {
        if (input.size()!=128) {
            return false;
        }
        printf(".  0 1 2 3 4 5 6 7\n");
        for (int i = 0; i<input.size(); ++i) {
            if (i%16==0) {
                if (i>0) {
                    putc('\n', stdout);
                }
                printf("%d ", i/16);
            }
            putc(input[i], stdout);
        }
        putc('\n', stdout);
        putc('\n', stdout);
        return true;
    }

public:
    stdio_player_t(colour_e colour)
        : player_t(colour)
    {
    }

    virtual bool is_connected()
    {
        return true;
    }

    virtual bool poll_move(move_t & move)
    {
        move.clear();
        // try to read a message from stdin
        if (!poll_input()) {
            return true;
        }
        // loop to parse each coordinate pair we receive
        for (size_t i = 0; i<input.size(); i+=2) {
            pos_t p;
            if (!str_to_pos(&input[i], p)) {
                break;
            }
            move.push_back(p);
        }
        // must have two coordinates for valid move
        return move.size() >= 2;
    }

    virtual bool invalid_move(const move_t & move)
    {
        std::string serial_move;
        if (!move.serialize(serial_move)) {
            assert(!"Unable to serialize move");
            return false;
        }
        printf("Illegal Move: %s\n", serial_move.c_str());
        return true;
    }

    virtual bool send_move(const move_t & move)
    {
        std::string serial_move;
        if (!move.serialize(serial_move)) {
            assert(!"Unable to serialize move");
            return false;
        }
        printf("Opposition move: %s\n", serial_move.c_str());
        return true;
    }

    virtual bool send_board(const board_t & board)
    {
        std::string board_serial;
        // send other player board state
        if (!board.serialize(board_serial)) {
            assert(!"error trying to serialize board state");
            return false;
        }
        draw_board(board_serial);
        return true;
    }

    virtual bool send_colour(colour_e c)
    {
        return true;
    }

    virtual bool request_move()
    {
        return true;
    }

    virtual bool bad_input() {
        printf("Bad input\n");
        return true;
    }
};

player_t * new_stdio_player(colour_e colour)
{
    return new stdio_player_t(colour);
}
