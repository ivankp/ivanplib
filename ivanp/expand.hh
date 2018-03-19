#ifndef IVANP_EXPAND_HH
#define IVANP_EXPAND_HH

#ifdef __cpp_fold_expressions

#define EXPAND(EXPR) ((EXPR),...);

#else

#define EXPAND(EXPR) { \
  using expander = int[]; \
  (void)expander{0, ((void)(EXPR), 0)...}; \
}

#endif

#endif
