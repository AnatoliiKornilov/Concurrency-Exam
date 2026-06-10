// g++ multi_thread_echo_client.cpp -std=c++23 -o client

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

const char* HOST = "127.0.0.1";
const int PORT = 8080;
const int NUM_CLIENT_THREADS = 10;
const unsigned int ITERATIONS_PER_THREAD = 5;

void client_worker(int id) {
  for (unsigned int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
    // 1. Создаём сокет
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
      std::osyncstream(std::cerr) << "[Client " << id << "] socket() failed\n";
      continue;
    }

    // 2. Готовим адрес сервера
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server_addr.sin_addr);

    // 3. Устанавливаем соединение
    if (connect(sock, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
      std::osyncstream(std::cerr) << "[Client " << id << "] connect() failed\n";
      close(sock);
      continue;
    }

    // 4. Формируем сообщение
    std::string msg = "Hello from client " + std::to_string(id) + "\n";
    write(sock, msg.c_str(), msg.size());

    // 5. Читаем ответ
    char buffer[1024];
    ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
      buffer[bytes] = '\0';

      std::osyncstream(std::cout)
          << "[Client " << id << "] received: " << buffer;
    } else {
      std::osyncstream(std::cout) << "[Client " << id << "] no response\n";
    }

    close(sock);

    std::this_thread::sleep_for(1s);
  }

  std::osyncstream(std::cout) << "[Client " << id << "] finished after " << ITERATIONS_PER_THREAD << " iterations.\n";
}

int main() {
  std::vector<std::thread> threads;

  for (int i = 0; i < NUM_CLIENT_THREADS; ++i) {
    threads.emplace_back(client_worker, i + 1);
  }

  for (std::thread& thread : threads) {
    thread.join();
  }

  return 0;
}
