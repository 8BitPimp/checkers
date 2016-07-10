#pragma once
#include <cstdint>
#include <cassert>
#include <vector>
#include <queue>
#include <string>
#include <array>

static const int32_t EMPTY = -1;

struct pos_t
{
    int32_t x, y;

    bool operator == (const pos_t & rhs) const {
        return x==rhs.x && y==rhs.y;
    }
};

enum colour_e
{
    WHITE = 0,
    BLACK
};

enum piece_state_e
{
    CAPTURED = 0,
    SINGLE,
    CROWNED,
};

struct piece_t
{
    colour_e owner;
    piece_state_e type;
    pos_t pos;
};

struct move_t
    : public std::deque<pos_t>
{
    bool serialize(std::string & out) const;
    bool pop(pos_t & out);
};

struct event_t {
    enum {
        NONE = 0,
        MOVE,
        CAPTURE,
        CROWN
    } type;
    pos_t pos[2];
};

struct board_t
{
    // board layout
    std::array<int32_t, 8*8> board;
    // board pieces (12 * {white, black})
    std::array<piece_t, 12*2> piece;

    bool serialize(std::string & out) const;

    bool reset();

    piece_t * operator [] (const pos_t);
    const piece_t * operator [] (const pos_t) const;

    bool move(const pos_t from,
              const pos_t to,
              const colour_e turn,
              std::deque<event_t> & events);

    bool get_piece(const pos_t, piece_t * &out);
};

struct player_t
{
    const colour_e colour;
    player_t(colour_e c) : colour(c) {}

    // query if the player is currently connected
    virtual bool is_connected() = 0;
    // check if we have received a move from the player
    virtual bool poll_move(move_t & out) = 0;
    // inform the player they have sent an invalid move
    virtual bool invalid_move(const move_t & move) = 0;
    // send the opponents move
    virtual bool send_move(const move_t & move) = 0;
    // send the entire board state
    virtual bool send_board(const board_t & board) = 0;
    // send the player their colour
    virtual bool send_colour(colour_e) = 0;
    // tell the player to begin their turn
    virtual bool request_move() = 0;
    // tell the player they sent over bad input
    virtual bool bad_input() = 0;
};

struct render_t
{
    // start the renderer
    virtual bool init() = 0;
    // per frame renderer update
    virtual bool tick() = 0;
    // add a board event to the renderers event queue
    virtual bool push_event(const event_t & event) = 0;
    // set piece and board state
    virtual bool set_pieces(const board_t & board) = 0;
};

struct checkers_t
{
    // ctor
    checkers_t(std::array<player_t*, 2> &,
               render_t *);
    // is the game currently active
    bool is_active() const;
    // poll the players for moves
    bool poll_players();
    // end the game
    bool end();

protected:
    bool init_players();
    bool init();
    bool apply_move(const move_t &);

    // the current board state
    board_t board;
    // connected players
    std::array<player_t*, 2> player;
    bool active;
    render_t * render;
};

struct tcp_factory_t
{
    tcp_factory_t();
    ~tcp_factory_t();
    bool start();
    bool stop();
    player_t * new_tcp_player(colour_e clr);
protected:
    struct impl_t;
    impl_t * imp_;
};

extern player_t * new_stdio_player(colour_e);
extern render_t * new_sdl_render();

void log(const char * fmt, ...);
