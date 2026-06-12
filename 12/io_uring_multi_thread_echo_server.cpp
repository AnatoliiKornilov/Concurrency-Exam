// g++ io_uring_multi_thread_echo_server.cpp -std=c++23 -pthread -luring -o server

#include <arpa/inet.h>
#include <fcntl.h>
#include <liburing.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

const int PORT = 8080;
const int BACKLOG = 128;
const int BUFFER_SIZE = 4096;
const int QUEUE_DEPTH = 256;
const int THREADS = 2;

struct Connection {
  int fd;
  enum class State { READ, WRITE } state;
  char buffer[BUFFER_SIZE];
  std::size_t len;
};

class UringReactor {
 public:
  explicit UringReactor(int id) : id_(id) {}

  bool init() {
    memset(&ring_, 0, sizeof(ring_));

    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring_, 0);

    if (ret < 0) {
      std::cerr << "io_uring_queue_init failed: " << -ret << std::endl;
      return false;
    }

    return true;
  }

  ~UringReactor() { io_uring_queue_exit(&ring_); }

  void add_accept(int listen_fd) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);

    if (sqe == nullptr) {
      std::cerr << "Failed to get SQE for accept\n";
      return;
    }

    io_uring_prep_accept(sqe, listen_fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, nullptr);
  }

  void add_read(Connection *conn) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);

    if (sqe == nullptr) {
      std::cerr << "Failed to get SQE for read\n";
      return;
    }

    conn->state = Connection::State::READ;

    io_uring_prep_recv(sqe, conn->fd, conn->buffer, BUFFER_SIZE, 0);
    io_uring_sqe_set_data(sqe, conn);
  }

  void add_write(Connection *conn, size_t len) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);

    if (sqe == nullptr) {
      std::cerr << "Failed to get SQE for write\n";
      return;
    }

    conn->len = len;
    conn->state = Connection::State::WRITE;

    io_uring_prep_send(sqe, conn->fd, conn->buffer, len, 0);
    io_uring_sqe_set_data(sqe, conn);
  }

  void run(int listen_fd) {
    add_accept(listen_fd);

    io_uring_submit(&ring_);  // отправляем в ядро

    while (true) {
      int ret = io_uring_submit_and_wait(&ring_, 1);

      if (ret < 0) {
        std::cerr << "io_uring_submit_and_wait error: " << -ret << std::endl;
        break;
      }

      struct io_uring_cqe* cqe;
      unsigned int head;
      unsigned int processed = 0;

      io_uring_for_each_cqe(&ring_, head, cqe) {
        processed++;

        Connection* conn = (Connection*) io_uring_cqe_get_data(cqe);

        int res = cqe->res;

        if (conn == nullptr) {
          if (res >= 0) {
            int client_fd = res;

            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

            Connection* new_conn = new Connection{client_fd, Connection::State::READ, {0}, 0};
            add_read(new_conn);
          }

          add_accept(listen_fd);
        } else {
          if (res <= 0) {
            close(conn->fd);

            delete conn;
          } else {
            if (conn->state == Connection::State::READ) {
              add_write(conn, res);
            } else {
              add_read(conn);
            }
          }
        }
      }

      io_uring_cq_advance(&ring_, processed);
    }
  }

 private:
  int id_;
  struct io_uring ring_;
};

void worker_thread(int thread_id, int listen_fd) {
  UringReactor reactor(thread_id);

  if (!reactor.init()) {
    std::cerr << "Thread " << thread_id << " failed to init io_uring\n";
    return;
  }

  std::cout << "Thread " << thread_id << " started, listening on fd " << listen_fd << std::endl;

  reactor.run(listen_fd);
}

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }

  if (listen(server_fd, BACKLOG) < 0) {
    perror("listen");
    return 1;
  }

  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  std::cout << "io_uring echo server listening on port " << PORT << std::endl;

  std::vector<std::thread> threads;
  for (int i = 0; i < THREADS; ++i) {
    threads.emplace_back(worker_thread, i, server_fd);
  }

  for (std::thread& thread : threads) {
    thread.join();
  }

  close(server_fd);
  return 0;
}
