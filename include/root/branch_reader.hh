// ------------------------------------------------------------------
// Class template for reading TTree branches of any of specified types
// Works similarly to std::variant
// Writted by Ivan Pogrebnyak
// ------------------------------------------------------------------

#ifndef BRANCH_READER_HH
#define BRANCH_READER_HH

#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>

#include <TLeaf.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include "ivanp/error.hh"

namespace ivanp {

template <typename>
constexpr const char* root_type_str();
template <>
constexpr const char* root_type_str<Char_t>() { return "Char_t"; }
template <>
constexpr const char* root_type_str<UChar_t>() { return "UChar_t"; }
template <>
constexpr const char* root_type_str<Short_t>() { return "Short_t"; }
template <>
constexpr const char* root_type_str<UShort_t>() { return "UShort_t"; }
template <>
constexpr const char* root_type_str<Int_t>() { return "Int_t"; }
template <>
constexpr const char* root_type_str<UInt_t>() { return "UInt_t"; }
template <>
constexpr const char* root_type_str<Float_t>() { return "Float_t"; }
template <>
constexpr const char* root_type_str<Double_t>() { return "Double_t"; }
template <>
constexpr const char* root_type_str<Long64_t>() { return "Long64_t"; }
template <>
constexpr const char* root_type_str<ULong64_t>() { return "ULong64_t"; }
template <>
constexpr const char* root_type_str<Bool_t>() { return "Bool_t"; }

template <typename... Ts>
class branch_reader {
public:
  using value_type = std::common_type_t<std::remove_extent_t<Ts>...>;

  static constexpr bool is_array = ( ... || std::is_array_v<Ts> );

  static constexpr size_t num_types = sizeof...(Ts);
  static_assert(num_types>0);

private:
  size_t index;

  template <typename T>
  using reader_type = std::conditional_t< is_array,
    TTreeReaderArray<T>,
    TTreeReaderValue<T>
  >;

  char data[ std::max({sizeof(reader_type<std::remove_extent_t<Ts>>)...}) ];

  template <typename U, typename... Us>
  size_t get_index(const char* type_name) const {
    if (!strcmp(root_type_str<U>(),type_name))
      return num_types - (sizeof...(Us) + 1);
    else if constexpr (sizeof...(Us) > 0)
      return get_index<Us...>(type_name);
    else
      throw error("this branch_reader cannot read ",type_name);
  }

  template <typename U, typename... Us, typename F>
  auto call_impl(F&& f) {
    if (index == (num_types - (sizeof...(Us) + 1)))
      return f(reinterpret_cast<reader_type<U>*>(+data));
    else if constexpr (sizeof...(Us) > 0)
      return call_impl<Us...>(std::forward<F>(f));
    else __builtin_unreachable();
  }
  template <typename F>
  auto call(F&& f) {
    return call_impl<std::remove_extent_t<Ts>...>(std::forward<F>(f));
  }

  static auto* get_leaf(TTree* tree, const char* name) {
    auto* leaf = tree->GetLeaf(name);
    if (leaf) return leaf;
    throw error("no leaf \"",name,"\" in tree \"",tree->GetName(),'\"');
  }

public:
  branch_reader(TTreeReader& reader, const char* branch_name)
  : index(get_index<std::remove_extent_t<Ts>...>(
      get_leaf(reader.GetTree(),branch_name)->GetTypeName()
    ))
  {
    call([&](auto* p) {
      using T = std::decay_t<decltype(*p)>;
      new(p) T(reader,branch_name);
    });
  }
  branch_reader(TTreeReader& reader, const std::string& branch_name)
  : branch_reader(reader,branch_name.c_str()) { }
  ~branch_reader() {
    call([](auto* p) {
      using T = std::decay_t<decltype(*p)>;
      p->~T();
    });
  }

  value_type operator*() {
    return call([&](auto* p) -> value_type { return **p; });
  }

  value_type operator[](size_t i) {
    return call([&,i](auto* p) -> value_type { return (*p)[i]; });
  }

  const char* GetBranchName() {
    return call([&](auto* p){ return p->GetBranchName(); });
  }
};

using float_reader = branch_reader<double,float>;
using floats_reader = branch_reader<double[],float[]>;

template <typename T>
class branch_reader<T> {
public:
  using value_type = std::remove_extent_t<T>;

  static constexpr bool is_array = std::is_array_v<T>;

private:
  using impl_type = std::conditional_t< is_array,
    TTreeReaderArray<value_type>,
    TTreeReaderValue<value_type>
  >;
  impl_type impl;

public:
  branch_reader(TTreeReader& reader, const char* branch_name)
  : impl(reader,branch_name) { }
  branch_reader(TTreeReader& reader, const std::string& branch_name)
  : branch_reader(reader,branch_name.c_str()) { }

  const char* GetBranchName() const noexcept { return impl.GetBranchName(); }

  value_type& operator*() { return *impl; }

  template <bool A = is_array, typename V = value_type>
  std::enable_if_t<A,V&>
  operator[](size_t i) { return impl[i]; }

  template <bool A = is_array, typename V = value_type>
  std::enable_if_t<!A, decltype(std::declval<V&>()[0])>
  operator[](size_t i) { return (*impl)[i]; }
};

}

#endif
