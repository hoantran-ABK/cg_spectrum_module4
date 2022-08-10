#include <enet/enet.h>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

ENetHost* NetHost = nullptr;
ENetPeer* Peer = nullptr;
bool IsServer = false;
thread* PacketThread = nullptr;

int serverValue = 0;
bool clientWaitingForResponse = false;

bool s_guessWasCorrect = false;
bool c_guessWasCorrect = false;

enum PacketHeaderTypes
{
    PHT_Invalid = 0,
    PHT_IsDead,
    PHT_Position,
    PHT_Count,
    PHT_Guess,
    PHT_Result
};

struct GamePacket
{
    GamePacket() {}
    PacketHeaderTypes Type = PHT_Invalid;
};

struct IsDeadPacket : public GamePacket
{
    IsDeadPacket()
    {
        Type = PHT_IsDead;
    }

    int playerId = 0;
    bool IsDead = false;
};

struct PositionPacket : public GamePacket
{
    PositionPacket()
    {
        Type = PHT_Position;
    }

    int playerId = 0;
    int x = 0;
    int y = 0;
};

struct GuessPacket : public GamePacket
{
    GuessPacket()
    {
        Type = PHT_Guess;
    }

    GuessPacket(int g)
    {
        Type = PHT_Guess;
        this->guess = g;
    }

    int guess = 0;
};

struct ResultPacket : public GamePacket
{
    ResultPacket()
    {
        Type = PHT_Result;
    }

    ResultPacket(bool c)//, int r)
    {
        Type = PHT_Result;
        this->isCorrect = c;
        //this->result = r;
    }

    bool isCorrect = false;
    //int result = 0;
};

//can pass in a peer connection if wanting to limit
bool CreateServer()
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    NetHost = enet_host_create(&address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);

    return NetHost != nullptr;
}

bool CreateClient()
{
    NetHost = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);

    return NetHost != nullptr;
}

bool AttemptConnectToServer()
{
    ENetAddress address;
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    Peer = enet_host_connect(NetHost, &address, 2, 0);
    return Peer != nullptr;
}

void HandleReceivePacket(const ENetEvent& event)
{
    GamePacket* RecGamePacket = (GamePacket*)(event.packet->data);
    if (RecGamePacket)
    {
        cout << "Received Game Packet " << endl;

        if (RecGamePacket->Type == PHT_IsDead)
        {
            cout << "u dead?" << endl;
            IsDeadPacket* DeadGamePacket = (IsDeadPacket*)(event.packet->data);
            if (DeadGamePacket)
            {
                string response = (DeadGamePacket->IsDead ? "yeah" : "no");
                cout << response << endl;
            }
        }

        if (RecGamePacket->Type == PHT_Guess)
        {
            // handle guess, implied to be client->server direction packet (server is handling this packet now)
            GuessPacket* clientGuess = (GuessPacket*)(event.packet->data);
            if (clientGuess)
            {
                cout << "Client's Guess: " << clientGuess->guess << endl;

                if (clientGuess->guess == serverValue)
                {
                    cout << "Client's Guess was right!" << endl;
                    s_guessWasCorrect = true;
                }
                else
                {
                    cout << "Client's Guess was incorect..." << endl;
                    s_guessWasCorrect = false;
                }
            }
        }

        if (RecGamePacket->Type == PHT_Result)
        {
            // handle results returned, implied to be server->client direction (client is handling this packet now)
            // ...
            ResultPacket* serverResponse = (ResultPacket*)(event.packet->data);
            if (serverResponse)
            {
                //cout << "Server's Response to my guess: " << serverResponse->isCorrect << endl;//<< ", " << serverResponse->result << endl;
                
                if (serverResponse->isCorrect)
                {
                    //cout << "Your guess was correct!" << serverValue << endl;
                    c_guessWasCorrect = true;

                }
                else
                {
                    cout << "Your guess was incorrect. Please try again... " << endl;
                    c_guessWasCorrect = false;
                }
            }
            
            // check if result is correct or not here

            // make sure client is not looking for a response anymore after handling it
            clientWaitingForResponse = false;
        }

    }
    else
    {
        cout << "Invalid Packet " << endl;
    }

    /* Clean up the packet now that we're done using it. */
    enet_packet_destroy(event.packet);
    {
        enet_host_flush(NetHost);
    }
}

