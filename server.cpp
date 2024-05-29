// server.cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>

std::mutex file_mutex;
int server_socket;

void handle_client(int client_socket) {
    struct message {
        char operation[16];
        char name[128];
        int fileSize;
        char contents[32768];
    } m;

    char buffer[sizeof(message)];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received != sizeof(buffer)) {
        std::cerr << "Error: Failed to receive data.\n";
        close(client_socket);
        close(server_socket);
        return;
    }

    message received_data;
    std::memcpy(&received_data, buffer, sizeof(message));
    std::cout << "Received: " << received_data.operation <<  " " << received_data.name << " " << received_data.fileSize << " " << received_data.contents << std::endl;    
    if (strcmp(received_data.operation, "upload") == 0) {
        std::lock_guard<std::mutex> lock(file_mutex);

        if (mkdir("uploads", 0777) == -1 && errno != EEXIST) {
            std::cerr << "Error: Could not create uploads directory.\n";
            close(client_socket);
            return;
        }

        std::string filename(received_data.name);
        std::ofstream file("uploads/" + filename, std::ios::binary);
        file.write(received_data.contents, received_data.fileSize);
        file.close();
    }  else if (strcmp(received_data.operation, "download") == 0) {
        std::lock_guard<std::mutex> lock(file_mutex);

        std::string filename(received_data.name);
        std::ifstream file("uploads/" + filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: File not found.\n";
            close(client_socket);
            return;
        }

        file.seekg(0, std::ios::end);
        int size = file.tellg();
        size = size > 32768 ? 32768 : size; // Prevent overflow
        file.seekg(0, std::ios::beg);

        char* file_buffer = new char[size];
        file.read(file_buffer, size);
        file.close();

        message response_data;
        strcpy(response_data.operation, "download");
        strcpy(response_data.name, filename.c_str());
        response_data.fileSize = size;
        std::memcpy(response_data.contents, file_buffer, size);

        char serialized_response[sizeof(message)];
        std::memcpy(serialized_response, &response_data, sizeof(message));

        send(client_socket, serialized_response, sizeof(serialized_response), 0);
        delete[] file_buffer;
    }
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        close(server_socket);
        std::cout << "Server shutting down...\n";
        exit(0);
    }
}

int main() {
    signal(SIGINT, signal_handler);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error: Could not create socket.\n";
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Error: Could not bind socket.\n";
        return 1;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error: Could not listen on socket.\n";
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket >= 0) {
            std::cout << "New client" << std::endl;
            std::thread(handle_client, client_socket).detach();
        } else {
            std::cerr << "Error: Could not accept client.\n";
            return -1;
        }
    }

    close(server_socket);
    return 0;
}
