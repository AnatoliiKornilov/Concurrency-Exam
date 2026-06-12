// g++ epoll_multi_thread_echo_server.cpp -std=c++23 -pthread -o server

#include "thread_pool.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>

const int PORT = 8080;
const int MAX_EVENTS = 64;
const int BUFFER_SIZE = 1024;
const int THREAD_POOL_SIZE = 4;

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

ThreadPool pool(THREAD_POOL_SIZE);

void handle_client_data(int client_fd) {
  char buffer[BUFFER_SIZE];

  ssize_t length = read(client_fd, buffer, sizeof(buffer));

  if (length <= 0) {
    close(client_fd);
    return;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  write(client_fd, buffer, length);
}

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  bind(server_fd, (sockaddr*)&addr, sizeof(addr));

  listen(server_fd, 10);

  set_nonblocking(server_fd);

  int epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("epoll_create1");
    return 1;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
    perror("ctl add server");
    return 1;
  }

  std::vector<epoll_event> events(MAX_EVENTS);

  std::cout << "Multithreaded epoll echo server listening on port " << PORT << std::endl;

  while (true) {
    int nfds = epoll_wait(epfd, events.data(), MAX_EVENTS, -1);

    if (nfds < 0) {
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < nfds; ++i) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        int client_fd = accept(server_fd, nullptr, nullptr);

        if (client_fd < 0) {
          continue;
        }

        set_nonblocking(client_fd);

        ev.events = EPOLLIN;
        ev.data.fd = client_fd;

        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

        std::cout << "New client: " << client_fd << std::endl;
      } else {
        pool.enqueue([fd]() { 
          handle_client_data(fd); 
        });
      }
    }
  }

  close(epfd);

  close(server_fd);

  return 0;
}
