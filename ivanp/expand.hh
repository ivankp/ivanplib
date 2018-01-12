#ifndef IVANP_EXPAND_HH
#define IVANP_EXPAND_HH

#define EXPAND(EXPR) { \
  using expander = int[]; \
  (void)expander{0, ((void)(EXPR), 0)...}; \
}

#endif
