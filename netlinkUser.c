#include <linux/netlink.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAX_PAYLOAD 1024
#define NETLINK_USER 31

struct msg_to_kern {
  uint32_t request; // 0: read an object and delete if found (val field should be ignored);
                    // 1: insert an object to table.
  uint32_t key1;
  uint32_t key2;
  uint32_t val;
};

struct msg_from_kern {
  uint32_t status; // 0: read failed (remaining data should be ignored);
                   // 1: read succeeded;
                   // 2: write finished (remaining data is an echo).
  uint32_t key1;
  uint32_t key2;
  uint32_t val;
};

int main()
{
  struct msg_to_kern query;
  struct msg_from_kern* reply;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr* nlh = NULL;
  struct iovec iov;
  int sock_fd;
  struct msghdr msg;

  printf("Input request type (0: read & delete, 1: insert): ");
  scanf("%u", &(query.request));
  if (query.request == 0) {
    printf("\nREAD MODE\n");
    printf("Input key 1: ");
    scanf("%u", &(query.key1));
    printf("Input key 2: ");
    scanf("%u", &(query.key2));
    query.val = 0;
  } else if (query.request == 1) {
    printf("\nWRITE MODE\n");
    printf("Input key 1: ");
    scanf("%u", &(query.key1));
    printf("Input key 2: ");
    scanf("%u", &(query.key2));
    printf("Input value: ");
    scanf("%u", &(query.val));
  } else {
    printf("Invalid value\n");
    return 0;
  }

  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid();
  bind(sock_fd, (struct sockaddr*) &src_addr, sizeof(src_addr));

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;
  dest_addr.nl_groups = 0;

  nlh = (struct nlmsghdr*) malloc(NLMSG_SPACE(MAX_PAYLOAD));
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;
  memcpy(NLMSG_DATA(nlh), &query, sizeof(query));

  iov.iov_base = (void*) nlh;
  iov.iov_len = nlh->nlmsg_len;
  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void*) &dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  sendmsg(sock_fd, &msg, 0);
  recvmsg(sock_fd, &msg, 0);

  reply = (struct msg_from_kern*) NLMSG_DATA(nlh);
  if (reply->status == 0) {
    printf("Read failed. Object not found.\n");
  } else if (reply->status == 1) {
    printf("Object found. K1, K2, V: %u, %u, %u\n", reply->key1, reply->key2, reply->val);
  } else if (reply->status == 2) {
    printf("Insertion finished. K1, K2, V: %u, %u, %u\n", reply->key1, reply->key2, reply->val);
  }

  close(sock_fd);
  return 0;
}