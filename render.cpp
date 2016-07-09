#include <SDL/SDL.h>
#include <map>
#include <thread>
#include <mutex>

#include "checkers.h"

struct sdl_render_t : public render_t {
protected:

    struct vec3f_t {
        float x, y, z;
    };

    static const int WIDTH = 154;
    static const int HEIGHT = 91;
    static const int TICK_INTERVAL = 1000/25;

    SDL_Surface * screen;
    SDL_Surface * sub_target;
    SDL_Surface * bmp_board;
    SDL_Surface * bmp_sprites;

    std::array<piece_t, 12*2> pieces;
    std::array<vec3f_t, 12*2> pos;
    std::array<bool,    12*2> visible;
    std::deque<event_t> event_queue;

    bool active;
    int32_t ticks_next;
    std::mutex mux;

    // current event being processed
    event_t event;
    float delta;
    size_t active_piece;

    bool present()
    {
        const uint32_t * src = (const uint32_t*)sub_target->pixels;
        uint32_t * dst = (uint32_t*)screen->pixels;
        const uint32_t dp = (screen->pitch/4);
        for (uint32_t y = 0; y<HEIGHT; ++y) {
            for (uint32_t x = 0; x<WIDTH; ++x) {
                const uint32_t c = src[x];
                for (int i = 0; i<4; i++) {
                    dst[(dp*i)+(x*4)+0] = c;
                    dst[(dp*i)+(x*4)+1] = c;
                    dst[(dp*i)+(x*4)+2] = c;
                    dst[(dp*i)+(x*4)+3] = c;
                }
            }
            src += (sub_target->pitch/4);
            dst += (screen->pitch/4) * 4;
        }
        SDL_Flip(screen);
        SDL_FillRect(sub_target, nullptr, 0x101010);
        return true;
    }

    bool poll_sdl_events()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            active &= (event.type!=SDL_QUIT);
            if (event.type==SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    active = false;
                }
            }
        }
        return true;
    }

    bool lerp_piece(int index,
                    pos_t from,
                    pos_t to,
                    float lerp) {
        
        // lerp between positions
        float lx = float(to.x)*lerp+float(from.x)*(1.f-lerp);
        float ly = float(to.y)*lerp+float(from.y)*(1.f-lerp);
#if 0
        // triangle interpolation
        float lz = 2.f * fminf(lerp, 1-lerp);
#else
        // parabolic interpolation
        float lz = 4.f * -(lerp-1)*lerp;
#endif
        // save out the piece 3d position
        pos[index] = vec3f_t{lx, ly, lz};
        return true;
    }
    
    bool draw_shadow(int index)
    {
        // grab piece 3D position
        const float lx = pos[index].x;
        const float ly = pos[index].y;
        const float lz = pos[index].z;
        // grab some visible state information
        const piece_state_e state = pieces[index].type;
        // captured peices are not drawn
        if (state==piece_state_e::CAPTURED || !visible[index]) {
            return true;
        }
        // transform into 2D screen space
        const int32_t tx = int32_t(5.f +lx*12.f+ly*7.f + lz * 2.f);
        const int32_t ty = int32_t(30.f-lx *4.f+ly*7.f);
        // sprite sheet location
        SDL_Rect src = SDL_Rect {
            32, 0, 16, 8
        };
        // screen space location
        SDL_Rect dst = SDL_Rect {
            int16_t(tx) + (state==CROWNED ? 2 : 1),
            int16_t(ty),
            16,
            8
        };
        // blit bottom layer
        SDL_BlitSurface(bmp_sprites, &src, sub_target, &dst);
        return true;
    }

    bool draw_piece(int index)
    {
        // grab piece 3D position
        const float lx = pos[index].x;
        const float ly = pos[index].y;
        const float lz = pos[index].z;
        // grab some visible state information
        const colour_e clr = pieces[index].owner;
        const piece_state_e state = pieces[index].type;
        // captured peices are not drawn
        if (state==piece_state_e::CAPTURED || !visible[index]) {
            return true;
        }
        // transform into 2D screen space
        const int32_t tx = int32_t(5.f +lx*12.f+ly*7.f);
        const int32_t ty = int32_t(30.f-lx *4.f+ly*7.f - lz * 8.f);
        // sprite sheet location
        SDL_Rect src = SDL_Rect {
            (clr==WHITE ? 0 : 16), 0, 16, 8
        };
        // screen space location
        SDL_Rect dst = SDL_Rect {
            int16_t(tx), int16_t(ty), 16, 8
        };
        // blit bottom layer
        SDL_BlitSurface(bmp_sprites, &src, sub_target, &dst);
        // blit top layer
        if (state==piece_state_e::CROWNED) {
            dst.y -= 3;
            SDL_BlitSurface(bmp_sprites, &src, sub_target, &dst);
        }
        return true;
    }
    
    bool draw_pieces()
    {
        for (size_t i = 0; i<pieces.size(); ++i) {
            draw_shadow(int32_t(i));
        }
        for (size_t i = 0; i<pieces.size(); ++i) {
            if (i!=active_piece) {
                draw_piece(int32_t(i));
            }
        }
        if (active_piece!=-1) {
            draw_piece(active_piece);
        }
        return true;
    }

    bool handle_event_none()
    {
        delta = 0.f;
        active_piece = -1;
        if (!pop_event(event)) {
            return true;
        }
        for (size_t i = 0; i<pieces.size(); ++i) {
            piece_t & p = pieces[i];
            if (p.pos==event.pos[0]) {
                // grab the current piece
                active_piece = int32_t(i);
                return true;
            }
        }
        // unable to find the piece referenced by event
        assert(!"Unable to find piece in event");
        return false;
    }

    bool handle_event_move()
    {
        static const float SPEED = 0.0333f;
        assert(active_piece != -1);
        piece_t & current_piece = pieces[active_piece];
        // check if the event is over
        if ((delta += SPEED)>1.f) {
            event.type = event_t::NONE;
            // update the current piece to its new position
            current_piece.pos = event.pos[1];
            // make sure its at its resting place
            lerp_piece(active_piece,
                       event.pos[1],
                       event.pos[1],
                       1.f);
        }
        else {
            lerp_piece(active_piece,
                       event.pos[0],
                       event.pos[1],
                       delta);
        }
        return true;
    }

    bool handle_event_capture()
    {
        static const float SPEED = 0.1f;
        assert(active_piece != -1);
        piece_t & current_piece = pieces[active_piece];
        // check if the event is over
        if ((delta += SPEED)>1.f) {
            event.type = event_t::NONE;
            // this piece has been captured
            current_piece.type = piece_state_e::CAPTURED;
            // move off the board
            current_piece.pos = pos_t{-1, -1};
            // piece becomes invisible
            visible[active_piece] = false;
        }
        else {
            // flash the piece thats been captured
            visible[active_piece] = (int32_t(delta*32)&1) ? true : false;
        }
        return true;
    }

    bool handle_event_crown()
    {
        assert(active_piece != -1);
        piece_t & current_piece = pieces[active_piece];
        // check if the event is over
        if ((delta += 0.03f)>1.f) {
            event.type = event_t::NONE;
            // this piece has been crowned
            current_piece.type = piece_state_e::CROWNED;
        }
        else {
            // flash the piece thats being crowned
            current_piece.type = int32_t(delta*32)&1 ?
                                 piece_state_e::SINGLE :
                                 piece_state_e::CAPTURED;
            lerp_piece(active_piece,
                       current_piece.pos,
                       current_piece.pos,
                       0.f);
        }
        return true;
    }

    bool game_tick()
    {
        // draw the checkers board
        SDL_BlitSurface(bmp_board, nullptr, sub_target, nullptr);
        // dispatch activity based on event from game
        switch (event.type) {
        case (event_t::NONE):
            handle_event_none();
            break;
        case (event_t::MOVE):
            handle_event_move();
            break;
        case (event_t::CAPTURE):
            handle_event_capture();
            break;
        case (event_t::CROWN):
            handle_event_crown();
            break;
        }
        // draw the board pieces
        draw_pieces();
        // flush draw surface to the screen
        if (!present()) {
            assert(!"Error while trying to present");
            return false;
        }
        return true;
    }

    bool pop_event(event_t & out)
    {
        std::lock_guard<std::mutex> guard(mux);
        if (event_queue.empty()) {
            return false;
        }
        else {
            out = event_queue.front();
            event_queue.pop_front();
            return true;
        }
    }

