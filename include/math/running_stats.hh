#ifndef IVANP_RUNNING_STAT_HH
#define IVANP_RUNNING_STAT_HH

#include <cmath>

namespace ivanp {

class running_stats {
  double W=0, m=0, S=0;
public:
  bool ok() const noexcept { return std::isnormal(W); }

  void push(double x, double w=1) noexcept {
    if (!std::isnormal(w)) return;
    W += w;
    if (ok()) {
      const double m0 = m;
      m += (x-m0) * w / W;
      S += w * (x-m0) * (x-m);
    }
  }
  void operator()(double x) noexcept { return push(x); }

  double total() const noexcept { return W; }
  double mean() const noexcept { return ok() ? m : 0.; }
  double var() const noexcept { return ok() ? S/W : 0.; }
  double stdev() const noexcept { return std::sqrt( var() ); }
};

}

#endif
