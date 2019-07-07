#include <chrono>
#include <thread>
#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <sys/ioctl.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <netinet/ip.h>
#include<netdb.h>
#include<errno.h>

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

typedef int SOCKET;
#define NET_INVALID_SOCKET	-1
#define NET_SOCKET_ERROR -1
#define GetLastError() errno
#define closesocket close
#define ioctlsocket ioctl
#define SD_SEND 0x01
#define NO_ERROR 0L

#define MAXSIZE 1024
#define URL_MAXSIZE 128
#define RESULT_MAXSIZE 1000000

SOCKET CreateSocket(int protocol)
{
    SOCKET result = NET_INVALID_SOCKET;
    int type = SOCK_DGRAM;
    if (protocol == IPPROTO_TCP)
        type = SOCK_STREAM;

    result = socket(AF_INET, type, protocol);
    if (result != NET_INVALID_SOCKET && protocol == IPPROTO_TCP)
    {
        u_long mode = 1;
        int err = ioctlsocket(result, FIONBIO, &mode);
        if (err != NO_ERROR)
        {
            closesocket(result);
            result = NET_INVALID_SOCKET;
        }
    }
    return result;
}

sockaddr_in* CreateAddress(char* ip, int port)
{
    sockaddr_in* myAddr = (sockaddr_in*)std::calloc(sizeof(*myAddr), 1);
    myAddr->sin_family = AF_INET;
    myAddr->sin_port = htons(u_short(port));
	
    if (ip == NULL)
        myAddr->sin_addr.s_addr = INADDR_ANY;
    else
	{
		myAddr->sin_addr.s_addr = inet_addr(ip);
		if (myAddr->sin_addr.s_addr == INADDR_NONE)
		{
			struct addrinfo addrInfo;
			struct addrinfo* pAddrInfo;
			memset(&addrInfo, 0, sizeof(struct addrinfo));
			addrInfo.ai_family = AF_INET;
			if (getaddrinfo(ip, NULL, &addrInfo, &pAddrInfo) != 0)
				return NULL;
			else
			{
				myAddr->sin_addr = ((struct sockaddr_in*)pAddrInfo->ai_addr)->sin_addr;
				freeaddrinfo(pAddrInfo);
			}
		}
	}
    return myAddr;
}

int Connect(SOCKET sock, sockaddr_in* address)
{
    if (connect(sock, (sockaddr*)address, sizeof(sockaddr_in)) == NET_SOCKET_ERROR)
        return GetLastError();
    else
        return 0;
}

int ReceiveTCP(SOCKET sock, char* buffer, int maxBytes)
{
    int bytes = recv(sock, buffer, maxBytes, 0);
    if (bytes == NET_SOCKET_ERROR)
        return -1;
    return bytes;
}

int SendTCP(SOCKET sock, char* buffer, int bytes)
{
    int result = send(sock, buffer, bytes, 0);
    if (result == NET_SOCKET_ERROR)
        return -1;
    else
        return result;
}

void Close(SOCKET sock, bool now)
{
    if (now)
        closesocket(sock);
    else
        shutdown(sock, SD_SEND);
}

