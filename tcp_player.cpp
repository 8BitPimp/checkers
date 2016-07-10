#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <memory>

#include "checkers.h"

struct tcp_player_t : public player_t
{
protected:
    SOCKET sock_;

public:
    tcp_player_t(colour_e colour, SOCKET socket_)
        : player_t(colour)
        , sock_(socket_)
    {
        assert(sock_ != INVALID_SOCKET);
    }

    virtual bool is_connected()
    {
        return sock_ != INVALID_SOCKET;
    }

    virtual bool poll_move(move_t & out)
    {
        return false;
    }

    virtual bool invalid_move(const move_t & move)
    {
        return false;
    }

    virtual bool send_move(const move_t & move)
    {
        std::string state;
        move.serialize(state);
        int ret = send(sock_, state.c_str(), state.length(), 0);
        return ret == state.length();
    }

    virtual bool send_board(const board_t & board)
    {
        std::string state;
        board.serialize(state);
        int ret = send(sock_, state.c_str(), state.length(), 0);
        return ret == state.length();
    }

    virtual bool send_colour(colour_e c)
    {
        const std::string colour[] = {
            "WHITE", "BLACK"
        };
        const std::string & opt = colour[c];
        int ret = send(sock_, opt.c_str(), opt.length(), 0);
        return ret == opt.length();
        return true;
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

struct tcp_factory_t::impl_t
{
    SOCKET ls_sock;
    WSADATA wsaData;

    impl_t()
        : ls_sock(INVALID_SOCKET)
    {
    }

    bool start()
    {
#if defined(_MSC_VER)
        // init winsock library
        if (WSAStartup(MAKEWORD(2, 2), &wsaData)!=NO_ERROR) {
            log("unable to init winsock");
            return false;
        }
#endif
        // create the listen socket
        ls_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ls_sock==INVALID_SOCKET) {
            log("unable to create the listen socket");
            return false;
        }

        // bind socket to interface
        sockaddr_in service;
        service.sin_family = AF_INET;
        if (inet_pton(AF_INET, "127.0.0.1", PVOID(&service.sin_addr.s_addr))!=1) {
            log("unable create on-wire descriptor");
        }
        service.sin_port = htons(1234);
        if (bind(ls_sock, (SOCKADDR *)& service, sizeof(service))==SOCKET_ERROR) {
            log("unable to bind the listen socket");
            return false;
        }

        // switch to listen state
        if (listen(ls_sock, 2)==SOCKET_ERROR) {
            log("unable to enter listen state");
            return false;
        }

        return true;
    }

    player_t * new_tcp_player(colour_e clr)
    {
        // accept a new connection
        SOCKET pl_sock = accept(ls_sock, nullptr, nullptr);
        if (pl_sock==INVALID_SOCKET) {
            log("bad client connect");
            return nullptr;
        }
        else {
            log("new connection");
        }
        // create the new player object
        return new tcp_player_t(clr, pl_sock);
    }

    bool stop()
    {
#if defined(_MSC_VER)
        WSACleanup();
#endif
        return true;
    }
};

tcp_factory_t::tcp_factory_t()
    : imp_(new tcp_factory_t::impl_t)
{
}

tcp_factory_t::~tcp_factory_t()
{
    delete imp_;
}

bool tcp_factory_t::start()
{
    return imp_->start();
}

bool tcp_factory_t::stop()
{
    return imp_->stop();
}

player_t * tcp_factory_t::new_tcp_player(colour_e clr)
{
    return imp_->new_tcp_player(clr);
}
