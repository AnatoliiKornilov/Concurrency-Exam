// g++ single_threaded_echo_server.cpp -std=c++23 -o server

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int PORT = 8080;
int BACKLOG = 5;
size_t BUFFER_SIZE = 1024;

int main() {
  // 1. Создаём сокет
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    std::cerr << "socket creation failed" << std::endl;
    return 1;
  }

  // Устанавливаем опцию SO_REUSEADDR, чтобы порт можно было сразу
  // переиспользовать
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "setsockopt failed" << std::endl;
    close(server_fd);
    return 1;
  }

  // 2. Подготавливаем структуру адреса для bind
  struct sockaddr_in address;
  address.sin_family = AF_INET;          // IPv4
  address.sin_addr.s_addr = INADDR_ANY;  // Слушаем все интерфейсы
  address.sin_port = htons(PORT);  // Порт 8080 в сетевом порядке

  // Привязываем сокет к адресу
  if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
    std::cerr << "bind failed" << std::endl;
    close(server_fd);
    return 1;
  }

  // 3. Начинаем слушать
  if (listen(server_fd, BACKLOG) < 0) {
    std::cerr << "listen failed" << std::endl;
    close(server_fd);
    return 1;
  }

  std::cout << "Server is listening on port " << PORT << std::endl;

  // 4. Цикл обработки клиентов
  while (true) {
    // Блокируемся до подключения клиента
    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
      std::cerr << "accept failed" << std::endl;
      continue;
    }
    std::cout << "New client connected, fd = " << client_fd << std::endl;

    char buffer[BUFFER_SIZE];
    while (true) {
      // Блокируемся, пока клиент не пришлёт данные
      ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

      if (bytes_read <= 0) {
        // Клиент закрыл соединение или ошибка
        break;
      }
      // Отправляем данные обратно
      write(client_fd, buffer, bytes_read);
    }

    std::cout << "Client disconnected, fd = " << client_fd << std::endl;
    close(client_fd);
  }

  return 0;
}
