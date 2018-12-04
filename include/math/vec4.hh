#ifndef IVANP_VEC4_HH
#define IVANP_VEC4_HH

#include <cmath>

namespace ivanp {

template <typename T=double>
struct vec4 {
  T v[4];

  struct XYZT_t { } static XYZT;
  struct PtEtaPhiM_t { } static PtEtaPhiM;

  vec4(): v{0,0,0,0} { }
  vec4(T x, T y, T z, T t): v{x,y,z,t} { }
  vec4(T x, T y, T z, T t, XYZT_t): v{x,y,z,t} { }
  vec4(T pt, T eta, T phi, T m, PtEtaPhiM_t) {
    pt = std::abs(pt);
    v[0] = pt*std::cos(phi);
    v[1] = pt*std::sin(phi);
    v[2] = pt*std::sinh(eta);
    v[3] = std::sqrt(
      m >= 0
      ? v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+m*m
      : std::max(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]-m*m,T(0))
    );
  }
  template <typename U>
  vec4(const U& p, XYZT_t): vec4(p[0], p[1], p[2], p[3], XYZT) { }
  template <typename U>
  vec4(const U& p, PtEtaPhiM_t): vec4(p[0], p[1], p[2], p[3], PtEtaPhiM) { }

  T& operator[](unsigned i) noexcept { return v[i]; }
  const T& operator[](unsigned i) const noexcept { return v[i]; }

  T x() const noexcept { return v[0]; }
  T y() const noexcept { return v[1]; }
  T z() const noexcept { return v[2]; }
  T t() const noexcept { return v[3]; }

  T px() const noexcept { return v[0]; }
  T py() const noexcept { return v[1]; }
  T pz() const noexcept { return v[2]; }
  T e () const noexcept { return v[3]; }

  T pt2() const noexcept { return v[0]*v[0]+v[1]*v[1]; }
  T pt() const noexcept { return std::sqrt(pt2()); }
  T mag2() const noexcept { return v[0]*v[0]+v[1]*v[1]+v[2]*v[2]; }
  T mag() const noexcept { return std::sqrt(mag2()); }
  T cos_theta() const noexcept {
    const T a = mag();
    return a != 0 ? v[2]/a : 1;
  }
  T eta() const noexcept {
    const T ct = cos_theta();
    if (std::abs(ct) < 1) return -0.5*std::log((1-ct)/(1+ct));
    if (v[2] == 0) return 0;
    if (v[2] > 0) return 10e10;
    else       return -10e10;
  }
  T rap() const noexcept { return 0.5*std::log((v[3]+v[2])/(v[3]-v[2])); }
  T phi() const noexcept { return std::atan2(v[1],v[0]); }
  T m2() const noexcept { return v[3]*v[3]-mag2(); }
  T m() const noexcept {
    const T m2_ = m2();
    return m2_ >= 0 ? std::sqrt(m2_) : -std::sqrt(-m2_);
  }
  T et2() const noexcept { return v[3]*v[3] - v[2]*v[2]; }
  T et() const noexcept {
    const T et2_ = et2();
    return et2_ >= 0 ? std::sqrt(et2_) : -std::sqrt(-et2_);
  }

  template <typename U>
  vec4& operator+=(const U& r) noexcept {
    v[0] += r[0];
    v[1] += r[1];
    v[2] += r[2];
    v[3] += r[3];
    return *this;
  }
  template <typename U>
  vec4& operator-=(const U& r) noexcept {
    v[0] -= r[0];
    v[1] -= r[1];
    v[2] -= r[2];
    v[3] -= r[3];
    return *this;
  }
};

template <typename A, typename B>
inline auto operator+(const vec4<A>& a, const B& b) noexcept
-> vec4<decltype(a[0]+b[0])>
{ return { a[0]+b[0], a[1]+b[1], a[2]+b[2], a[3]+b[3] }; }
template <typename A, typename B>
inline auto operator-(const vec4<A>& a, const B& b) noexcept
-> vec4<decltype(a[0]-b[0])>
{ return { a[0]-b[0], a[1]-b[1], a[2]-b[2], a[3]-b[3] }; }

}

#endif
