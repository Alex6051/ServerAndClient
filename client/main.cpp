#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

using namespace std;

class TCPClient {
public:
    TCPClient(const string& name, const string& serverIP, int serverPort, int period)
        : clientName(name), serverIP(serverIP), serverPort(serverPort), period(period) {}

    void start() {
        while (true) {
            int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (clientSocket == -1) {
                cout << "Ошибка создания сокета" << endl;
                return;
            }

            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(serverPort);
            inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

            if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
                cout << "Ошибка подключения к серверу" << endl;
                close(clientSocket);
                return;
            }

            string message = getCurrentTime() + " " + clientName;
            send(clientSocket, message.c_str(), message.size(), 0);

            close(clientSocket);
            this_thread::sleep_for(chrono::seconds(period));
        }
    }

private:
    string clientName;
    string serverIP;
    int serverPort;
    int period;

    string getCurrentTime() {
        auto now = chrono::system_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        time_t currentTime = chrono::system_clock::to_time_t(now);
        tm* tmTime = localtime(&currentTime);

        stringstream ss;
        ss << "[" << put_time(tmTime, "%Y-%m-%d %H:%M:%S") << "." << setfill('0') << setw(3) << ms.count() << "]";
        return ss.str();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Использование: ./client <имя_клиента> <порт> <период>" << endl;
        return 1;
    }

    string clientName = argv[1];
    int port = stoi(argv[2]);
    int period = stoi(argv[3]);;

    TCPClient client(clientName, "127.0.0.1", port, period);
    client.start();

    return 0;
}