public:

    sdl_render_t()
        : screen(nullptr)
        , sub_target(nullptr)
        , bmp_board(nullptr)
        , bmp_sprites(nullptr)
        , active(false)
        , event(event_t {event_t::NONE})
        , delta(0.f)
        , active_piece(-1)
    {
        for (bool & vis:visible) {
            vis = true;
        }
    }

    virtual bool init()
    {
        // init the SDL library
        if (SDL_Init(SDL_INIT_VIDEO)) {
            return false;
        }
        // open and SDL window at 4x scale
        screen = SDL_SetVideoMode(WIDTH*4, HEIGHT*4, 32, 0);
        if (!screen) {
            return false;
        }
        // craete subtarget
        sub_target = SDL_CreateRGBSurface(
            SDL_SWSURFACE, 
            WIDTH, 
            HEIGHT, 
            32, 
            0xff<<16, 
            0xff<<8, 
            0xff<<0, 
            0xff<<24);
        if (!sub_target) {
            return false;
        }
        // load src graphics
        bmp_board = SDL_LoadBMP("board.bmp");
        bmp_sprites = SDL_LoadBMP("sprites.bmp");
        // check all gfx have loaded
        if (!bmp_board||!bmp_sprites) {
            return false;
        }
        // set transparency for the sprites
        SDL_SetColorKey(bmp_sprites, SDL_SRCCOLORKEY, 0xff00ff);
        // renderer is active
        active = true;
        ticks_next = SDL_GetTicks();
        // 
        SDL_WM_SetCaption("Checkers", nullptr);
        return true;
    }

    virtual bool tick()
    {
        // check the renderer has been initalized
        if (!screen || !active) {
            return false;
        }
        // handle any events that come from SDL
        if (!poll_sdl_events()) {
            return false;
        }
        // wait for our timeslice
        int32_t delta = SDL_GetTicks()-ticks_next;
        if (delta >= 0) {
            if (delta>500) {
                ticks_next += delta;
            }
            ticks_next += TICK_INTERVAL;
            game_tick();
        }
        return active;
    }

    virtual bool push_event(const event_t & event)
    {
        std::lock_guard<std::mutex> guard(mux);
        event_queue.push_back(event);
        return true;
    }

    virtual bool set_pieces(const board_t & board)
    {
        pieces = board.piece;
        for (size_t i = 0; i<pieces.size(); ++i) {
            piece_t & p = pieces[i];
            lerp_piece(int32_t(i), p.pos, p.pos, 0.f);
        }
        return true;
    }
};

render_t * new_sdl_render() {
    return new sdl_render_t;
}
