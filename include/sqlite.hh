#ifndef IVANP_SQLITE_HH
#define IVANP_SQLITE_HH

#include <sqlite3.h>
#include <exception>
#include <type_traits>

namespace ivanp {

class sqlite {
  sqlite3 *db;

  struct error: std::exception {
  private:
    const char* msg;
  public:
    error(): msg(nullptr) { }
    error(const char* msg): msg(msg) { }
    error(const error&) = delete;
    error(error&& r) noexcept: msg(r.msg) { r.msg = nullptr; }
    error& operator=(const error&) = delete;
    error& operator=(error&& r) noexcept {
      msg = r.msg;
      r.msg = nullptr;
      return *this;
    }
    // ~error() { if (msg) sqlite3_free(msg); }
    const char* what() const noexcept { return msg; }
  };

  template <typename F>
  static int exec_callback(
    void* arg, // 4th argument of sqlite3_exec
    int ncol, // number of columns
    char** row, // pointers to strings obtained as if from sqlite3_column_text()
    char** cols_names // names of columns
  ) {
    (*reinterpret_cast<F*>(arg))(ncol,row,cols_names);
    return 0;
  }

public:
  const char* errmsg() const { return sqlite3_errmsg(db); }

  sqlite(const char* fname) {
    if (sqlite3_open(fname,&db) != SQLITE_OK)
      throw error(errmsg());
  }
  ~sqlite() { sqlite3_close(db); }

  sqlite(const sqlite&) = delete;
  sqlite(sqlite&& r) noexcept: db(r.db) { r.db = nullptr; }
  sqlite& operator=(const sqlite&) = delete;
  sqlite& operator=(sqlite&& r) {
    db = r.db;
    r.db = nullptr;
    return *this;
  }

  sqlite& exec(const char* sql) {
    char* err;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK)
      throw error(err);
    return *this;
  }
  template <typename F>
  sqlite& exec(const char* sql, F&& f) {
    char* err;
    if (sqlite3_exec( db, sql,
          exec_callback<F>, reinterpret_cast<void*>(&f), &err
    ) != SQLITE_OK) throw error(err);
    return *this;
  }

  sqlite& operator()(const char* sql) {
    return exec(sql);
  }
  template <typename F>
  sqlite& operator()(const char* sql, F&& f) {
    return exec(sql,std::forward<F>(f));
  }

  class stmt {
    sqlite3_stmt *p = nullptr;

  public:
    stmt(sqlite3 *db, const char* sql) {
      if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK)
        throw error(sqlite3_errmsg(db));
    }
    ~stmt() { sqlite3_finalize(p); }

    stmt(const stmt&) = delete;
    stmt(stmt&& r) noexcept: p(r.p) { r.p = nullptr; }
    stmt& operator=(const stmt&) = delete;
    stmt& operator=(stmt&& r) {
      p = r.p;
      r.p = nullptr;
      return *this;
    }

    sqlite3* db_handle() const { return sqlite3_db_handle(p); }
    const char* errmsg() const { return sqlite3_errmsg(db_handle()); }

    void finalize() {
      if (p) {
        const auto err = sqlite3_finalize(p);
        if (err != SQLITE_OK) throw error(sqlite3_errstr(err));
        p = nullptr;
      }
    }
    bool step() {
      switch (sqlite3_step(p)) {
        case SQLITE_ROW: return true;
        case SQLITE_DONE: return false;
        default: throw error(errmsg());
      }
    }
    stmt& reset() {
      if (sqlite3_reset(p) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }

    // bind ---------------------------------------------------------
    stmt& bind(int i, double x) {
      if (sqlite3_bind_double(p, i, x) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i, int x) {
      if (sqlite3_bind_int(p, i, x) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i, sqlite3_int64 x) {
      if (sqlite3_bind_int64(p, i, x) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i) {
      if (sqlite3_bind_null(p, i) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i, const char* x, bool trans=true) {
      if (sqlite3_bind_text(p, i, x, -1,
            trans ? SQLITE_TRANSIENT : SQLITE_STATIC
      ) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i, const void* x, int n, bool trans=true) {
      if (sqlite3_bind_blob(p, i, x, n,
            trans ? SQLITE_TRANSIENT : SQLITE_STATIC
      ) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }
    stmt& bind(int i, std::nullptr_t, int n) {
      if (sqlite3_bind_zeroblob(p, i, n) != SQLITE_OK)
        throw error(errmsg());
      return *this;
    }

    template <typename T>
    auto bind(int i, const T& x, bool trans=true) -> std::enable_if_t<
      std::is_convertible<decltype(x.data()),const char*>::value,
      stmt
    >& {
      return bind(i,static_cast<const char*>(x.data()),trans);
    }

    // column -------------------------------------------------------
    int column_count() {
      return sqlite3_column_count(p);
    }
    double column_double(int i) {
      return sqlite3_column_double(p, i);
    }
    int column_int(int i) {
      return sqlite3_column_int(p, i);
    }
    sqlite3_int64 column_int64(int i) {
      return sqlite3_column_int64(p, i);
    }
    auto column_text(int i) {
      return sqlite3_column_text(p, i);
    }
    const void* column_blob(int i) {
      return sqlite3_column_blob(p, i);
    }
    int column_bytes(int i) {
      return sqlite3_column_bytes(p, i);
    }
    int column_type(int i) {
      // SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, or SQLITE_NULL
      return sqlite3_column_type(p, i);
    }
  };

  stmt prepare(const char* sql) { return { db, sql }; }
};

}

#endif
