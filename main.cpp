#include <thread>
#include <SDL/SDL.h>
#include "checkers.h"

void game_thread(checkers_t * game)
{
    while (game->is_active()) {
        // check for and handle moves from the current player
        game->poll_players();
        // give time back to the CPU
        SDL_Delay(1);
    }
}

int main(int argc, char * args[])
{
    tcp_factory_t fact_;
    if (!fact_.start()) {
        return 1;
    }

    // create two players
    std::array<player_t *, 2> players = {
        fact_.new_tcp_player(BLACK),
        fact_.new_tcp_player(WHITE)
    };

    // wait for both players to connect
    for (;;) {
        // if both players have connected
        if (players[0]->is_connected() &&
            players[1]->is_connected()) {
            // let the match begin
            break;
        }
        // not all players connected
        else {
            // give time back to the CPU
            SDL_Delay(1);
        }
    }
    // create the board renderer
    render_t * render = new_sdl_render();
    if (render && !render->init()) {
        return 1;
    }
    // create a new board with the selected players
    checkers_t game = checkers_t(players, render);
    // spark a new thread to simulate the game in
    std::thread * thread = new std::thread(game_thread, &game);
    if (!thread) {
        return 1;
    }
    // while there is an active game being played
    while (game.is_active()) {
        // draw the board to the screen
        if (!render->tick()) {
            game.end();
        }
        // give time back to the CPU
        SDL_Delay(1);
    }
    // wait for the game thread to die
#if 0
    thread->join();
#endif

    // end
    fact_.stop();
    return 0;
}
