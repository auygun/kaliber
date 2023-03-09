#ifndef BASE_PIPE_H
#define BASE_PIPE_H

namespace base {

class Pipe {
 public:
  enum Direction { kRead = 0, kWrite };

  Pipe();
  ~Pipe();

  bool Create();

  int operator[](int which) const { return fd_[which]; }
  operator bool() const { return fd_[kRead] >= 0 || fd_[kWrite] >= 0; }

  void CloseAll();
  void Close(int which);

  bool MakeNonBlocking(int which);

 private:
  int fd_[2] = {-1, -1};
};

}  // namespace base

#endif  // BASE_PIPE_H
