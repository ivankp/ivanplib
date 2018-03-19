#ifndef IVANP_APPLYING_ITER_HH
#define IVANP_APPLYING_ITER_HH

namespace ivanp {

template <typename It, typename F>
class applying_iter {
  It it;
  F f;

public:
  template <typename _It, typename _F>
  applying_iter(_It&& it, _F&& f)
  : it(std::forward<_It>(it)), f(std::forward<_F>(f)) { }

  inline auto operator*() { return f(*it); }
  inline auto operator*() const { return f(*it); }

  inline It& operator++() { return ++it; }
  inline It& operator--() { return --it; }

  inline bool operator!=(const applying_iter& r) const {
    return it != r.it;
  }
  inline bool operator!=(const It& r) const {
    return it != r;
  }
  inline bool operator==(const applying_iter& r) const {
    return it == r.it;
  }
  inline bool operator==(const It& r) const {
    return it == r;
  }
};

template <typename It, typename F>
inline applying_iter<It,F> make_applying_iter(It&& it, F&& f) {
  return { std::forward<It>(it), std::forward<F>(f) };
}

}

#endif
