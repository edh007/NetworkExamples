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
	sockaddr_in* result =
		(sockaddr_in*)calloc(sizeof(*result), 1);
	result->sin_family = AF_INET;
	result->sin_port = htons((u_short)port);

	if (ip == NULL)
#ifdef _WIN32
		result->sin_addr.S_un.S_addr = INADDR_ANY;
#else
        result->sin_addr.s_addr = INADDR_ANY;
#endif
	else
		inet_pton(result->sin_family, ip,
			&(result->sin_addr));
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

void Update(SOCKET socket, const char* name)
{
	sockaddr_in* serverAddress = CreateAddress("104.131.138.5", 8888);

	while (int err = Connect(socket, serverAddress))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		printf(".");

#ifdef _WIN32
		if (err == 10056)
			break;
		else if (err != 10035 && err != 10037)
		{
			printf("error! code : %i\n", err);
			return;
		}
#else
		if (err == 106)
			break;
		else if (err != 115)
		{
			printf("error! code : %i\n", err);
			return;
		}
#endif
	}

	char buffer[BUFFER_MAXLENGTH];
	memset(buffer, '\0', BUFFER_MAXLENGTH); //needs to clear out with \0
	int j = 0;
	for (int i = 0; *(name + i) != '\0'; ++i)
		if (*(name + i) == '\\' || *(name + i) == '/')
			j = i + 1;
#ifdef _WIN32
	strcpy_s(buffer, (name + j));
#else
	strcpy(buffer, (name + j));
#endif

	while (int sendResult = SendTCP(socket, buffer, strlen(buffer)) != strlen(buffer))
		if (sendResult == SOCKET_ERROR)
		{
			printf("error! code : %i\n", GetLastError());
			return;
		}

	Close(socket, false);

	while (int recvResult = ReceiveTCP(socket, buffer, BUFFER_MAXLENGTH) != 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		printf(".");
		if (recvResult == SOCKET_ERROR)
		{
			printf("error! code : %i\n", GetLastError());
			return;
		}
	}

	Close(socket, true);

    printf("%s\n", buffer);

	free(serverAddress);
}

int main(int /*argc*/, char* argv[])
{
	if (Init() == SOCKET_ERROR)
	{
		WSACleanup();
		return 0;
	}

	SOCKET socket = CreateSocket(IPPROTO_TCP);

	Update(socket, argv[0]);

	closesocket(socket);
	WSACleanup();
	return 0;
}
