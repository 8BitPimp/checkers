#include "checkers.h"

bool board_t::reset()
{
    // clear the board
    board.fill(EMPTY);
    // setup white pieces
    for (int i=0; i<12; i++) {
        const int y = (i/4);
        const int x = (i*2+(y&1)) % 8;
        piece[00+i].owner = WHITE;
        piece[00+i].type = SINGLE;
        piece[00+i].pos = pos_t {x, y};
    }
    // setup black pieces
    for (int i=0; i<12; i++) {
        const int y = (i/4)+5;
        const int x = (i*2+(y&1)) % 8;
        piece[12+i].owner = BLACK;
        piece[12+i].type = SINGLE;
        piece[12+i].pos = pos_t {x, y};
    }
    // assign pieces to the board
    for (size_t index=0; index<piece.size(); ++index) {
        piece_t & p = piece[index];
        board[p.pos.x + p.pos.y*8] = index;
    }
    return true;
}

bool board_t::serialize(std::string &out) const
{
    // clear output string
    out.clear();
    // for each square on the board
    for (int32_t i = 0; i<board.size(); ++i) {

        const auto piece_index = board[i];

        // if square is empty
        if (piece_index == EMPTY) {
            out += ((i^(i/8))&1) ? "  " : "__";
            continue;
        }
        // square is occupied
        else {
            // reference to the indexed piece
            const piece_t & p = piece[piece_index];
            // append owner of the piece
            out += p.owner == WHITE ? "W" : "B";
            // append the piece type
            switch (p.type) {
            case (SINGLE):
                out += "S";
                break;
            case (CROWNED):
                out += "K";
                break;
            default:
                assert(!"unknown piece type while serializing board state");
                return false;
            }
        }
    }
    return true;
}

bool board_t::get_piece(const pos_t pos, piece_t * & out)
{
    // check the position is on the board
    if (pos.x < 0 || pos.x > 7 ||
        pos.y < 0 || pos.y > 7) {
        return false;
    }
    // find 2d position index into 1d board array
    const int index = pos.x + pos.y * 8;
    // only black squares are valid
    if ((index & 1) != (pos.y & 1)) {
        return false;
    }
    // copy out this square
    {
        const auto piece_index = board[index];
        if (piece_index==EMPTY) {
            return nullptr;
        }
        out = &piece[piece_index];
    }
    return true;
}

bool board_t::move(const pos_t from,
                   const pos_t to,
                   const colour_e turn,
                   std::deque<event_t> & events)
{
    // get piece that the player is moving
    piece_t * piece = nullptr;
    if (!get_piece(from, piece)) {
        return false;
    }
    // check player is only moving their peices
    if (piece->owner!=turn) {
        return false;
    }
    // check move destination is unoccupied
    {
        piece_t *dummy = nullptr;
        if (get_piece(to, dummy)) {
            return false;
        }
    }
    // compute move differential
    const int32_t dx = to.x - from.x;
    const int32_t dy = to.y - from.y;
    // piece must move on a pure diagonal
    if (abs(dx)!=abs(dy)) {
        return false;
    }
    // can only move only 1 square or 2 squares away if capturing
    if (abs(dx) > 2) {
        return false;
    }
    // there are special rules for uncrowned pieces
    if (piece->type == SINGLE) {
        switch (piece->owner) {
        case (WHITE):
            if (dy<0) {
                // white must move downward only when uncrowned
                return false;
            }
            break;
        case (BLACK):
            if (dy>0) {
                // black must move upward only when uncrowned
                return false;
            }
            break;
        }
        // check if piece has moved to opposite edge and can be crowned
        if (to.y == (piece->owner==WHITE ? 7 : 0)) {
            piece->type = CROWNED;
            events.push_back(event_t{event_t::CROWN, to});
        }
    }
    // if this is a capturing move
    if (abs(dx) == 2) {
        // find square that is being jumped
        const pos_t mid = {(from.x+to.x)/2,
                           (from.y+to.y)/2};
        // try to find the piece that is being captured
        piece_t * taken = nullptr;
        if (!get_piece(mid, taken)) {
            // no piece is being captured
            return false;
        }
        // check for capture of same colour
        if (taken->owner == piece->owner) {
            // cant take your own pieces
            return false;
        }
        // push capture into renderer event list
        events.push_front(event_t{event_t::CAPTURE, {mid, mid}});
        // remove captured piece
        taken->type = CAPTURED;
        taken->pos = pos_t{-1, -1};
        board[mid.x + mid.y*8] = EMPTY;
    }
    // push move event into renderer event list
    events.push_front(event_t{event_t::MOVE, {from, to}});
    // update the board to reflect the movement
    piece->pos = to;
    {
        const int32_t from_index = from.x + from.y*8;
        const int32_t to_index = to.x + to.y*8;
        board[to_index] = board[from_index];
        board[from_index] = EMPTY;
    }
    return true;
}
