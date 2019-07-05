//gcc -o CS260Assignment2.exe main.cpp -std=gnu++11

//#include <iostream>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>

#define GetLastError() WSAGetLastError()

#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

typedef int SOCKET;

#define GetLastError() errno
#define closesocket close

#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define NO_ERROR (0L)
#define WSACleanup() ;
#define SD_SEND 0x01

#define ioctlsocket ioctl

#endif

#define BUFFER_MAXLENGTH 512
#define URL_MAXLENGTH 128
#define RESULT_MAXLENGTH 4096

int Init()
{
#ifdef _WIN32
	WSADATA wsa;

	return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    return 0;
#endif
}

SOCKET CreateSocket(int protocol)
{
	SOCKET result = INVALID_SOCKET;
	int type = SOCK_DGRAM;
	if (protocol == IPPROTO_TCP)
		type = SOCK_STREAM;

	result = socket(AF_INET, type, protocol);
	if (result != INVALID_SOCKET && protocol == IPPROTO_TCP)
	{
		u_long mode = 1;
		int err = ioctlsocket(result, FIONBIO, &mode);
		if (err != NO_ERROR)
		{
			closesocket(result);
			result = INVALID_SOCKET;
		}
	}
	return result;
}

sockaddr_in* CreateAddress(char* ip, int port)
{
	sockaddr_in* result = (sockaddr_in*)calloc(sizeof(*result), 1);

	result->sin_family = AF_INET;
	result->sin_port = htons(port);

	if (ip == NULL)
#ifdef _WIN32
		result->sin_addr.S_un.S_addr = INADDR_ANY;
	else
	{
		result->sin_addr.S_un.S_addr = inet_addr(ip);
		if (result->sin_addr.S_un.S_addr == INADDR_NONE)
		{
			ADDRINFOA hint;
			PADDRINFOA ppAddrs;
			memset(&hint, 0, sizeof(ADDRINFOA));
			hint.ai_family = AF_INET;
			int err = getaddrinfo(ip, NULL, &hint, &ppAddrs);
			if (err != 0)
				return NULL;
			else
			{
				result->sin_addr = ((SOCKADDR_IN*)ppAddrs->ai_addr)->sin_addr;
				freeaddrinfo(ppAddrs);
			}
		}
	}
#else
		result->sin_addr.s_addr = INADDR_ANY;
	else
	{
		result->sin_addr.s_addr = inet_addr(ip);
		if (result->sin_addr.s_addr == INADDR_NONE)
		{
			struct addrinfo hint;
			struct addrinfo* ppAddrs;
			memset(&hint, 0, sizeof(struct addrinfo));
			hint.ai_family = AF_INET;
			int err = getaddrinfo(ip, NULL, &hint, &ppAddrs);
			if (err != 0)
				return NULL;
			else
			{
				result->sin_addr = ((struct sockaddr_in*)ppAddrs->ai_addr)->sin_addr;
				freeaddrinfo(ppAddrs);
			}
		}
	}
#endif
	
	return result;
	// Caller will be responsible for free()
}

int Connect(SOCKET sock, sockaddr_in* address)
{
	if (connect(sock, (sockaddr*)address, sizeof(sockaddr_in)) == SOCKET_ERROR)
		return GetLastError();
	else
		return 0;
}

int SendTCP(SOCKET sock, char* buffer, int bytes)
{
	int result = send(sock, buffer, bytes, 0);
	if (result == SOCKET_ERROR)
		return -1;
	else
		return result;
}

int ReceiveTCP(SOCKET sock, char* buffer, int maxBytes)
{
	int bytes = recv(sock, buffer, maxBytes, 0);
	if (bytes == SOCKET_ERROR)
		return -1;
	return bytes;
}

void Close(SOCKET sock, bool now)
{
	if (now)
		closesocket(sock);
	else
		shutdown(sock, SD_SEND);
}

void Update(SOCKET socket, const char* URL)
{
	//changing URL into Hostname
	int slashCount = 0;
	int host = 0;
	int command = 0;
	char hostname[URL_MAXLENGTH] = { 0 };
	char getCommand[URL_MAXLENGTH] = { 0 };
	for (int i = 0; URL[i] != '\0'; ++i)
	{
		switch (slashCount)
		{
		case 2:
			if (URL[i] != '/')
				hostname[host++] = URL[i];

		default:
			if (URL[i] == '/')
				++slashCount;
			break;

		case 3:
			getCommand[command++] = URL[i];
		}	
	}
	printf("host: %s\n", hostname);

	sockaddr_in* serverAddress = CreateAddress(hostname, 80);

	//checking address
	const int len = 20;
	char buf[len];

	inet_ntop(AF_INET, &(serverAddress->sin_addr), buf, len);
	printf("address: %s\n", buf);
	//

	while (int err = Connect(socket, serverAddress))
	{
#ifdef _WIN32
		if (err == WSAEISCONN)
			break;
		if (err == INVALID_SOCKET)
		{
			printf("error! code : %i\n", err);
			return;
		}
#else
		if (err == 106)
			break;
		else if (err != 114 && err != 115)
		{
			printf("error! code : %i\n", err);
			return;
		}
#endif
	}
	printf("connected! sending request...\n");

	//request pool
	char buffer[BUFFER_MAXLENGTH];
	memset(buffer, '\0', BUFFER_MAXLENGTH); //needs to clear out with \0
#ifdef _WIN32
	strcat_s(buffer, "GET /");
	strcat_s(buffer, getCommand);
	strcat_s(buffer, " HTTP/1.1\nHost: ");
	strcat_s(buffer, hostname);
	strcat_s(buffer, "\n\n");
#else
	strcat(buffer, "GET /");
	strcat(buffer, getCommand);
	strcat(buffer, " HTTP/1.1\nHost: ");
	strcat(buffer, hostname);
	strcat(buffer, "\n\n");
#endif

	while (int sendResult = SendTCP(socket, buffer, strlen(buffer)) != (int)strlen(buffer))
		if (sendResult == SOCKET_ERROR)
		{
			printf("send error! code : %i\n", GetLastError());
			return;
		}
	Close(socket, false);

	printf("\nsent message:\n");
	printf("%s",buffer);
	printf("\n");


	int rcvMsgIter = 0;
	char receivedMsg[RESULT_MAXLENGTH] = { 0 };
	memset(buffer, '\0', BUFFER_MAXLENGTH); //needs to clear out with \0
	while (int recvResult = ReceiveTCP(socket, buffer, BUFFER_MAXLENGTH) != 0)
	{
		if (recvResult == SOCKET_ERROR)
		{
			printf("receive error! code : %i\n", GetLastError());
			return;
		}

		bool finished = false;
		for (int i = 0; i < BUFFER_MAXLENGTH; ++i)
		{
			if (rcvMsgIter >= RESULT_MAXLENGTH)
			{
				finished = true;
				break;
			}

			if (*(buffer + i) == '\0')
				break;

			receivedMsg[rcvMsgIter++] = *(buffer + i);

			if (*(buffer + i) == ']')
			{
				finished = true;
				break;
			}
		}

		if (finished)
			break;

		memset(buffer, '\0', BUFFER_MAXLENGTH);
	}

	Close(socket, true);

    printf("%s\n", receivedMsg);

	free(serverAddress);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("not enough arguments!\n");
		return 0;
	}

	if (Init() == SOCKET_ERROR)
	{
		WSACleanup();
		return 0;
	}

	SOCKET socket = CreateSocket(IPPROTO_TCP);

	Update(socket, argv[1]);

	closesocket(socket);
	WSACleanup();
	return 0;
}
