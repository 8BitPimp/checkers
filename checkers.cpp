#include "checkers.h"

checkers_t::checkers_t(
        std::array<player_t*, 2> & p,
        render_t * r)
    : player(p)
    , render(r)
    , active(true)
{
    // setup the starting board state
    init();
}

bool checkers_t::apply_move(const move_t & move)
{
    // clone the board to try out this move
    board_t clone = board;
    // collect events for the renderer
    std::deque<event_t> events;
    // for each pair in the move list
    for (size_t i=1; i<move.size(); ++i) {
        // try to apply this move
        if (!clone.move(move[i-1],
                        move[i],
                        player[0]->colour,
                        events)) {
            return false;
        }
    }
    // move applies to clone becomes new board state
    board = clone;
    // feed generated events to the renderer
    for (auto & event : events) {
        render->push_event(event);
    }
    // success
    return true;
}

bool checkers_t::init_players()
{
    // randomize starting player
    if (rand() & 1) {
        std::swap(player[0], player[1]);
    }
    // send colours to each player
    player[0]->send_colour(player[0]->colour);
    player[1]->send_colour(player[1]->colour);
    // send the current board state to both players
    {
        // send board state to both players
        player[0]->send_board(board);
        player[1]->send_board(board);
    }
    // ask player 0 to make their move
    player[0]->request_move();
    return true;
}

bool checkers_t::init()
{
    if (!board.reset()) {
        assert(!"unable to reset the game board");
        return false;
    }
    if (!init_players()) {
        assert(!"unable to init the players");
        return false;
    }
    // give the renderer our starting board state
    render->set_pieces(board);
    return true;
}

bool checkers_t::is_active() const
{
    return active;
}

bool checkers_t::end()
{
    active = false;
    return true;
}

bool checkers_t::poll_players()
{
    // request move from the current player
    move_t move;
    if (!player[0]->poll_move(move)) {
        // error polling the client for a move
        player[0]->bad_input();
        return false;
    }
    // bail out if no move was delivered (not yet ready)
    if (move.size() == 0) {
        return true;
    }
    // try to apply the move we have received
    if (!apply_move(move)) {
        // tell the player they requested an invalid move
        player[0]->invalid_move(move);
        return true;
    }
    // send player the previous board state
    if (!player[1]->send_move(move)) {
        assert(!"error sending player opponent move");
        return false;
    }
    // send the current board state
    if (!player[1]->send_board(board)) {
        assert(!"error sending player current board state");
        return false;
    }
    // request current move
    player[1]->request_move();
    // swap current players turn
    std::swap(player[0], player[1]);
    return true;
}
