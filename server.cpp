#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>

#define MAXLEN 10000
#define H_BASE 131
#define H_MOD 11971
#define DICT_MOD 94
#define USER_INFO "password"
#define MSG_INFO "message"

typedef struct sockaddr_in SOCK_IN;

using namespace std;

int SERVER_PORT = 12000;

string dict = \
"~`_a0A,b1B.c2C!d3D?e4E:f5F;g6G'h7H\"i8I+j9J-kK*lL\\mM=nN(oO)pP[qQ]rR{sS}tT/uU|vV@wW#xX$yY%zZ^ &\n";

typedef struct User{
    char username[256];
    char passwd[256];
    bool operator == (const struct User& usr){
        return strcmp(username, usr.username) == 0;
    }
} User;

typedef struct {
    char from[256];
    char to[256];
    char msg[MAXLEN];
} Msg;

vector<User> userlist;

void init(){
    User usr;
    FILE* fd = fopen(USER_INFO, "rb");
    userlist.clear();
    while(!feof(fd)){
        if(fread(&usr, sizeof(usr), 1, fd) == 0)
            break;
        if(strlen(usr.username) == 0)
            continue;
        userlist.push_back(usr);
    }
    fclose(fd);
}

int getPrivateKey(string username){
    int hash = 0;
    for(int i = 0;i < username.length();i++)
        hash = (hash * H_BASE + username[i]) % H_MOD;
    return hash;
}

string decode(string str, int key){
    int len = str.length();
    char text[MAXLEN];
    strcpy(text, str.c_str());
    for(int i = 0;i < len;i++)
        text[i] = dict[(dict.find(text[i]) + key - i + DICT_MOD) % DICT_MOD];
    return string(text);
}

string encode(string str, int key){
    int len = str.length();
    char text[MAXLEN];
    strcpy(text, str.c_str());
    for(int i = 0;i < len;i++)
        text[i] = dict[(dict.find(text[i]) - key % DICT_MOD + i + DICT_MOD) % DICT_MOD];
    return string(text);
}

int login(string username, string passwd, int fd){
    User usr;
    strcpy(usr.username, username.c_str());
    int id = find(userlist.begin(), userlist.end(), usr) - userlist.begin();
    if(id == userlist.size() || strcmp(userlist[id].passwd, passwd.c_str()) != 0){
        send(fd, "ERR", 4, 0);
        return -1;
    }
    send(fd, "OK", 3, 0);
    cout << username << " just LOGIN" << endl;
    return 1;
}

void addUser(string username, string passwd){
    FILE* fd = fopen(USER_INFO, "ab");
    User usr;
    strcpy(usr.username, username.c_str());
    strcpy(usr.passwd, passwd.c_str());
    userlist.push_back(usr);
    fwrite(&usr, sizeof(usr), 1, fd);
    cout << username << " just REGISTER" << endl;
    fclose(fd);
}

void recall(string username, int sock){
    FILE* fd = fopen(MSG_INFO, "rb");
    Msg info;
    while(!feof(fd)){
        if(fread(&info, sizeof(info), 1, fd) == 0)
            break;
        if(strcmp(info.to, username.c_str()) == 0){
            string contain = info.from + string("\n") + info.msg;
            send(sock, contain.c_str(), MAXLEN, 0);
        }
    }
    send(sock, "THATSALL", 9, 0);
    fclose(fd);
}

int regis(string username, string passwd, int fd){
    User usr;
    if(find(userlist.begin(), userlist.end(), usr) != userlist.end()){
        send(fd, "EXIST", 6, 0);
        return -1;
    }
    send(fd, "OK", 3, 0);
    addUser(username, passwd);
    return 1;
}

void store(string from, string to, string msg){
    FILE* fd = fopen(MSG_INFO, "ab");
    Msg info;
    strcpy(info.from, from.c_str());
    strcpy(info.to, to.c_str());
    msg = decode(msg, getPrivateKey(from));
    msg = encode(msg, getPrivateKey(to));
    strcpy(info.msg, msg.c_str());
    fwrite(&info, sizeof(info), 1, fd);
    fflush(fd);
    fclose(fd);
}

int createSocket(SOCK_IN& server_addr){
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return sockfd;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero), 8);
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("bind");
        return -1;
    }
    return sockfd;
}

string allUsers(){
    if(userlist.size() == 0)
        return string("");
    string names = userlist[0].username;
    for(int i = 1;i < userlist.size();i++){
        names += string("@");
        names += userlist[i].username;
    }
    return names;
}

int main(int argc, char** argv){
    if(argc == 2)
        SERVER_PORT = atoi(argv[1]);
    init();
    SOCK_IN server_addr;
    SOCK_IN client_addr;
    int sockfd, clientfd;
    int socksize = sizeof(struct sockaddr_in);
    string option;
    char text[MAXLEN];
    srand(time(NULL));
    sockfd = createSocket(server_addr);
    listen(sockfd, 10);
    while(true){
        cout << ">>> Listening <<<" << endl;
        clientfd = accept(sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&socksize);
        if(clientfd < 0)    continue;
        send(clientfd, allUsers().c_str(), MAXLEN, 0);
    WAIT:
        memset(text, 0, MAXLEN);
        recv(clientfd, text, MAXLEN, 0);
        option = string(text);
        if(option == "LOGIN"){
            recv(clientfd, text, MAXLEN, 0);
            int split = 0;
            while(text[split] != '@')
                split++;
            text[split++] = 0;
            string username = string(text);
            string passwd = string(text + split);
            if(login(username, passwd, clientfd) > 0)
                recall(username, clientfd);
            goto WAIT;
        }
        else if(option == "REGIS"){
            recv(clientfd, text, MAXLEN, 0);
            int split = 0;
            while(text[split] != '@')
                split++;
            text[split++] = 0;
            string username = string(text);
            string passwd = string(text + split);
            regis(username, passwd, clientfd);
            recall(username, clientfd);
            goto WAIT;
        }
        else if(option == "MSG"){
            recv(clientfd, text, MAXLEN, 0);
            int split = 0;
            while(text[split] != '@')
                split++;
            text[split++] = 0;
            string from = string(text + split);
            string to = string(text);
            recv(clientfd, text, MAXLEN, 0);
            store(from, to, string(text));
        }
        else if(option == "BYE")
            close(clientfd);
    }
    close(sockfd);
}
