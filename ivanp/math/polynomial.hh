#ifndef IVANP_POLYNOMIAL_HH
#define IVANP_POLYNOMIAL_HH

#include <cmath>

namespace ivanp { namespace poly {

template <typename T, typename U> // x -> ax+b
void transform_coords(double a, double b, int n, const T* oldc, U* newc) {
  int i, j, q0, q;
  if (n<2) return;

  int *p = new int[n]; // Pascal's triangle
  p[0] = 1;
  for (i=1; i<n; ++i) p[i] = 0;
  for (i=0; i<n; ++i) newc[i] = 0;

  for (j=0;;) {
    for (i=0; i<n; ++i) {
      if (p[i]==0) break;
      double m = p[i];
      if (i!=j) m *= std::pow(a,j-i);
      if (i!=0) m *= std::pow(b,i);
      newc[i] += m * oldc[j];
    }
    if (++j == n) break;
    for (i=1, q=1; q; ++i) {
      q0 = p[i];
      p[i] += q;
      q = q0;
    }
  }

  delete[] p;
}

}}

#endif
