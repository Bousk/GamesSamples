#include <main.hpp>

#include <string>

extern int main_p2p(bool isHost);
extern int main_tcp(bool isServer);
extern int main_solo();

enum class MainType
{
    Unknown,
    Solo,
    P2P_Host,
    P2P_Client,
};
int SDL_main(int argc, char* argv[])
{
    MainType type = MainType::Unknown;
    for (int i = 0; i < argc; ++i)
    {
        const std::string arg(argv[i]);
        if (arg == "-solo")
        {
            type = MainType::Solo;
            break;
        }
        else if (arg == "-p2p:host")
        {
            type = MainType::P2P_Host;
            break;
        }
        else if (arg == "-p2p:client")
        {
            type = MainType::P2P_Client;
            break;
        }
    }
    switch (type)
    {
        case MainType::Solo: return main_solo();
        case MainType::P2P_Host: return main_p2p(true);
        case MainType::P2P_Client: return main_p2p(false);
        case MainType::Unknown: return -1;
    }
}