#ifndef IVANP_RING_HH
#define IVANP_RING_HH

namespace ivanp {

template <typename T>
class ring: T {
public:
  inline auto& operator[](size_t i) noexcept {
    return T::operator[](i%T::size());
  }
  inline const auto& operator[](size_t i) const noexcept {
    return T::operator[](i%T::size());
  }
  inline T& operator*() noexcept { return *this; }
  inline T& operator*() const noexcept { return *this; }

  inline operator bool() const noexcept { return T::size(); }
};

}

#endif
