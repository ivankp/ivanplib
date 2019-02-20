#ifndef IVANP_ROOT_TKEY_HH
#define IVANP_ROOT_TKEY_HH

// Improved TKey interface

#include <string>
#include <stdexcept>
#include <TKey.h>
#include <TClass.h>

class ROOT_type_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

#ifdef ROOT_TList
template <typename T>
class list_cast {
  TList* list;
  using list_iter = decltype(list->begin());
  class iter {
    list_iter it;
  public:
    iter(list_iter it): it(it) { }
    inline T& operator*() noexcept { return static_cast<T&>(**it); }
    inline T* operator->() noexcept { return static_cast<T*>(*it); }
    inline iter& operator++() noexcept(noexcept(++it)) { ++it; return *this; }
    inline bool operator==(const iter& r) const noexcept(noexcept(it==r.it))
    { return it == r.it; }
    inline bool operator!=(const iter& r) const noexcept(noexcept(it!=r.it))
    { return it != r.it; }
  };
public:
  list_cast(TList* list): list(list) { }
  iter begin() { return list->begin(); }
  iter   end() { return list->  end(); }
};
#endif

#ifdef ROOT_TDirectory
inline list_cast<TKey> get_keys(TDirectory* dir) noexcept {
  return { dir->GetListOfKeys() };
}
#endif

inline const TClass* get_class(const char* name) {
  using namespace std::string_literals;
  const TClass* class_ptr = TClass::GetClass(name);
  if (!class_ptr) throw ROOT_type_error("cannot find TClass "s+name);
  return class_ptr;
}
inline const TClass* get_class(const TKey& key) {
  return get_class(key.GetClassName());
}

template <typename T>
inline bool inherits_from(const TClass* c) {
  return c->InheritsFrom(T::Class());
}

template <typename T>
inline T* key_cast(TKey& key) {
  return dynamic_cast<T*>(key.ReadObj());
}
template <typename T>
inline T* safe_key_cast(TKey& key) {
  if (inherits_from<T>(get_class(key)))
    return dynamic_cast<T*>(key.ReadObj());
  else return nullptr;
}

#endif
