#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <fstream>
#include <json/json.h>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

void main()
{
	Json::Value settings;
	std::ifstream settingsFile("settings.json", std::ifstream::binary);
	settingsFile >> settings;
	
	u_short port = settings["port"].asInt();
	bool running = true;

	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	cout << "Good morning everybody";

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		cerr << "Can't Initialize winsock! Quitting" << endl;
		return;
	}

	// Create a socket
	SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket == INVALID_SOCKET)
	{
		cerr << "Can't create a socket! Quitting" << endl;
		return;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listeningSocket, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening 
	listen(listeningSocket, SOMAXCONN);


	/*SOCKET client = accept(listeningSocket, nullptr, nullptr);
	char buf[4096];
	while (running)
	{
		ZeroMemory(buf, 4096);
		// Receive message
		int bytesIn = recv(client, buf, 4096, 0);
		send(client, buf, bytesIn + 1, 0);
	}*/
	









	fd_set fileDiscriptorsManager;
	char buf[4096];
	FD_ZERO(&fileDiscriptorsManager);
	FD_SET(listeningSocket, &fileDiscriptorsManager);
	while (running)
	{
		fd_set copyOfFileDiscriptorsManager = fileDiscriptorsManager;
		int socketCount = select(0, &copyOfFileDiscriptorsManager, nullptr, nullptr, nullptr);
		for (int i = 0; i < socketCount; i++)
		{
			SOCKET sock = copyOfFileDiscriptorsManager.fd_array[i];

			// Is it an inbound communication?
			if (sock == listeningSocket)
			{
				// Accept a new connection
				SOCKET client = accept(listeningSocket, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &fileDiscriptorsManager);

				// Send a welcome message to the connected client
				string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
				/*ZeroMemory(buf, 4096);
				// Receive message
				int bytesIn = recv(client, buf, 4096, 0);
				send(client, buf, bytesIn + 1, 0);*/
			}
			else // It's an inbound message
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &fileDiscriptorsManager);
				}
				else
				{
					send(sock, buf, bytesIn + 1, 0);
					/*// Check to see if it's a command. \quit kills the server
					if (buf[0] == '\\')
					{
						// Is the command quit? 
						string cmd = string(buf, bytesIn);
						if (cmd == "\\quit")
						{
							running = false;
							break;
						}

						// Unknown command
						continue;*/
					}
				}

		}
	}





/*	// Create the master file descriptor set and zero it
	fd_set fileDiscriptorsManager;
	FD_ZERO(&fileDiscriptorsManager);

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	FD_SET(listeningSocket, &fileDiscriptorsManager);

	// this will be changed by the \quit command (see below, bonus not in video!)
	bool running = true;

	while (running)
	{
		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages. 

		// E.g. You have a server and it's master file descriptor set contains 5 items;
		// the listening socket and four clients. When you pass this set into select(), 
		// only the sockets that are interacting with the server are returned. Let's say
		// only one client is sending a message at that time. The contents of 'copy' will
		// be one socket. You will have LOST all the other sockets.

		// SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

		fd_set copyOfFileDiscriptorsManager = fileDiscriptorsManager;

		// See who's talking to us
		int socketCount = select(0, &copyOfFileDiscriptorsManager, nullptr, nullptr, nullptr);
		cerr << "after select function" << endl;
		// Loop through all the current connections / potential connect
		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copyOfFileDiscriptorsManager.fd_array[i];

			// Is it an inbound communication?
			if (sock == listeningSocket)
			{
				// Accept a new connection
				SOCKET client = accept(listeningSocket, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &fileDiscriptorsManager);

				// Send a welcome message to the connected client
				string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else // It's an inbound message
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &fileDiscriptorsManager);
				}
				else
				{
					// Check to see if it's a command. \quit kills the server
					if (buf[0] == '\\')
					{
						// Is the command quit? 
						string cmd = string(buf, bytesIn);
						if (cmd == "\\quit")
						{
							running = false;
							break;
						}

						// Unknown command
						continue;
					}
				}
			}
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listeningSocket, &fileDiscriptorsManager);
	closesocket(listeningSocket);

	// Message to let users know what's happening.
	string msg = "Server is shutting down. Goodbye\r\n";

	while (fileDiscriptorsManager.fd_count > 0)
	{
		// Get the socket number
		SOCKET sock = fileDiscriptorsManager.fd_array[0];

		// Send the goodbye message
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &fileDiscriptorsManager);
		closesocket(sock);
	}*/

	// Cleanup winsock
	WSACleanup();

	system("pause");
}