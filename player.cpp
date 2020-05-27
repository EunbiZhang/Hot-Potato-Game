#include "potato.h"

using namespace std;

int main(int argc, char *argv[]){
    if (argc != 3) {//corner case: syntax
        cout << "Syntax: player <machine_num> <port_num>\n" << endl;
        return 1;
    }
    int status;
    unsigned int myPort;
    int max_sd = 0;
    int server_socket_fd;
    int client_socket_fd;
    int left_client_socket_fd;
    int info[2] = {0, 0};//info[0] is the player_id, info[1] is the total players number
    int left_id = 0;//left player id
    int right_id = 0;//right player id
    int right_client_connection_fd;//right client connection fd
    //create a server socket for neighbors--------------------------------------------------------
    struct sockaddr_in servaddr;
    struct sockaddr_in servaddr_storage;
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        return -1;
    } 
    if(server_socket_fd > max_sd){max_sd = server_socket_fd;}
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(0);
    int yes = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(server_socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        return -1;
    }
    status = listen(server_socket_fd, 2);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        return -1;
    }
    socklen_t len = sizeof(servaddr_storage);
    getsockname(server_socket_fd, (struct sockaddr *)&servaddr_storage, &len);
    myPort = ntohs(servaddr_storage.sin_port);

    //create client socket to server------------------------------------------------------------------------  
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = argv[1];
    const char *port     = argv[2]; 
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    }
    client_socket_fd = socket(host_info_list->ai_family, 
                            host_info_list->ai_socktype, 
                            host_info_list->ai_protocol);
    if (client_socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    }
    if(client_socket_fd > max_sd){max_sd = client_socket_fd;}
    //connect to the server socket 
    status = connect(client_socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } 
    freeaddrinfo(host_info_list);
    if(send(client_socket_fd, &myPort, sizeof(myPort), 0) <= 0) { 
        cerr << "error: send" << endl; return -1; 
    }
    recv(client_socket_fd, info, sizeof(info), 0);
    cout << "Connected as player " << info[0] << " out of " << info[1] << " total players" << endl; 
    srand((unsigned int)time(NULL) + info[0]);
    //recv the port and ip of the neighbors from the ringmaster
    if(info[0] == 0){
        left_id = info[1] - 1;
        right_id = info[0] + 1;
    }
    else if(info[0] == info[1] - 1){
        left_id = info[0] - 1;
        right_id = 0;
    }
    else{
        left_id = info[0] - 1;
        right_id = info[0] + 1;
    }
    int leftPort;
    recv(client_socket_fd, &leftPort, sizeof(leftPort), 0);  
    char buffer[50];
    recv(client_socket_fd, buffer, 50, 0);
    const char* leftIP = buffer;

    //create client socket to the left neighbor-------------------------------------------------------------------
    struct sockaddr_in left_servaddr;
    left_client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (left_client_socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        return -1;
    } 
    if(left_client_socket_fd > max_sd){max_sd = left_client_socket_fd;}
    memset(&left_servaddr, 0, sizeof(left_servaddr));
    left_servaddr.sin_family = AF_INET;
    left_servaddr.sin_port = htons(leftPort);
    if(inet_pton(AF_INET, leftIP, &left_servaddr.sin_addr) <= 0){
        cerr << "error: server address" << endl;
        return -1;
    }

    //connect with the left neighbors and accept connection from right neighbor----------------------------------------------
    connect(left_client_socket_fd, (struct sockaddr *)&left_servaddr, sizeof(left_servaddr));  
    struct sockaddr_in right_socket_addr;
    socklen_t right_socket_addr_len = sizeof(right_socket_addr);
    right_client_connection_fd = accept(server_socket_fd, (struct sockaddr *)&right_socket_addr, &right_socket_addr_len);
    if (right_client_connection_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
    } //accept the connections with its right neighbor
    if(right_client_connection_fd > max_sd){max_sd = right_client_connection_fd;}

    //start the game---------------------------------------------------------------------------------------
    //cout << "start the game" << endl;
    int fd[2];
    fd[0] = right_client_connection_fd;
    fd[1] = left_client_socket_fd;
    fd_set readfds;
    potato_t potato;
    while(potato.hops > -1){
        FD_ZERO(&readfds);
        //add sockets to set
        FD_SET(right_client_connection_fd, &readfds);
        FD_SET(left_client_socket_fd, &readfds);
        FD_SET(client_socket_fd, &readfds);
        //wait for an activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0 ){
            cerr << "error: select" << endl;
            return -1;
        }
        //loop througn the connections to see what data to read
        if(FD_ISSET(client_socket_fd, &readfds)){//potato from ringmaster: shutdown or startplayer
            memset(&potato, 0, sizeof(potato));
            recv(client_socket_fd, &potato, sizeof(potato_t), 0);
            if(potato.isPotato == false){continue;}
            if(potato.hops == -1){
                break;//shut down
            }
            transfer_potato(potato, info[0]);
            if(potato.hops == 0){
                cout << "I'm it" << endl;
                potato.hops--;
                if(send(client_socket_fd, &potato, sizeof(potato_t), 0) <= 0) { //send to ringmaster
                    cerr << "error: send" << endl; return -1; 
                }
            }
            else{
                int random = rand() % 2; //send to neighbors
                if(random == 0){
                    //send the potato to left player
                    if(send(left_client_socket_fd, &potato, sizeof(potato_t), 0) <= 0) { 
                        cerr << "error: send" << endl; return -1;
                    }
                    cout << "sending potato to " << left_id << endl;
                }
                else{
                    //send the potato to right player
                    if(send(right_client_connection_fd, &potato, sizeof(potato_t), 0) <= 0) { 
                        cerr << "error: send" << endl; return -1; 
                    }
                    cout << "sending potato to " << right_id << endl;
                }
            }
        }
        else{//potato from players
            for(int i = 0; i < 2; ++i){
                if(FD_ISSET(fd[i], &readfds)){
                    memset(&potato, 0, sizeof(potato));
                    recv(fd[i], &potato, sizeof(potato_t), 0);
                    if(potato.isPotato == false){continue;}
                    transfer_potato(potato, info[0]);
                    if(potato.hops == 0){
                        cout << "I'm it" << endl;
                        potato.hops = -1;
                        if(send(client_socket_fd, &potato, sizeof(potato_t), 0) <= 0) { //send to ringmaster
                            cerr << "error: send" << endl; return -1; 
                        }
                    }
                    else{
                        int random = rand() % 2; //send to neighbors
                        if(random == 0){
                            //send the potato to left player
                            if(send(left_client_socket_fd, &potato, sizeof(potato_t), 0) <= 0) { 
                                cerr << "error: send" << endl; return -1;
                            }
                            cout << "sending potato to " << left_id << endl;
                        }
                        else{
                            //send the potato to right player
                            if(send(right_client_connection_fd, &potato, sizeof(potato_t), 0) <= 0) { 
                                cerr << "error: send" << endl; return -1; 
                            }
                            cout << "sending potato to " << right_id << endl;
                        }
                        
                    }
                    break;
                }
            }
        }
    }

    close(left_client_socket_fd);
    close(server_socket_fd);
    close(client_socket_fd);
    return 0;
}
