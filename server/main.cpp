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
            throw runtime_error("Ошибка открытия файла лога");
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
        // Создание сокета
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            throw runtime_error("Ошибка создания сокета");
        }

        // Настройка адреса
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        // Привязка сокета
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            close(serverSocket);
            throw runtime_error("Ошибка привязки сокета");
        }

        // Начало прослушивания порта
        if (listen(serverSocket, SOMAXCONN) == -1) {
            close(serverSocket);
            throw runtime_error("Ошибка прослушивания порта");
        }

        cout << "Сервер запущен, ожидает подключения..." << endl;

        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientSize = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

            if (clientSocket != -1) {
                thread(&TCPServer::handleClient, this, clientSocket).detach();
            } else {
                cout << "Ошибка принятия подключения" << endl;
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
            cout << "Ошибка обработки клиента: " << e.what() << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Использование: ./server <порт>" << endl;
        return 1;
    }

    int port = stoi(argv[1]);

    try {
        TCPServer server(port, "log.txt");
        server.start();
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << endl;
        return 1;
    }

    return 0;
}