int Listen(SOCKET sock, int backlog)
{
	int max = backlog;
	if (max < 1)
		max = SOMAXCONN;

	int result = listen(sock, max);
	if (result == NET_SOCKET_ERROR)
		return GetLastError();

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "You need more arguments!" << std::endl;
		return 0;
	}

    struct sockaddr_in cli_addr, serv_addr;
    int sockfd, acceptSocket;
	
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bzero((char*)&cli_addr, sizeof(cli_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
	{
		std::cout << "Error code : " << GetLastError() << std::endl;
	}

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		std::cout << "Error code : " << GetLastError() << std::endl;
	}

	if(int ListenResult = Listen(sockfd, 10))
	{
		std::cout << "Error code : " << ListenResult << std::endl;
	}
	
	unsigned int length = sizeof(cli_addr);
	pid_t pid;

	bool endPoint = false;
	while(endPoint == false)
	{
		acceptSocket = accept(sockfd, (struct sockaddr*)&cli_addr, &length);

		if (acceptSocket < 0)
		{
			std::cout << "Error code : " << GetLastError() << std::endl;
		}

		pid = fork();
		if (pid == 0)
		{
			//std::cout << "PID: " << pid << std::endl;
			char buffer[MAXSIZE];
			bzero((char*)buffer, MAXSIZE);
			recv(acceptSocket, buffer, MAXSIZE, 0);

			// if(int(strlen(buffer)) != 0)
				// std::cout << "Buffer: " << buffer << std::endl;
			
			std::string receiver(buffer);
			auto getPos = receiver.find("GET ");
			auto HTTPPos = receiver.find("HTTP/1.1\r\n");
			auto URLPos = receiver.find("Host: ");
			auto endPos = receiver.find("\r\n\r\n");
			if (getPos >= 0 && HTTPPos >= 0 && URLPos >= 0 && endPos >= 0
			&& MAXSIZE > getPos && MAXSIZE > HTTPPos && MAXSIZE > URLPos && MAXSIZE > endPos )
			{
				std::string url = receiver.substr(URLPos + strlen("Host: "), endPos - URLPos - strlen("Host: "));
				std::string subDomain = receiver.substr(strlen("GET "), HTTPPos - strlen("GET "));
				
				char fullURL[MAXSIZE];
				memset(fullURL, '\0', MAXSIZE);
				//strcat(fullURL, "http://");
				strcat(fullURL, url.c_str());
				//strcat(fullURL, subDomain.c_str());
				//std::cout << "URL: \n" << fullURL << std::endl;
				
				std::string update;
				
				// Assignment 3 part: Proxy - Destination
				SOCKET tcpSocket = CreateSocket(IPPROTO_TCP);
				if(tcpSocket == NET_INVALID_SOCKET)
				{
					printf("Error Create Socket\n");
				}
				//printf("Create Socket\n");
				sockaddr_in* serverAddress = CreateAddress(fullURL, 80);
				
				if(serverAddress == NULL)
				{
					printf("Error Create Address\n");
				}
				//printf("Create Address\n");
				while (int connectResult = Connect(tcpSocket, serverAddress))
				{

					if (connectResult == 106)
						break;
					else if (connectResult != 114 && connectResult != 115)
					{
						update = std::string("Error code : " + connectResult);
					}
				}
				//printf("Connected! Send the request.\n");
				
				while (int sendResult = SendTCP(tcpSocket, buffer, strlen(buffer)) != (int)strlen(buffer))
				{
					if (sendResult == NET_SOCKET_ERROR)
					{
						update =  std::string("Error code : " + GetLastError());
					}
				}
				//Close(tcpSocket, false);
				
				// std::cout << "Sent Message: \n" << std::endl;
				// std::cout << buffer << std::endl;

				int numMsg = 0;
				char msg[RESULT_MAXSIZE] = { 0 };
				memset(buffer, '\0', MAXSIZE); //needs to clear out with \0

				// Listen for data with recv() until it returns 0
				int recvResult = -1;
				int loading = 0;
				while (recvResult != 0)
				{
					recvResult = ReceiveTCP(tcpSocket, buffer, MAXSIZE);
					//std::cout << "recvResult: " << recvResult << std::endl;
					if (recvResult == -1)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						loading++;
						if(loading > 10)
							recvResult = 0;
						
						if (GetLastError() != EWOULDBLOCK)
						{
							std::cout << "Error Code: " << GetLastError() << std::endl;
							recvResult = 0;
						}

					}
					else
					{
						bool check = false;
						for (int i = 0; i < MAXSIZE; ++i)
						{
							if (numMsg >= RESULT_MAXSIZE)
							{
								check = true;
								break;
							}

							if (*(buffer + i) == '\0')
								break;

							msg[numMsg++] = *(buffer + i);
						}

						if (check == true)
							break;
					}
					memset(buffer, '\0', MAXSIZE);
				}
				Close(tcpSocket, true);
				memset(buffer, '\0', MAXSIZE);				
				free(serverAddress);
				update = std::string(msg);
				
				std::cout << "Response from the Server: \n" << msg << std::endl;
				// Send the result
				// while (int sendResult = SendTCP(acceptSocket, msg, strlen(msg)) != (int)strlen(msg))
				// {
					// std::cout << "Send Result: " << sendResult << std::endl;
					// if (sendResult == NET_SOCKET_ERROR)
					// {
						// std::cout << "Error Code: " << GetLastError() << std::endl;
						// break;
					// }
				// }
				int r = send(acceptSocket, update.c_str(), strlen(update.c_str()), 0);
				std::cout << "Send Result: " << r << std::endl;
				memset(msg, '\0', MAXSIZE); //needs to clear out with \0
			}
			else
			{
				send(acceptSocket, "400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED", 18, 0);
			}
			close(acceptSocket);
			close(sockfd);
			//endPoint = true;
			_exit(0);
		}
		else
		{
			close(acceptSocket);
		}
	}
    return 0;
}
