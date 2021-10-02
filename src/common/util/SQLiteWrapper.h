#pragma once
#include <sqlite3.h>

#include <QtCore/QVariant>
#include <map>
#include <memory>

namespace librevault {

class SQLValue {
 public:
  enum ValueType {
    INT = SQLITE_INTEGER,
    DOUBLE = SQLITE_FLOAT,
    TEXT = SQLITE_TEXT,
    BLOB = SQLITE_BLOB,
    NULL_VALUE = SQLITE_NULL
  };

 protected:
  ValueType value_type;
  QVariant value;

 public:
  SQLValue();                            // Binds NULL value;
  SQLValue(int64_t int_val);             // Binds INT value;
  SQLValue(uint64_t int_val);            // Binds INT value;
  SQLValue(double double_val);           // Binds DOUBLE value;
  SQLValue(const QString& text_val);     // Binds TEXT value;
  SQLValue(const QByteArray& blob_val);  // Binds BLOB value;

  ValueType type() { return value_type; };

  [[nodiscard]] bool isNull() const { return value_type == ValueType::NULL_VALUE; };
  [[nodiscard]] int64_t toInt() const { return value.toLongLong(); }
  [[nodiscard]] uint64_t toUInt() const { return value.toULongLong(); }
  [[nodiscard]] double toDouble() const { return value.toDouble(); }
  [[nodiscard]] QString toString() const { return value.toString(); }
  [[nodiscard]] QByteArray toByteArray() const { return value.toByteArray(); }

  operator bool() const { return !isNull(); }
  operator int64_t() const { return toInt(); };
  operator uint64_t() const { return toUInt(); };
  operator double() const { return toDouble(); }
  operator QString() const { return toString(); }
  operator QByteArray() const { return toByteArray(); }
};

class SQLiteResultIterator {
  sqlite3_stmt* prepared_stmt = nullptr;
  std::shared_ptr<int64_t> shared_idx;
  QVector<QString> cols;
  int64_t current_idx = 0;
  int rescode = SQLITE_OK;

  mutable std::vector<SQLValue> result;

  void fillResult() const;

 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = std::vector<SQLValue>;

  SQLiteResultIterator(sqlite3_stmt* prepared_stmt, std::shared_ptr<int64_t> shared_idx, QVector<QString> cols,
                       int rescode);
  explicit SQLiteResultIterator(int rescode);

  SQLiteResultIterator& operator++();
  bool operator==(const SQLiteResultIterator& lvalue);
  bool operator!=(const SQLiteResultIterator& lvalue);
  const value_type& operator*() const;
  const value_type* operator->() const;

  SQLValue operator[](size_t pos) const;
};

class SQLiteResult {
  int rescode = SQLITE_OK;

  sqlite3_stmt* prepared_stmt = nullptr;
  std::shared_ptr<int64_t> shared_idx;
  QVector<QString> cols;

 public:
  explicit SQLiteResult(sqlite3_stmt* prepared_stmt);
  virtual ~SQLiteResult();

  void finalize();

  SQLiteResultIterator begin();
  SQLiteResultIterator end();

  [[nodiscard]] bool have_rows() const { return rescode == SQLITE_ROW; }
};

class SQLiteDB {
 public:
  explicit SQLiteDB(const QString& db_path);
  virtual ~SQLiteDB();

  SQLiteResult exec(const QString& sql, const std::map<QString, SQLValue>& values = std::map<QString, SQLValue>());

 private:
  sqlite3* db = nullptr;
};

class SQLiteSavepoint {
 public:
  SQLiteSavepoint(SQLiteDB& db);
  ~SQLiteSavepoint();

  void commit();

 private:
  SQLiteDB& db;
  const QString name;
};

}  // namespace librevault
