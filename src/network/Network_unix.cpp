#include "Network.h"

using namespace L;

void Network::init() {}
int Network::error() { return errno; }
bool Network::would_block() {
  int err(error());
  return err == EWOULDBLOCK || err == EAGAIN;
}
void Network::make_non_blocking(SOCKET sd) {
  fcntl(sd, F_SETFL, O_NONBLOCK);
}
