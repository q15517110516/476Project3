#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>

#define MAXLEN 10000
#define CONST_MOD 94
#define H_BASE 131
#define H_MOD 11971

typedef struct sockaddr_in SOCK_IN;

using namespace std;

int RETRY = 3;

char SERVER_IP[16] = "127.0.0.1";
int SERVER_PORT = 12000;
string dict = \
"~`_a0A,b1B.c2C!d3D?e4E:f5F;g6G'h7H\"i8I+j9J-kK*lL\\mM=nN(oO)pP[qQ]rR{sS}tT/uU|vV@wW#xX$yY%zZ^ &\n";
vector<string> userlist;

int connect2server(SOCK_IN& serverAddr){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("Please check your network settings");
        return sockfd;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    if(connect(sockfd,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        perror("Connection failed");
        return -1;
    }
    return sockfd;
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
        text[i] = dict[(dict.find(text[i]) + key - i + CONST_MOD) % CONST_MOD];
    return string(text);
}

string encode(string str, int key){
    int len = str.length();
    char text[MAXLEN];
    strcpy(text, str.c_str());
    for(int i = 0;i < len;i++)
        text[i] = dict[(dict.find(text[i]) - key % CONST_MOD + i + CONST_MOD) % CONST_MOD];
    return string(text);
}

void parseUsers(string all){
    if(all.length() == 0)
        return;
    boost::split(userlist, all, [](char c){return c == '@';});
    cout << "Users:" << endl;
    for(int i = 0;i < userlist.size();i++){
        if(userlist[i] == "")   continue;
        cout << "    " << userlist[i];
    }
    cout << endl;
}

void parseAndPrint(string msg, int key){
    cout << endl << "#############################################" << endl;
    vector<string> results;
    boost::split(results, msg, [](char c){return c == '\n';});
    cout << "From: " << results[0] << endl;
    cout << "---------------" << endl;
    string contain = string();
    for(int i = 1;i < results.size();i++){
        contain += results[i];
        if(i != results.size() - 1)
            contain += string("\n");
    }
    cout << decode(contain, key) << endl;
    cout << "#############################################" << endl;
}

int login(string username, string passwd, int fd){
    char text[MAXLEN] = {0};
    int privateKey = getPrivateKey(username);
    send(fd, "LOGIN", 6, 0);
    string contain = username + string("@") + encode(passwd, privateKey);
    send(fd, contain.c_str(), MAXLEN, 0);
    recv(fd, text, MAXLEN, 0);
    if(strcmp(text, "OK") != 0)
        return -1;
    return privateKey;
}

int regis(string username, string passwd, int fd){
    char text[MAXLEN] = {0};
    int privateKey = getPrivateKey(username);
    send(fd, "REGIS", 6, 0);
    string contain = username + string("@") + encode(passwd, privateKey);
    send(fd, contain.c_str(), MAXLEN, 0);
    recv(fd, text, MAXLEN, 0);
    if(strcmp(text, "EXIST") == 0)
        return -1;
    return privateKey;
}

int main(int argc, char** argv){
    if(argc > 2){
        strcpy(SERVER_IP, argv[1]);
        SERVER_PORT = atoi(argv[2]);
    }
    SOCK_IN serverAddr;
    int sockfd, key = 27;
    char text[MAXLEN] = {0};
    while((sockfd = connect2server(serverAddr)) < 0 && RETRY--);
    if(sockfd < 0){
        printf("Connection overtime, please retry later\n");
        return 0;
    }
    recv(sockfd, text, MAXLEN, 0);
    string allUsers = string(text);
    parseUsers(allUsers);
    string username, passwd, chose;
    cout << "Login or Regist? [L/r] ";
    cout.flush();
    cin >> chose;
    if(chose == "R" || chose == "r")
        goto REGIS;
    goto LOGIN;
LOGIN:
    cout << endl << "username: ";
    cout.flush();
    cin >> username;
    cout << "password: ";
    cout.flush();
    cin >> passwd;
    if((key = login(username, passwd, sockfd)) < 0){
        printf("No existing user.\n");
        goto LOGIN;
    }
    goto HISTORY;
REGIS:
    cout << endl << "username: ";
    cout.flush();
    cin >> username;
    cout << "password: ";
    cout.flush();
    cin >> passwd;
    if((key = regis(username, passwd, sockfd)) < 0){
        printf("Username already exist\n");
        goto REGIS;
    }
HISTORY:
    cout << endl << "History messages: " << key << endl;
    cout << "-----------------" << endl;
    do{
        recv(sockfd, text, MAXLEN, 0);
        if(strcmp(text, "THATSALL") == 0)
            break;
        parseAndPrint(string(text), key);
        cout << endl;
    }while(true);
// SENDTO:
    string buf;
    cout << "Send message? [Y/n]";
    cout.flush();
    cin >> chose;
    if(chose == "n" || chose == "N"){
        send(sockfd, "BYE", 4, 0);
        close(sockfd);
        return 0;
    }
    send(sockfd, "MSG", 4, 0);
    cout << "To: ";
    cout.flush();
    cin >> buf;
    buf += string("@") + username;
    send(sockfd, buf.c_str(), MAXLEN, 0);
    buf = string();
    cout << "Message: (end with '##' in a new line)" << endl;
    do{
        cin.getline(text, MAXLEN);
        if(strcmp(text, "##") == 0) break;
        buf += string(text) + string("\n");
    }while(true);
    buf = encode(buf, key);
    send(sockfd, buf.c_str(), MAXLEN, 0);

    close(sockfd);
    return 0;
}