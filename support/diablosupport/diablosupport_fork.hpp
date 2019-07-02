#ifndef DIABLOSUPPORT_FORK_HPP
#define DIABLOSUPPORT_FORK_HPP

#include <functional>

typedef std::function<void(void)> F_ForkFunction;

void ForkAndWait(F_ForkFunction child);

#endif /* DIABLOSUPPORT_FORK_HPP */
