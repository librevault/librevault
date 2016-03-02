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
#include <boost/filesystem.hpp>
#include <memory>
#include <map>

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

	union {
		int64_t int_val;
		double double_val;
		struct {
			union {
				const uint8_t* blob_val;
				const char* text_val;
			};
			uint64_t size;
		};
	};
public:
	SQLValue();	// Binds NULL value;
	SQLValue(int64_t int_val);	// Binds INT value;
	SQLValue(uint64_t int_val);	// Binds INT value;
	SQLValue(double double_val);	// Binds DOUBLE value;

	SQLValue(const std::string& text_val);	// Binds TEXT value;
	SQLValue(const char* text_val, uint64_t blob_size);	// Binds TEXT value;

	SQLValue(const std::vector<uint8_t>& blob_val);	// Binds BLOB value;
	SQLValue(const uint8_t* blob_ptr, uint64_t blob_size);	// Binds BLOB value;
	template<uint64_t array_size> SQLValue(std::array<uint8_t, array_size> blob_array) : SQLValue(blob_array.data(), blob_array.size()){}

	ValueType get_type(){return value_type;};

	bool is_null() const {return value_type == ValueType::NULL_VALUE;};
	int64_t as_int() const {return int_val;}
	uint64_t as_uint() const {return (uint64_t)int_val;}
	double as_double() const {return double_val;}
	std::string as_text() const {return std::string(text_val, text_val+size);}
	std::vector<uint8_t> as_blob() const {return std::vector<uint8_t>(blob_val, blob_val+size);}
	template<uint64_t array_size> std::array<uint8_t, array_size> as_blob() const {
		std::array<uint8_t, array_size> new_array; std::copy(blob_val, blob_val+std::min(size, array_size), new_array.data());
		return new_array;
	}

	operator bool() const {return !is_null();}
	operator int64_t() const {return int_val;};
	operator uint64_t() const {return (uint64_t)int_val;};
	operator double() const {return double_val;}
	operator std::string() const {return std::string(text_val, text_val+size);}
	operator std::vector<uint8_t>() const {return std::vector<uint8_t>(blob_val, blob_val+size);}
	template<uint64_t array_size> operator std::array<uint8_t, array_size>() const {
		return as_blob<array_size>();
	}
};

class SQLiteResultIterator : public std::iterator<std::input_iterator_tag, std::vector<SQLValue>> {
	sqlite3_stmt* prepared_stmt = 0;
	std::shared_ptr<int64_t> shared_idx;
	std::shared_ptr<std::vector<std::string>> cols;
	int64_t current_idx = 0;
	int rescode = SQLITE_OK;

	mutable std::vector<SQLValue> result;

	void fill_result() const;
public:
	SQLiteResultIterator(sqlite3_stmt* prepared_stmt, std::shared_ptr<int64_t> shared_idx, std::shared_ptr<std::vector<std::string>> cols, int rescode);
	SQLiteResultIterator(int rescode);

	SQLiteResultIterator& operator++();
	SQLiteResultIterator operator++(int);
	bool operator==(const SQLiteResultIterator& lvalue);
	bool operator!=(const SQLiteResultIterator& lvalue);
	const value_type& operator*() const;
	const value_type* operator->() const;

	SQLValue operator[](size_t pos) const;

	int result_code() const {return rescode;};
};

class SQLiteResult {
	int rescode = SQLITE_OK;

	sqlite3_stmt* prepared_stmt = 0;
	std::shared_ptr<int64_t> shared_idx;
	std::shared_ptr<std::vector<std::string>> cols;
public:
	SQLiteResult(sqlite3_stmt* prepared_stmt);
	virtual ~SQLiteResult();

	void finalize();

	SQLiteResultIterator begin();
	SQLiteResultIterator end();

	int result_code() const {return rescode;};
	bool have_rows(){return result_code() == SQLITE_ROW;}
	std::vector<std::string> column_names(){return *cols;};
};

class SQLiteDB {
public:
	SQLiteDB(){};
	SQLiteDB(const boost::filesystem::path& db_path);
	SQLiteDB(const char* db_path);
	virtual ~SQLiteDB();

	void open(const boost::filesystem::path& db_path);
	void open(const char* db_path);
	void close();

	sqlite3* sqlite3_handle(){return db;};

	SQLiteResult exec(const std::string& sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());

	int64_t last_insert_rowid();
private:
	sqlite3* db = 0;
};

class SQLiteSavepoint {
public:
	SQLiteSavepoint(SQLiteDB& db, const std::string savepoint_name);
	SQLiteSavepoint(SQLiteDB* db, const std::string savepoint_name);
	~SQLiteSavepoint();
private:
	SQLiteDB& db;
	const std::string name;
};

class SQLiteLock {
public:
	SQLiteLock(SQLiteDB& db);
	SQLiteLock(SQLiteDB* db);
	~SQLiteLock();
private:
	SQLiteDB& db;
};

} /* namespace librevault */
