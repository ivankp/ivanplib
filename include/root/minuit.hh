#ifndef IVANP_MINUIT_HH
#define IVANP_MINUIT_HH

#include <utility>
#include <TMinuit.h>

template <typename F>
struct minuit final: public TMinuit {
  F f;

  minuit(unsigned npar, const F& f): TMinuit(npar), f(f) { }
  minuit(unsigned npar, F&& f): TMinuit(npar), f(std::move(f)) { }
  minuit(minuit&& r): TMinuit(r.fNpar), f(std::move(r.f)) { }

  inline Int_t Eval(
    Int_t npar, Double_t *grad, Double_t &fval, Double_t *par, Int_t flag
  ) {
    fval = f(par);
    return 0;
  }
};

template <typename F>
inline minuit<std::decay_t<F>> make_minuit(unsigned npar, F&& f) {
  return { npar, std::forward<F>(f) };
}

#endif
