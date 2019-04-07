#ifndef IVANP_ENUMERATE_HH
#define IVANP_ENUMERATE_HH

template <typename I, typename T>
class enumerated {
  decltype(std::begin(std::declval<T>())) it;
  decltype(std::end(std::declval<T>())) _end;
  I i;
public:
  enumerated(T& x): it(std::begin(x)), _end(std::end(x)), i(0) { }

  const enumerated& begin() const { return *this; }
  const enumerated&   end() const { return *this; }

  bool operator!=(const enumerated&) const { return it != _end; }
  void operator++() { ++it; ++i; }
  std::pair<decltype(*it),I>
  operator*() const { return { *it, i }; }
};

template <typename I = size_t, typename T>
enumerated<I,T> enumerate(T&& x) { return { std::forward<T>(x) }; }

#endif
