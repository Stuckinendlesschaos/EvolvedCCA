#include <arpa/inet.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>

#define MSG_COUNT 1024
#define MSG_SIZE 1024
#define PORT 12233

static uint64_t packets = 0;
static uint64_t bytes = 0;
static void timer_handler(int signo) {
  printf("packets=%lu bytes=%lu\n", packets, bytes);
  packets = 0;
  bytes = 0;
}

int main(int argc, char *argv[]) {
  int retval;
  int sockfd;
  struct sockaddr_in addr;
  struct mmsghdr msg[MSG_COUNT];
  struct iovec iov[MSG_COUNT];
  char bufs[MSG_COUNT][MSG_SIZE];

  // Timer hook using SIGALRM
  signal(SIGALRM, timer_handler);
  struct itimerval timer = {0};
  timer.it_value.tv_sec = 1;
  timer.it_interval.tv_sec = 1;
  setitimer(ITIMER_REAL, &timer, NULL);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return 1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = inet_addr("0.0.0.0");

  memset(msg, 0, sizeof(msg));
  for (int i = 0; i < MSG_COUNT; i++) {
    iov[i].iov_base = bufs[i];
    iov[i].iov_len = MSG_SIZE;
    msg[i].msg_hdr.msg_iov = &iov[i];
    msg[i].msg_hdr.msg_iovlen = 1;
  }

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }

  while (1) {
    retval = recvmmsg(sockfd, msg, MSG_COUNT, MSG_WAITFORONE, NULL);
    if (retval < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("recvmmsg");
      exit(EXIT_FAILURE);
    } else {
      packets += retval;
      for (int i = 0; i < MSG_COUNT; i++) {
        auto *m = &msg[i];
        bytes += m->msg_len;
        m->msg_len = 0;
      }
    }
  }

  close(sockfd);

  exit(EXIT_SUCCESS);
}