void BroadcastIsDeadPacket()
{
    IsDeadPacket* DeadPacket = new IsDeadPacket();
    DeadPacket->IsDead = true;
    ENetPacket* packet = enet_packet_create(DeadPacket,
        sizeof(IsDeadPacket),
        ENET_PACKET_FLAG_RELIABLE);

    /* One could also broadcast the packet by         */
    enet_host_broadcast(NetHost, 0, packet);
    //enet_peer_send(event.peer, 0, packet);

    /* One could just use enet_host_service() instead. */
    //enet_host_service();
    enet_host_flush(NetHost);
    delete DeadPacket;
}

void ServerProcessPackets()
{
    srand(time(0));
    int randomGen = rand() % 100;
    serverValue = randomGen;
    cout << "Random Server Number To Guess: " << serverValue << endl;

    while (1)
    {
        ENetEvent event;
        while (enet_host_service(NetHost, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                cout << "A new client connected from "
                    << event.peer->address.host
                    << ":" << event.peer->address.port
                    << endl;
                /* Store any relevant client information here. */
                event.peer->data = (void*)("Client information");
                BroadcastIsDeadPacket();
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                HandleReceivePacket(event);
                // send actual response packet here?
                {
                    ResultPacket* resultPacket = new ResultPacket();
                    resultPacket->isCorrect = s_guessWasCorrect;
                    ENetPacket* packet = enet_packet_create(resultPacket,
                        sizeof(ResultPacket),
                        ENET_PACKET_FLAG_RELIABLE);

                    enet_peer_send(event.peer, 0, packet);
                    enet_host_flush(NetHost);

                    enet_packet_destroy(packet);
                }
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                cout << (char*)event.peer->data << "disconnected." << endl;
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                //notify remaining player that the game is done due to player leaving
            }
        }
    }
}

void ClientProcessPackets()
{
    clientWaitingForResponse = false;

    while (1)
    {
        ENetEvent event;
        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(NetHost, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case  ENET_EVENT_TYPE_CONNECT:
                cout << "Connection succeeded " << endl;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                HandleReceivePacket(event);
                break;
            }
        }

        // check if a response needs to be received first
        if (clientWaitingForResponse)
        {
            cout << "Waiting for response from server about my guess..." << endl;
            continue;
        }
        else // if not waiting on a response
        {
            // asking for input OR end loop because the previous guess was correct
            if (c_guessWasCorrect)
            {
                int ex;
                cout << "Your guess was correct! Enter any key to continue" << endl;
                cin >> ex;
                break;
            }
            else
            {
                int clientGuess;
                // get input as a guess
                cout << "Enter a guess: ";
                cin >> clientGuess;

                GuessPacket* guessPacket = new GuessPacket();
                guessPacket->guess = clientGuess;
                ENetPacket* packet = enet_packet_create(guessPacket,
                    sizeof(GuessPacket),
                    ENET_PACKET_FLAG_RELIABLE);

                enet_peer_send(Peer, 0, packet);
                enet_host_flush(NetHost);

                enet_packet_destroy(packet);
            }
        }
    }
}

int main(int argc, char** argv)
{
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        cout << "An error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    cout << "1) Create Server " << endl;
    cout << "2) Create Client " << endl;
    int UserInput;
    cin >> UserInput;
    if (UserInput == 1)
    {
        //How many players?

        if (!CreateServer())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet server.\n");
            exit(EXIT_FAILURE);
        }

        IsServer = true;
        cout << "waiting for players to join..." << endl;
        PacketThread = new thread(ServerProcessPackets);
    }
    else if (UserInput == 2)
    {
        if (!CreateClient())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet client host.\n");
            exit(EXIT_FAILURE);
        }

        ENetEvent event;
        if (!AttemptConnectToServer())
        {
            fprintf(stderr,
                "No available peers for initiating an ENet connection.\n");
            exit(EXIT_FAILURE);
        }

        PacketThread = new thread(ClientProcessPackets);

        //handle possible connection failure
        {
            //enet_peer_reset(Peer);
            //cout << "Connection to 127.0.0.1:1234 failed." << endl;
        }
    }
    else
    {
        cout << "Invalid Input" << endl;
    }
    

    if (PacketThread)
    {
        PacketThread->join();
    }
    delete PacketThread;
    if (NetHost != nullptr)
    {
        enet_host_destroy(NetHost);
    }
    
    return EXIT_SUCCESS;
}