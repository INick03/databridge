#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

struct message {
    char operation[16];
    char name[128];
    int fileSize;
    char contents[32768];
};

void upload_file(int server_socket) {
    std::string operation = "upload";

    std::cout << "Enter filename to upload: ";
    std::string filename;
    std::cin >> filename;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: File not found.\n";
        return;
    }

    file.seekg(0, std::ios::end);
    int size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size];
    file.read(buffer, size);
    file.close();

    message m;

    strcpy(m.operation, "upload");
    strcpy(m.name, filename.c_str());
    m.fileSize = size;
    strcpy(m.contents, buffer);

    char serialized[sizeof(message)];
    std::memcpy(serialized, &m, sizeof(message));

    std::cout << m.name << " " << m.fileSize << " " << m.contents << std::endl;
    send(server_socket, serialized, sizeof(serialized), 0);
    delete[] buffer;
}

void download_file(int server_socket) {
    char buffer[8192];

    std::cout << "Enter filename to download: ";
    std::string filename;
    std::cin >> filename;

    std::string operation = "download";

    message m;
    strcpy(m.operation, operation.c_str());
    strcpy(m.name, filename.c_str());

    char serialized[sizeof(message)];
    std::memcpy(serialized, &m, sizeof(message));
    send(server_socket, serialized, sizeof(serialized), 0);

    char fileBuffer[sizeof(message)];
    recv(server_socket, fileBuffer, sizeof(fileBuffer), 0);
    message received_data;
    std::memcpy(&received_data, fileBuffer, sizeof(message));

    std::cout << received_data.operation << " " << received_data.name << " " << received_data.fileSize << " " << received_data.contents << std::endl;
    std::ofstream file(received_data.name, std::ios::binary | std::ios::out);
    file << received_data.contents;
    file.close();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    const char *server_ip = argv[1];
        int port = std::stoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error: Could not create socket.\n";
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid address/Address not supported.\n";
        return 1;
    }

    if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Error: Could not connect to server.\n";
        return 1;
    }

    std::cout << "Choose an operation: upload or download\n";
    std::string operation;
    std::cin >> operation;

    if (operation == "upload") {
        upload_file(client_socket);
    } else if (operation == "download") {
        download_file(client_socket);
    } else {
        std::cerr << "Invalid operation.\n";
    }

    close(client_socket);
    return 0;
}

