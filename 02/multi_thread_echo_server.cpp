// g++ multi_thread_echo_server.cpp -std=c++23 -o server

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

const int PORT = 8080;
const int BACKLOG = 10;  // размер очереди ожидания
const size_t BUFFER_SIZE = 1024;

// Функция, которая будет выполняться в отдельном потоке для каждого клиента
void handle_client(int client_fd) {
  std::cout << "[Thread " << std::this_thread::get_id()
            << "] New client, fd=" << client_fd << std::endl;

  char buffer[BUFFER_SIZE];

  while (true) {
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

    if (bytes_read <= 0) {
      // Клиент закрыл соединение или ошибка
      break;
    }

    // Эхо: отправляем обратно столько же байт
    ssize_t bytes_written = write(client_fd, buffer, bytes_read);

    if (bytes_written != bytes_read) {
      std::cerr << "Partial write or error on fd=" << client_fd << std::endl;
      break;
    }
  }

  std::cout << "[Thread " << std::this_thread::get_id()
            << "] Client disconnected, fd=" << client_fd << std::endl;

  close(client_fd);
}

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    std::cerr << "socket() failed\n";
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "bind() failed\n";
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, BACKLOG) < 0) {
    std::cerr << "listen() failed\n";
    close(server_fd);
    return 1;
  }

  std::cout << "Multithreaded echo server listening on port " << PORT
            << std::endl;

  // Храним потоки, чтобы потом join (здесь не обязательно, так как сервер бесконечный)
  std::vector<std::thread> threads;

  while (true) {
    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      continue;
    }

    // Создаём поток, который займётся клиентом
    threads.emplace_back([client_fd]() { handle_client(client_fd); });

    threads.back().detach();
  }

  for (std::thread& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  close(server_fd);
  return 0;
}
