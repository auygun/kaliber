#include "gel/pipe.h"

#include <fcntl.h>
#include <unistd.h>

namespace base {

Pipe::Pipe() = default;

Pipe::~Pipe() {
  CloseAll();
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
