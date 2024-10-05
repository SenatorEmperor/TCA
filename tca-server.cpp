#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void xorEncryptDecrypt(string &data, const string &key) {
    size_t keyLength = key.length();
    for (size_t i = 0; i < data.length(); i++) {
        data[i] ^= key[i % keyLength];
    }
}

void handleClient(SOCKET ClientSocket, const string &key, const string &username) {
    char recvbuf[512];
    int recvbuflen = 512;

    while (true) {
        memset(recvbuf, 0, recvbuflen);
        int bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (bytesReceived > 0) {
            string message(recvbuf, bytesReceived);
            xorEncryptDecrypt(message, key);
            cout << "\n" << message << endl;

            string response, fullresponse, usernameview;
            usernameview = username + ": ";
            cout << "\nYou: ";
            getline(cin, response);
            if (response.empty()) continue;  // Skip empty messages

            fullresponse = usernameview + response;
            xorEncryptDecrypt(fullresponse, key);
            send(ClientSocket, fullresponse.c_str(), fullresponse.length(), 0);
        } else if (bytesReceived == 0) {
            cout << "Connection closed" << endl;
            break;
        } else {
            cerr << "Receiving error" << endl;
            break;
        }
    }

    closesocket(ClientSocket);
}

int main() {
    int port;
    string key, username;

    cout << "Welcome to the CHAT-TERMINAL-APPLICATION by firex.\n";
    cout << "Use ngrok if you want to communicate through the internet.\n";
    cout << "Enter the requirements below to start as server.\n";
    cout<<"\n";
    cout << "Port: ";
    cin >> port;
    cout << "Key: ";
    cin >> key;
    cout << "Username: ";
    cin >> username;
    cin.ignore();  // Ignore leftover newline character from input buffer

    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    struct sockaddr_in server, client;
    int clientSize = sizeof(client);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize WinSock" << endl;
        return 1;
    }

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        cerr << "Failed to create a Socket" << endl;
        WSACleanup();
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(ListenSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        cerr << "Failed to bind" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, 1) == SOCKET_ERROR) {
        cerr << "Failed to listen" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Listening on port: " << port << endl;

    ClientSocket = accept(ListenSocket, (struct sockaddr*)&client, &clientSize);
    if (ClientSocket == INVALID_SOCKET) {
        cerr << "Connection->(accept) failed" << endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, clientIp, sizeof(clientIp));
    cout << "[ " << clientIp << " ] connected to server" << endl;

    closesocket(ListenSocket);

    // Start handling client in a separate thread
    thread clientThread(handleClient, ClientSocket, key, username);

    clientThread.join(); // Wait for the client handling thread to finish
    WSACleanup();

    return 0;
}
