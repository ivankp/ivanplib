#ifndef IVANP_UNFOLD_HH
#define IVANP_UNFOLD_HH

#ifdef __cpp_fold_expressions

#define UNFOLD(EXPR) ((EXPR),...);

#else

#define UNFOLD(EXPR) { \
  using expander = int[]; \
  (void)expander{0, ((void)(EXPR), 0)...}; \
}

#endif

#endif
