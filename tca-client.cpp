#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

mutex mtx;
condition_variable cv;
bool messageReady = false;
string receivedMessage;

void xorEncryptDecrypt(string &data, const string &key) {
    size_t keyLength = key.length();
    for (size_t i = 0; i < data.length(); i++) {
        data[i] ^= key[i % keyLength];
    }
}

void receiveMessages(SOCKET ConnectSocket, const string& key) {
    char recvbuf[512];
    int recvbuflen = 512;

    while (true) {
        memset(recvbuf, 0, recvbuflen);
        int bytesReceived = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (bytesReceived > 0) {
            string message(recvbuf, bytesReceived);
            xorEncryptDecrypt(message, key);

            // Synchronize message reception
            {
                lock_guard<mutex> lock(mtx);
                receivedMessage = message;
                messageReady = true;
            }
            cv.notify_one();
        } else if (bytesReceived == 0) {
            cout << "Connection closed" << endl;
            break;
        } else {
            cerr << "Receiving error" << endl;
            break;
        }
    }
}

int main() {
    int port;
    string key, username;
    char IPorNGROKlink[100];

    cout << "Welcome to the CHAT-TERMINAL-APPLICATION by firex.\n";
    cout << "Use the IP address or ngrok link to connect to the server.\n";
    cout << "Enter the requirements below to start as client.\n";
    cout<<"\n";
    cout << "Port: ";
    cin >> port;
    cout << "Key: ";
    cin >> key;
    cout << "Username: ";
    cin >> username;
    cout << "IP address of the server (or ngrok link): ";
    cin >> IPorNGROKlink;

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = nullptr, *ptr = nullptr, hints;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize WinSock" << endl;
        return 1;
    }

    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        cerr << "Failed to create a Socket" << endl;
        WSACleanup();
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int resultCode = getaddrinfo(IPorNGROKlink, to_string(port).c_str(), &hints, &result);
    if (resultCode != 0) {
        cerr << "getaddrinfo failed with error: " << resultCode << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    bool connected = false;
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            cerr << "Failed to connect" << endl;
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        connected = true;
        break;
    }

    freeaddrinfo(result);

    if (!connected) {
        cerr << "Unable to connect to any address" << endl;
        WSACleanup();
        return 1;
    }

    cout << "Connected to server" << endl;

    thread receiveThread(receiveMessages, ConnectSocket, key);

    while (true) {
        string message, fullmessage, usernameview;
        cout << "\nYou: ";
        getline(cin, message);
        usernameview = username + ": ";
        fullmessage = usernameview + message;
        xorEncryptDecrypt(fullmessage, key);
        send(ConnectSocket, fullmessage.c_str(), fullmessage.length(), 0);

        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [] { return messageReady; });
            cout <<"\n"<<receivedMessage << endl;
            messageReady = false;
        }
    }

    receiveThread.join();
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
