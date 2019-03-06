#ifndef IVANP_PCRE_HH
#define IVANP_PCRE_HH

#include <stdexcept>
#include <array>
#include <pcre.h>
#include "ivanp/string.hh"

namespace ivanp { namespace pcre {

struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... T>
  error(T&&... x): std::runtime_error(cat(std::forward<T>(x)...)) { };
  error(const char* str): std::runtime_error(str) { };
};

class regex;

class match {
  friend class regex;

  unsigned n, nalloc;
  int* subs;
  const char* orig;

  match& expand() {
    delete[] subs;
    subs = new int[(nalloc*=2)];
    return *this;
  }

  struct iterator {
    int* sub;
    const char* orig;

    std::array<const char*,2> operator*() {
      return { orig+sub[0], orig+sub[1] };
    }
    iterator& operator++() { sub += 2; return *this; }
    iterator& operator--() { sub -= 2; return *this; }
    auto operator- (const iterator& o) const noexcept { return sub- o.sub; }
    bool operator==(const iterator& o) const noexcept { return sub==o.sub; }
    bool operator!=(const iterator& o) const noexcept { return sub!=o.sub; }
    bool operator< (const iterator& o) const noexcept { return sub< o.sub; }
  };

public:
  match(unsigned nmax=4): n(0), nalloc(nmax*3), subs(new int[nalloc]) { }
  ~match() { if (subs) delete[] subs; }

  match(const match& o)
  : n(o.n), nalloc(n*3), subs(new int[nalloc]), orig(o.orig) { }
  match(match&& o)
  : n(o.n), nalloc(o.nalloc), subs(o.subs), orig(o.orig) {
    o.n = 0;
    o.nalloc = 0;
    o.subs = nullptr;
  }
  match& operator=(const match& o) {
    n = o.n;
    if (nalloc < n*3) {
      nalloc = n*3;
      if (subs) delete[] subs;
      subs = new int[nalloc];
    }
    memcpy(subs,o.subs,n*2*sizeof(int));
    orig = o.orig;
    return *this;
  }
  match& operator=(match&& o) {
    n = o.n; o.n = 0;
    nalloc = o.nalloc; o.nalloc = 0;
    subs = o.subs; o.subs = nullptr;
    orig = o.orig;
    return *this;
  }

  iterator begin() const { return { subs, orig }; }
  iterator end() const { return { subs+n*2, orig }; }
};

class regex {
  ::pcre* compiled;
  ::pcre_extra* extra;

public:
  regex(const char* expr, int options=0): compiled(nullptr), extra(nullptr) {
    const char* err_str;
    int err_offset;
    compiled = pcre_compile(expr,options,&err_str,&err_offset,nullptr);
    if (!compiled) throw error("pcre_compile: ",expr,": ",err_str);
    extra = pcre_study(compiled,0,&err_str); // optimize
    if (err_str) throw error("pcre_study: ",expr,": ",err_str);
  }
  regex(const std::string& expr, int options=0)
  : regex(expr.c_str(),options) { }

  regex(const regex&) = delete;
  regex& operator=(const regex&) = delete;
  regex(regex&& o): compiled(o.compiled), extra(o.extra) {
    o.compiled = nullptr;
    o.extra = nullptr;
  }
  regex& operator=(regex&& o) {
    compiled = o.compiled; o.compiled = nullptr;
    extra = o.extra; o.extra = nullptr;
    return *this;
  }

  ~regex() {
    if (compiled) pcre_free(compiled);
    if (extra) {
#ifdef PCRE_CONFIG_JIT
      pcre_free_study(extra);
#else
      pcre_free(extra);
#endif
    }
  }

  bool operator()(match& m, const char* str, size_t len = 0) {
    if (!len) len = strlen(str);
    const int rc = pcre_exec(
      compiled, extra, str, len,
      0, // start looking at this point
      0, // options
      m.subs, m.nalloc
    );

    if (rc < 0) {
      switch (rc) {
        case PCRE_ERROR_NOMATCH      : return false;
        case PCRE_ERROR_NULL         : throw error("PCRE_ERROR_NULL");
        case PCRE_ERROR_BADOPTION    : throw error("PCRE_ERROR_BADOPTION");
        case PCRE_ERROR_BADMAGIC     : throw error("PCRE_ERROR_BADMAGIC");
        case PCRE_ERROR_UNKNOWN_NODE : throw error("PCRE_ERROR_UNKNOWN_NODE");
        case PCRE_ERROR_NOMEMORY     : throw error("PCRE_ERROR_NOMEMORY");
        default                      : throw error();
      }
    }
    if (m.nalloc) {
      if (rc == 0) { // too many matches
        if (!(*this)(m.expand(),str,len)) return false;
      }
      m.n = rc;
      m.orig = str;
    }

    return true;
  }
};

regex operator""_re(const char* expr, size_t len) { return { expr }; }

}}

#endif
