#ifndef IVANP_SCRIBE_BINNER_HH
#define IVANP_SCRIBE_BINNER_HH

namespace ivanp { namespace scribe {

template <typename Bin>
struct hist_trait {
  static std::string type_name() {
    return "hist<"+trait<Bin>::type_name()+">";
  }
  static std::string type_def() {
    std::stringstream s;
    s << "[\"[lin_axis,list_axis]#\",\"axes\"],"
         "[\"[null," << trait<Bin>::type_name() << ",^#]#\",\"bins\"]";
    // UNFOLD(( s << subst_t<char,Ax>('#') ))
    return s.str();
  }
};

template <typename Bin, typename... Ax, typename Container, typename Filler>
struct trait<binner<Bin,std::tuple<Ax...>,Container,Filler>>: hist_trait<Bin> {
private:
  using hist = binner<Bin,std::tuple<Ax...>,Container,Filler>;
  using i_t = typename hist::size_type;
  using ii_t = typename hist::index_array_type;

  template <unsigned I>
  static std::enable_if_t<I!=0>
  write_bins(std::ostream& o, const hist& h, ii_t& ii) {
    using spec = typename hist::template axis_spec<I-1>;
    i_t n = h.template axis<I-1>().nbins();
    scribe::write_values(o,(size_type)(n+2));
    n += spec::nover::value;
    if (!spec::under::value) scribe::write_values(o,(union_index_type)0);
    for (i_t& i = std::get<I-1>(ii) = 0; i<n; ++i) {
      scribe::write_values(o,(union_index_type)(I==1?1:2));
      write_bins<I-1>(o, h, ii);
    }
    if (!spec::over::value) scribe::write_values(o,(union_index_type)0);
  }
  template <unsigned I>
  static std::enable_if_t<I==0>
  write_bins(std::ostream& o, const hist& h, ii_t& ii) {
    scribe::write_values(o,h[ii]);
  }
public:
  static void write_value(std::ostream& o, const hist& h) {
    scribe::write_values(o,(size_type)sizeof...(Ax));
    scribe::write_values(o,h.axes());
    ii_t ii { };
    write_bins<sizeof...(Ax)>(o,h,ii);
  }
};

// Axes =============================================================

struct lin_axis;
struct list_axis;

template <>
struct trait<lin_axis> {
  static std::string type_name() { return "lin_axis"; }
  static std::string type_def() {
    std::stringstream s;
    s << "[\"" << trait<size_type>::type_name() << "\",\"nbins\"],"
         "[\"" << trait<double>::type_name() << "\",\"min\"],"
         "[\"" << trait<double>::type_name() << "\",\"max\"]";
    return s.str();
  }
};
template <>
struct trait<list_axis> {
  static std::string type_name() { return "list_axis"; }
  static std::string type_def() {
    std::stringstream s;
    s << "[\"" << trait<double>::type_name() << "#\",\"edges\"]";
    return s.str();
  }
};

template <typename Container, bool Inherit>
struct trait<ivanp::container_axis<Container,Inherit>>: trait<list_axis> {
  using axis = ivanp::container_axis<Container,Inherit>;
  static void write_value(std::ostream& o, const axis& a) {
    scribe::write_values(o,(union_index_type)1);
    scribe::write_values(o,a.edges());
  }
};
template <typename T, bool Inherit>
struct trait<ivanp::uniform_axis<T,Inherit>>: trait<lin_axis> {
  using axis = ivanp::uniform_axis<T,Inherit>;
  static void write_value(std::ostream& o, const axis& a) {
    scribe::write_values(o,(union_index_type)0);
    scribe::write_values(o,(size_type)a.nbins());
    scribe::write_values(o,(double)a.min());
    scribe::write_values(o,(double)a.max());
  }
};
template <typename T, typename Ref, bool Inherit>
struct trait<ivanp::ref_axis<T,Ref,Inherit>> {
  static std::string type_def() { return "[lin_axis,list_axis]"; }
  using axis = ivanp::ref_axis<T,Ref,Inherit>;
  static void write_value(std::ostream& o, const axis& a) {
    if (a.is_uniform()) {
      scribe::write_values(o,(union_index_type)0);
      scribe::write_values(o,(size_type)a.nbins());
      scribe::write_values(o,(double)a.min());
      scribe::write_values(o,(double)a.max());
    } else {
      scribe::write_values(o,(union_index_type)1);
      scribe::write_values(o,vector_of_edges<double>(a));
    }
  }
};
template <typename T>
struct trait<ivanp::abstract_axis<T>> {
  static std::string type_def() { return "[lin_axis,list_axis]"; }
  using axis = ivanp::abstract_axis<T>;
  static void write_value(std::ostream& o, const axis& a) {
    if (a.is_uniform()) {
      scribe::write_values(o,(union_index_type)0);
      scribe::write_values(o,(size_type)a.nbins());
      scribe::write_values(o,(double)a.min());
      scribe::write_values(o,(double)a.max());
    } else {
      scribe::write_values(o,(union_index_type)1);
      scribe::write_values(o,vector_of_edges<double>(a));
    }
  }
};

// Bin types ========================================================

template <typename T> using bin_type_t = typename T::bin_type;

template <typename T, typename... Args>
inline std::enable_if_t<is_detected<bin_type_t,T>::value>
add_bin_types(writer& w, Args&&... args) {
  w.add_type<T>();
  add_bin_types<typename T::bin_type>(w,std::forward<Args>(args)...);
}
template <typename T, typename... Args>
inline std::enable_if_t<!is_detected<bin_type_t,T>::value>
add_bin_types(writer& w, Args&&... args) {
  w.add_type<T>(std::forward<Args>(args)...);
}

template <typename Bin, typename E, typename... Es>
struct trait<category_bin<Bin,E,Es...>> {
  using type = category_bin<Bin,E,Es...>;

