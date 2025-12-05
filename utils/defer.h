#ifndef DEFER_UTILS_H
#define DEFER_UTILS_H

#define DeferLoopEnd(end) for (int _i_ = 0; !_i_; _i_ += 1, (end))
#define DeferLoop(begin, end)                                                  \
  for (int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define DeferLoopChecked(begin, end)                                           \
  for (int _i_ = 2 * !(begin); (_i_ == 2 ? ((end), 0) : !_i_); _i_ += 1, (end))

#endif // !DEFER_UTILS_H
