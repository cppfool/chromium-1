// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NETWORK_CHANGE_NOTIFIER_LINUX_H_
#define NET_BASE_NETWORK_CHANGE_NOTIFIER_LINUX_H_

// socket.h is needed to define types for the linux kernel header netlink.h
// so it needs to come before netlink.h.
#include <sys/socket.h>
#include <linux/netlink.h>
#include "base/basictypes.h"
#include "base/message_loop.h"
#include "net/base/network_change_notifier_helper.h"

struct nlmsghdr;

namespace net {

class NetworkChangeNotifierLinux
    : public NetworkChangeNotifier, public MessageLoopForIO::Watcher {
 public:
  NetworkChangeNotifierLinux();

  // NetworkChangeNotifier methods:

  virtual void AddObserver(Observer* observer) {
    helper_.AddObserver(observer);
  }

  virtual void RemoveObserver(Observer* observer) {
    helper_.RemoveObserver(observer);
  }

  // MessageLoopForIO::Watcher methods:

  virtual void OnFileCanReadWithoutBlocking(int fd);
  virtual void OnFileCanWriteWithoutBlocking(int /* fd */);

 private:
  virtual ~NetworkChangeNotifierLinux();

  // Starts listening for netlink messages.  Also handles the messages if there
  // are any available on the netlink socket.
  void ListenForNotifications();

  // Attempts to read from the netlink socket into |buf| of length |len|.
  // Returns the bytes read on synchronous success and ERR_IO_PENDING if the
  // recv() would block.  Otherwise, it returns a net error code.
  int ReadNotificationMessage(char* buf, size_t len);

  // Handles the netlink message and notifies the observers.
  void HandleNotifications(
      const struct nlmsghdr* netlink_message_header, size_t len);

  internal::NetworkChangeNotifierHelper helper_;

  int netlink_fd_;  // This is the netlink socket descriptor.
  struct sockaddr_nl local_addr_;
  MessageLoopForIO* const loop_;
  MessageLoopForIO::FileDescriptorWatcher netlink_watcher_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierLinux);
};

}  // namespace net

#endif  // NET_BASE_NETWORK_CHANGE_NOTIFIER_LINUX_H_
