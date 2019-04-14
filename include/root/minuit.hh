#ifndef IVANP_MINUIT_HH
#define IVANP_MINUIT_HH

#include <utility>
#include <TMinuit.h>
#include <iostream>
#include <iomanip>

template <typename F>
struct minuit: public TMinuit {
  F f;
  unsigned npar;

  minuit(unsigned npar, const F& f): TMinuit(npar), f(f), npar(npar) { }
  minuit(unsigned npar, F&& f): TMinuit(npar), f(std::move(f)), npar(npar) { }
  minuit(minuit&& r): TMinuit(r.fNpar), f(std::move(r.f)), npar(r.npar) { }

  Int_t Eval(
    Int_t npar, Double_t *grad, Double_t &fval, Double_t *par, Int_t flag
  ) {
    fval = f(par);
    return 0;
  }

  struct par_iter {
    minuit* m;
    unsigned i;
    TString name;
    Double_t val=0, err=0, min=0, max=0;

    auto& operator++() noexcept { ++i; return *this; }
    bool operator!=(const par_iter&) const noexcept { return i != m->npar; }

    auto& begin() { return *this; }
    auto& end  () { return *this; }

    const auto& operator*() {
      Int_t iuint;
      m->mnpout( i, name, val, err, min, max, iuint );
      return *this;
    }

    auto& set() {
      Int_t ierflg;
      m->mnparm( i, name, val, err, min, max, ierflg );
      if (ierflg) throw std::runtime_error("mnparm");
    }

    friend std::ostream& operator<<(std::ostream& o, const par_iter& p) {
      return o
        << std::setw(3) << p.i
        << std::left << ' ' << std::setw(5) << p.name << std::internal
        << ": " << p.val << " ± " << p.err << '\n';
    }
  };

  par_iter pars() { return { this }; }
};

template <typename F>
minuit<std::decay_t<F>> make_minuit(unsigned npar, F&& f) {
  return { npar, std::forward<F>(f) };
}

#endif
