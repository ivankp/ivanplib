#ifndef IVANP_TIME_SEED_HH
#define IVANP_TIME_SEED_HH

#include <random>
#include <chrono>

auto time_seed() {
  const auto t = std::chrono::system_clock::now().time_since_epoch();
  return
    std::chrono::duration_cast<std::chrono::microseconds>(t).count()
    - std::chrono::duration_cast<std::chrono::seconds>(t).count()*1000000u;
}

#endif
