#include "potato.h"

using namespace std;

int main(int argc, char *argv[]){
    if (argc != 4) {
        cout << "Syntax: ringmaster <port_num> <num_players> <num_hops>\n" << endl;
        return 1;
    }
    //get start info and create the potato
    int players = atoi(argv[2]);
    int hops = atoi(argv[3]);
    if(players <= 1){
        cout << "Syntax: num_players must be greater than 1\n" << endl;
        return 1;
    }
    if(hops < 0 || hops > 512){
        cout << "Syntax: num_hops must be greater than or equal to zero and less than or equal to 512\n" << endl;
        return 1;
    }
    potato_t potato;
    mkpotato(players, hops, potato);

    //global variables store game info
    vector<int> players_socket(players);//store the players' client socket fd to the ringmaster
    vector<int> players_port(players);//store the players' port number
    vector<string> players_IP(players);//store the players' IP address
    cout << "Potato Ringmaster" << endl;
    cout << "Players = " << players << endl;
    cout << "Hops = " << hops << endl;

    //create the server socket-----------------------------------------------------------------------------
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port     = argv[1];
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    }
    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } 

    //bind
    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } 

    //listen
    status = listen(socket_fd, players);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } 

    //get all the players ready
    while(players_socket[players-1] == 0){
        struct sockaddr_in socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd;
        client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (client_connection_fd == -1) {
            cerr << "Error: cannot accept connection on socket" << endl;
            return -1;
        } //accept the connections   

        for(int i = 0; i < players; ++i){//add the player into game info
            if(players_socket[i] == 0){
                players_socket[i] = client_connection_fd;
                players_IP[i] = inet_ntoa(socket_addr.sin_addr);
                recv(client_connection_fd, &players_port[i], sizeof(players_port[i]), 0); 
                //send new connection greeting message: info{player_id, total player num}
                int info[2] = {i, players};
                if(send(client_connection_fd, info, sizeof(info), 0) <= 0) { 
                    cerr << "error: send" << endl; return -1;
                } 
                cout << "Player " << i << " is ready to play" << endl;
                break;
            }
        }
    }
    //send the neighbors' port number and IP to every player
    for(int i = 0; i < players; ++i){
        int left_id = 0;
        if(i == 0){
            left_id = players - 1;
        }
        else if(i == players - 1){
            left_id = i - 1;
        }
        else{
            left_id = i - 1;
        }
        send(players_socket[i], &players_port[left_id], sizeof(players_port[left_id]), 0);
        const char * leftIP = players_IP[left_id].c_str();
        char buffer[50];
        strncpy(buffer, leftIP, 50);
        buffer[strlen(leftIP)] = '\0';
        send(players_socket[i], buffer, sizeof(buffer), 0);
    }

    //start the game by sending potato to a random player------------------------------------------------
    //---------------------------------------------------------------------------------------------------
    if(hops == 0){
        potato.hops--;//let all the players shut down
        for(int i = 0; i < players; ++i){
            if(send(players_socket[i], &potato, sizeof(potato_t), 0) <= 0) { 
                cerr << "error: send" << endl; return -1; 
            }
        }
    }
    else{
        srand((unsigned int)time(NULL));
        int random = rand() % players;
        if(send(players_socket[random], &potato, sizeof(potato_t), 0) <= 0) { 
            cerr << "error: send" << endl; return -1; 
        }
        cout << "Ready to start the game, sending potato to player " << random << endl;
        cout << "Trace of potato:" << endl;
        
        //start the game
        int max_sd = 0;
        fd_set readfds;  
        FD_ZERO(&readfds);
        //add sockets to set
        for(int i = 0; i < players; ++i){
            if(players_socket[i] > 0){
                FD_SET(players_socket[i], &readfds);
            }
            if(players_socket[i] > max_sd){
                max_sd = players_socket[i];
            }
        }
        //wait for an activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0 ){
            cerr << "error: select" << endl;
            return -1;
        }
        //loop througn the connections to see what data to read
        for(int i = 0; i <= max_sd; ++i){
            if(FD_ISSET(i, &readfds)){
                recv(i, &potato, sizeof(potato_t), 0);
                if(potato.isPotato == false){continue;}
                if(potato.hops == -1){
                    //shutdown
                    for(int i = 0; i < players; ++i){
                        if(send(players_socket[i], &potato, sizeof(potato_t), 0) <= 0) { 
                            cerr << "send" << endl; 
                        }
                    }
                    cout << potato.player_id[0];
                    for(int j = 1; j < potato.cnt; ++j){
                        cout << ", " << potato.player_id[j];
                    }
                    cout << endl;
                }
                break;
            }
        }
          
    }

    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
}