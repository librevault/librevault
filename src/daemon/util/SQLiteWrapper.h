/* Written in 2015 by Alexander Shishenko <alex@shishenko.com>
 *
 * LVSQLite - SQLite wrapper, used in Librevault.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#pragma once
#include <sqlite3.h>

#include <QtCore/QVariant>
#include <boost/filesystem/path.hpp>
#include <map>
#include <memory>

#include "util/blob.h"

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
  SQLValue();                   // Binds NULL value;
  SQLValue(int64_t int_val);    // Binds INT value;
  SQLValue(uint64_t int_val);   // Binds INT value;
  SQLValue(double double_val);  // Binds DOUBLE value;

  SQLValue(const std::string& text_val);  // Binds TEXT value;

  SQLValue(const std::vector<uint8_t>& blob_val);         // Binds BLOB value;
  SQLValue(const uint8_t* blob_ptr, uint64_t blob_size);  // Binds BLOB value;
  template <uint64_t array_size>
  SQLValue(std::array<uint8_t, array_size> blob_array) : SQLValue(blob_array.data(), blob_array.size()) {}

  ValueType get_type() { return value_type; };

  bool is_null() const { return value_type == ValueType::NULL_VALUE; };
  int64_t as_int() const { return value.toLongLong(); }
  uint64_t as_uint() const { return value.toULongLong(); }
  double as_double() const { return value.toDouble(); }
  std::string as_text() const { return value.toString().toStdString(); }
  std::vector<uint8_t> as_blob() const { return conv_bytearray(value.toByteArray()); }
  template <uint64_t array_size>
  std::array<uint8_t, array_size> as_blob() const {
    auto blob_val = conv_bytearray(value.toByteArray());
    std::array<uint8_t, array_size> new_array;
    std::copy(blob_val.data(), blob_val.data() + std::min((uint64_t)blob_val.size(), array_size), new_array.data());
    return new_array;
  }

  operator bool() const { return !is_null(); }
  operator int64_t() const { return as_int(); };
  operator uint64_t() const { return as_uint(); };
  operator double() const { return as_double(); }
  operator std::string() const { return as_text(); }
  operator std::vector<uint8_t>() const { return as_blob(); }
  template <uint64_t array_size>
  operator std::array<uint8_t, array_size>() const {
    return as_blob<array_size>();
  }
};

class SQLiteResultIterator : public std::iterator<std::input_iterator_tag, std::vector<SQLValue>> {
  sqlite3_stmt* prepared_stmt = nullptr;
  std::shared_ptr<int64_t> shared_idx;
  std::shared_ptr<std::vector<std::string>> cols;
  int64_t current_idx = 0;
  int rescode = SQLITE_OK;

  mutable std::vector<SQLValue> result;

  void fill_result() const;

 public:
  SQLiteResultIterator(sqlite3_stmt* prepared_stmt, std::shared_ptr<int64_t> shared_idx,
                       std::shared_ptr<std::vector<std::string>> cols, int rescode);
  SQLiteResultIterator(int rescode);

  SQLiteResultIterator& operator++();
  SQLiteResultIterator operator++(int);
  bool operator==(const SQLiteResultIterator& lvalue);
  bool operator!=(const SQLiteResultIterator& lvalue);
  const value_type& operator*() const;
  const value_type* operator->() const;

  SQLValue operator[](size_t pos) const;
  ;
};

class SQLiteResult {
  int rescode = SQLITE_OK;

  sqlite3_stmt* prepared_stmt = nullptr;
  std::shared_ptr<int64_t> shared_idx;
  std::shared_ptr<std::vector<std::string>> cols;

 public:
  SQLiteResult(sqlite3_stmt* prepared_stmt);
  virtual ~SQLiteResult();

  void finalize();

  SQLiteResultIterator begin();
  SQLiteResultIterator end();

  bool have_rows() const { return rescode == SQLITE_ROW; }
};

class SQLiteDB {
 public:
  SQLiteDB() = default;
  SQLiteDB(const boost::filesystem::path& db_path);
  virtual ~SQLiteDB();

  SQLiteResult exec(const std::string& sql, const std::map<QString, SQLValue>& values = std::map<QString, SQLValue>());

 private:
  sqlite3* db = nullptr;
};

class SQLiteSavepoint {
 public:
  SQLiteSavepoint(SQLiteDB& db, QString savepoint_name);
  ~SQLiteSavepoint();

  void commit();

 private:
  SQLiteDB& db;
  const QString name;
};

}  // namespace librevault
