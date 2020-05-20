#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>

namespace base {

using Callback = std::function<void()>;

}  // namespace base

#endif  // CALLBACK_H
