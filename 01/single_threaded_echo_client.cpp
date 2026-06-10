// g++ single_threaded_echo_client.cpp -std=c++23 -o client

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

const char* HOST = "127.0.0.1";
int PORT = 8080;
size_t BUFFER_SIZE = 1024;

int main() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    std::cerr << "socket() failed\n";
    return 1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, HOST, &server_addr.sin_addr) <= 0) {
    std::cerr << "inet_pton() failed\n";
    close(sock);
    return 1;
  }

  if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "connect() failed\n";
    close(sock);
    return 1;
  }

  std::string msg;
  std::cout << "Enter message (or 'quit' to exit): ";
  std::getline(std::cin, msg);

  if (msg == "quit") {
    close(sock);
    return 0;
  }
  msg += '\n';

  write(sock, msg.c_str(), msg.size());

  char buffer[BUFFER_SIZE];
  ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
  if (bytes > 0) {
    buffer[bytes] = '\0';
    std::cout << "Server echo: " << buffer;
  } else {
    std::cout << "No response or server closed connection.\n";
  }

  close(sock);
  return 0;
}