#include "gel/pipe.h"

#include <fcntl.h>
#include <unistd.h>

namespace base {

Pipe::Pipe() = default;

Pipe::~Pipe() {
  CloseAll();
}

Pipe::Pipe(Pipe&& other) {
  fd_[0] = other.fd_[0];
  fd_[1] = other.fd_[1];
  other.fd_[0] = other.fd_[1] = -1;
}

Pipe& Pipe::operator=(Pipe&& other) {
  CloseAll();
  fd_[0] = other.fd_[0];
  fd_[1] = other.fd_[1];
  other.fd_[0] = other.fd_[1] = -1;
  return *this;
}

bool Pipe::Create() {
  if (pipe(fd_))
    return false;
  return true;
}

void Pipe::CloseAll() {
  Close(kRead);
  Close(kWrite);
}

void Pipe::Close(int which) {
  if (fd_[which] >= 0) {
    close(fd_[which]);
    fd_[which] = -1;
  }
}

bool Pipe::MakeNonBlocking(int which) {
  if (fd_[which] < 0)
    return false;
  auto flags = fcntl(fd_[which], F_GETFL, 0);
  if (flags < 0)
    return false;
  if (flags & O_NONBLOCK)
    return true;
  return fcntl(fd_[which], F_SETFL, flags | O_NONBLOCK) != 0;
}

}  // namespace base
