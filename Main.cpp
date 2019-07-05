#include "Main.h"

int main(int argc, char* argv[])
{
	if (Init() == SOCKET_ERROR)
	{
		WSACleanup();
		return 0;
	}
	SOCKET socket = CreateSocket(IPPROTO_UDP);
	Update(socket, argv[0]);
	closesocket(socket);
	WSACleanup();
	return 0;
}

int Init()
{
	WSADATA wsa;

	return WSAStartup(MAKEWORD(2, 2), &wsa);
}

SOCKET CreateSocket(int protocol)
{
	SOCKET result = INVALID_SOCKET;

	int type = SOCK_DGRAM;
	if (protocol == IPPROTO_TCP)
		type = SOCK_STREAM;

	result = socket(AF_INET, type, protocol);

	return result;
}

sockaddr_in* CreateAddress(char* ip, int port)
{
	sockaddr_in* result =
		(sockaddr_in*)calloc(sizeof(*result), 1);
	result->sin_family = AF_INET;
	result->sin_port = htons(port);

	if (ip == NULL)
		result->sin_addr.S_un.S_addr = INADDR_ANY;
	else
		inet_pton(result->sin_family, ip,
			&(result->sin_addr));
	return result;
	// Caller will be responsible for free()
}

int Bind(SOCKET sock, sockaddr_in* addr)
{
	int size = sizeof(sockaddr_in);
	int result = bind(sock, (sockaddr*)addr, size);
	if (result == SOCKET_ERROR)
		return GetLastError();

	return 0;
}

int Send(SOCKET sock, char* buffer, int bytes, sockaddr_in* dest)
{
	int result = sendto(sock, buffer, bytes, 0, (sockaddr*)dest, sizeof(sockaddr_in));
	if (result == SOCKET_ERROR)
		return -1;
	else
		return result;
}

int Receive(SOCKET sock, char* buffer, int maxBytes)
{
	sockaddr sender;
	int size = sizeof(sockaddr);

	int bytes = recvfrom(sock, buffer, maxBytes, 0, &sender, &size);
	if (bytes == SOCKET_ERROR)
		return -1;

	// Sender¡¯s IP address is now in sender.sa_data

	return bytes;
}

int Connect(SOCKET sock, sockaddr_in* address)
{
	if (connect(sock, (sockaddr*)address, sizeof(sockaddr_in)) == SOCKET_ERROR)
		return WSAGetLastError();
	else
		return 0;
}

void Update(SOCKET socket, const char* name)
{
	sockaddr_in* localAddress = CreateAddress(NULL, 8888); //INADDR_ANY
	sockaddr_in* serverAddress = CreateAddress("104.131.138.5", 8888);
	if (int err = Bind(socket, localAddress))
	{
		std::cout << "error! code : " << err << std::endl;
		return;
	}

	char buffer[BUFFER_MAXLENGTH];
	memset(buffer, '\0', BUFFER_MAXLENGTH); //needs to clear out with \0
	int j = 0;
	for (int i = 0; *(name + i) != '\0'; ++i)
		if (*(name + i) == '\\')
			j = i + 1;
	strcpy(buffer, (name + j));

	int sendResult = Send(socket, buffer, strlen(buffer), serverAddress);
	if (sendResult == -1)
	{
		std::cout << "error! code : " << GetLastError() << std::endl;
		return;
	}
	clock_t begin_time = clock();
	bool success = false;
	while (clock() - begin_time < 5 * CLOCKS_PER_SEC) //loop only for 5 sec
	{
		int receive = Receive(socket, buffer, BUFFER_MAXLENGTH); //this gets into eternal loop, don't know why
		if (receive != SOCKET_ERROR)
		{
			success = true;
			break;
		}
	}

	if (success)
		std::cout << buffer << std::endl;

	free(localAddress);
	free(serverAddress);
}