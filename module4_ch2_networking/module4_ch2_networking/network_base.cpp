#include <enet/enet.h>
#include <iostream>

//#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "Winmm.lib")

using namespace std;

ENetAddress t_address;
ENetHost* t_server;

ENetHost* t_client;


bool tCreateServer()
{
    // ------------ Server Setup ------------

    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    t_address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    t_address.port = 1234;
    t_server = enet_host_create(&t_address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);

    return t_server != nullptr;
}
bool tCreateClient()
{
    t_client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);

    return t_client != nullptr;
}

int test_base(int argc, char** argv)
{
    int UserInput;

    if (enet_initialize() != 0)
    {
        //fprintf(stderr, "An error occurred while initializing ENet.\n");
        cout << "n error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    cout << "1) Create Server" << endl;
    cout << "2) Create Client" << endl;
    cin >> UserInput;
    if (UserInput == 1)
    {
        cout << "Creating Server..." << endl;

        if (!tCreateServer())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet server host.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (UserInput == 2)
    {
        cout << "Creating Client..." << endl;

        if (!tCreateClient())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet client host.\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cout << "Invalud Input..." << endl;
    }

    if (t_server != nullptr)
    {
        enet_host_destroy(t_server);
    }
    if (t_client != nullptr)
    {
        enet_host_destroy(t_client);
    }

    return EXIT_SUCCESS;
}