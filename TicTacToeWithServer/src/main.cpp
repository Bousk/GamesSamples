#include <main.hpp>

#include <NetService.hpp>

#include <string>

extern int main_game(NetService::NetworkType);

int SDL_main(int argc, char* argv[])
{
    NetService::NetworkType networkType = NetService::NetworkType::None;
    for (int i = 0; i < argc; ++i)
    {
        const std::string arg(argv[i]);
        if (arg == "-solo")
        {
            networkType = NetService::NetworkType::None;
            break;
        }
        else if (arg == "-client")
        {
            networkType = NetService::NetworkType::Client;
            break;
        }
        else if (arg == "-host")
        {
            networkType = NetService::NetworkType::Host;
            break;
        }
        else if (arg == "-server")
        {
            networkType = NetService::NetworkType::DedicatedServer;
            break;
        }
    }
    return main_game(networkType);
}