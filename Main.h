#pragma once

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <ctime>

#define GetLastError() WSAGetLastError()
#define BUFFER_MAXLENGTH 512

int main(int argc, char* argv[]);
int Init();
SOCKET CreateSocket(int protocol);
sockaddr_in* CreateAddress(char* ip, int port);
int Bind(SOCKET sock, sockaddr_in* addr);
int Send(SOCKET sock, char* buffer, int bytes, sockaddr_in* dest);
int Receive(SOCKET sock, char* buffer, int maxBytes);
int Connect(SOCKET sock, sockaddr_in* address);
void Update(SOCKET socket, const char* name);