  static std::string type_name() { return enum_traits<E>::name(); }
  static std::string type_def() {
    std::stringstream s;
    s << "[\"" << trait<typename type::bin_type>::type_name();
    for (const char* name : enum_traits<E>::all_str())
      s << "\",\"" << name;
    s << "\"]";
    return s.str();
  }
  static void write_value(std::ostream& o, const type& x) {
    scribe::write_values(o,x.bins);
  }
};

template <typename T>
struct trait<nlo_bin<T>, std::enable_if_t<!std::is_array<T>::value>> {
  static std::string type_name() { return "weights"; }
  static std::string type_def() { return
    "[\"" + trait<T>::type_name() + "#2\",\"w\"],"
    "[\"" + trait<decltype(std::declval<nlo_bin<T>>().n)>::type_name()
    + "\",\"n\"]";
  }
  static void write_value(std::ostream& o, const nlo_bin<T>& bin) {
    // bin.finalize();
    scribe::write_values(o,bin.w,bin.w2+bin.wtmp*bin.wtmp,bin.n);
  }
};

template <typename T>
struct trait<nlo_bin<T>, std::enable_if_t<std::is_array<T>::value>> {
  using type = std::remove_extent_t<T>;
  static std::string type_name() { return "weights"; }
  static std::string type_def() { return
    "[\"" + trait<type>::type_name() + "#2#"
    + std::to_string(nlo_bin<T>::weights.size()) + "\",\"w\"],"
    "[\"" + trait<decltype(std::declval<nlo_bin<T>>().n)>::type_name()
    + "\",\"n\"]";
  }
  template <typename U>
  static std::string type_def(const std::vector<U>& weights_names) {
    std::stringstream s;
    s << "[\"" + trait<type>::type_name() + "#2";
    for (const auto& name : weights_names) s << "\",\"" << name;
    s << "\"],[\""
      << trait<decltype(std::declval<nlo_bin<T>>().n)>::type_name()
      << "\",\"n\"]";
    return s.str();
  }
  static void write_value(std::ostream& o, const nlo_bin<T>& bin) {
    // bin.finalize();
    for (const auto& w : bin.ws)
      scribe::write_values(o,w.w,w.w2+w.wtmp*w.wtmp);
    scribe::write_values(o,bin.n);
  }
};

}}

#endif
