#include <arpa/inet.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <iostream>

#define MSG_COUNT 1024
#define MSG_SIZE 32

std::mutex mtx;

static uint64_t packets = 0;
static uint64_t bytes = 0;
static void timer_handler(int signo) {
  printf("packets=%lu bytes=%lu\n", packets, bytes);
  packets = 0;
  bytes = 0;
}

void send_udp(int sockfd, mmsghdr *msg) {
  int retval;
  while (true) {
    retval = sendmmsg(sockfd, msg, MSG_COUNT, 0);
    if (retval < 0) {
      perror("Failed to sendmmsg");
      std::exit(1);
    } else {
      std::lock_guard<std::mutex> lock(mtx);
      packets += retval;
      bytes += retval * MSG_SIZE;
    }
  }
}

int main(int argc, char *argv[]) {
  struct mmsghdr msg[MSG_COUNT];
  struct iovec iov;

  std::vector<std::thread> threads;

  // Timer hook using SIGALRM
  signal(SIGALRM, timer_handler);
  struct itimerval timer = {0};
  timer.it_value.tv_sec = 1;
  timer.it_interval.tv_sec = 1;
  setitimer(ITIMER_REAL, &timer, NULL);

  // Prepare message content
  memset(&iov, 0, sizeof(iov));
  iov.iov_base = (void *)std::string(MSG_SIZE, '\x00').c_str();
  iov.iov_len = MSG_SIZE;

  // Fill each message
  memset(msg, 0, sizeof(msg));
  for (int i = 0; i < MSG_COUNT; i++) {
    msg[i].msg_hdr.msg_iov = &iov;
    msg[i].msg_hdr.msg_iovlen = 1;
  }

  for (int i = 1; i < argc; i++) {
    // Declare variables
    struct sockaddr_in servaddr;
    int sockfd;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("Failed to create socket");
      std::exit(1);
    }

    // Get destination address
    std::string dst_str = argv[i];
    std::string dst_ip = dst_str.substr(0, dst_str.find(':'));
    int dst_port = std::stoi(dst_str.substr(dst_str.find(':') + 1));

    // Set destination address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(dst_port);
    servaddr.sin_addr.s_addr = inet_addr(dst_ip.c_str());

    // Predefine Ports, not used in multi-threaded mode
    // struct sockaddr_in cliaddr;
    // cliaddr.sin_family = AF_INET;
    // cliaddr.sin_port = htons(65400);
    // cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // if(bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0){
    //   perror("Failed to bind");
    //   std::exit(1);
    // }

    // Connect to destination
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      perror("Failed to connect");
      std::exit(1);
    }

    threads.push_back(
        std::thread(send_udp, std::move(sockfd), msg));
  }

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}
