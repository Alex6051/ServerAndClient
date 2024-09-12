#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <memory>
#include <stdexcept>

using namespace std;

class LogFile {
public:
    LogFile(const string& filename) {
        logFile.open(filename, ios::app);
        if (!logFile.is_open()) {
            throw runtime_error("������ �������� ����� ����");
        }
    }

    void write(const string& message) {
        lock_guard<mutex> lock(fileMutex);
        logFile << message << endl;
    }

private:
    ofstream logFile;
    mutex fileMutex;
};

class TCPServer {
public:
    TCPServer(int port, const string& logFilePath)
        : serverPort(port), logFile(make_unique<LogFile>(logFilePath)) {}

    void start() {
        // �������� ������
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            throw runtime_error("������ �������� ������");
        }

        // ��������� ������
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        // �������� ������
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            close(serverSocket);
            throw runtime_error("������ �������� ������");
        }

        // ������ ������������� �����
        if (listen(serverSocket, SOMAXCONN) == -1) {
            close(serverSocket);
            throw runtime_error("������ ������������� �����");
        }

        cout << "������ �������, ������� �����������..." << endl;

        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientSize = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

            if (clientSocket != -1) {
                thread(&TCPServer::handleClient, this, clientSocket).detach();
            } else {
                cout << "������ �������� �����������" << endl;
            }
        }

        close(serverSocket);
    }

private:
    int serverPort;
    unique_ptr<LogFile> logFile;

    void handleClient(int clientSocket) {
        try {
            char buffer[1024];

            while (true) {
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) {
                    break;
                }

                string message(buffer);
                logFile->write(message);
            }

            close(clientSocket);
        } catch (const exception& e) {
            cout << "������ ��������� �������: " << e.what() << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "�������������: ./server <����>" << endl;
        return 1;
    }

    int port = stoi(argv[1]);

    try {
        TCPServer server(port, "log.txt");
        server.start();
    } catch (const exception& e) {
        cout << "������: " << e.what() << endl;
        return 1;
    }

    return 0;
}
