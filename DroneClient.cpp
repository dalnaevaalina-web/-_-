#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    cout << "========================================" << endl;
    cout << "   ПОДКЛЮЧЕНИЕ К СЕРВЕРУ..." << endl;
    cout << "========================================" << endl;

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
        cout << "УСПЕШНО ПОДКЛЮЧЁН к серверу!" << endl;
    }
    else {
        cout << "ОШИБКА ПОДКЛЮЧЕНИЯ! Запусти сначала сервер." << endl;
        return 1;
    }

    string input;
    char buffer[1024];

    while (true) {
        cout << "\nВведите координаты (или 'exit' для выхода): ";
        getline(cin, input);

        if (input == "exit") {
            break;
        }

        send(client_socket, input.c_str(), (int)input.length(), 0);
        cout << "Отправлено: " << input << endl;

        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "ОТВЕТ ОТ СЕРВЕРА: " << buffer << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }

    closesocket(client_socket);
    WSACleanup();

    cout << "Отключён от сервера" << endl;
    return 0;
}