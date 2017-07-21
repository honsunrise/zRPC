//
// Created by zhsyourai on 12/6/16.
//

#include <asm/errno.h>
#include <errno.h>
#include <malloc.h>
#include <sys/socket.h>
#include <unistd.h>
#include "source.h"
#include "rtti.h"
#include "zRPC/fd.h"

zRPC_fd *zRPC_fd_create(int fd) {
  zRPC_fd *new_fd = (zRPC_fd *) malloc(sizeof(zRPC_fd));
  RTTI_INIT_PTR(zRPC_fd, &new_fd->source);
  zRPC_source_init(&new_fd->source);
  new_fd->origin_fd = fd;
  return new_fd;
}

void zRPC_fd_destroy(zRPC_fd *fd) {
  if (fd) free(fd);
}

int zRPC_fd_origin(zRPC_fd *fd) {
  return fd->origin_fd;
}

void zRPC_fd_close(zRPC_fd *fd) {
  close(fd->origin_fd);
}

ssize_t zRPC_fd_read(zRPC_fd *fd, void *buf, size_t len) {
  struct msghdr msg;
  struct iovec iov;
  ssize_t read_bytes;

  if (len == 0) {
    return 0;
  }

  iov.iov_base = buf;
  iov.iov_len = len;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  do {
    read_bytes = recvmsg(fd->origin_fd, &msg, 0);
  } while (read_bytes < 0 && errno == EINTR);
  return read_bytes;
}

ssize_t zRPC_fd_write(zRPC_fd *fd, void *buf, size_t len) {
  struct msghdr msg;
  struct iovec iov;
  ssize_t sent_length;
  size_t sending_length = len;

  iov.iov_base = buf;
  iov.iov_len = sending_length;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  do {
    sent_length = sendmsg(fd->origin_fd, &msg, MSG_NOSIGNAL);
  } while (sent_length < 0 && (errno == EINTR || errno == EAGAIN));
  return sent_length;
}

