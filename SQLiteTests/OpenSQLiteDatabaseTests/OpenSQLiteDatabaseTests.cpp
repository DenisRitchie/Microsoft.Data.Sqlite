/*****************************************************************//**
 * \file   OpenSQLiteDatabaseTests.cpp
 * \brief  Ejemplo básico de SQLite con C++ moderno
 *
 * \author Denis West
 * \date   November 2022
 *********************************************************************/

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>
#elif __has_include(<winsqlite/winsqlite3.h>)
#include <winsqlite/winsqlite3.h>
#else
#include "../../Library/sqlite3.h"
#include "../../Library/sqlite3.c"
#endif

#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

class SQLiteException
{
public:
  constexpr explicit SQLiteException(const int32_t error_code, const std::string_view message) noexcept
    : m_Message{ message }
    , m_ErrorCode{ error_code }
  {
  }

  constexpr std::string_view Message() const noexcept
  {
    return m_Message;
  }

  constexpr int32_t ErrorCode() const noexcept
  {
    return m_ErrorCode;
  }

private:
  int32_t m_ErrorCode;
  std::string m_Message;
};

class SQLite;

class SQLiteRow
{
public:
  SQLiteRow(sqlite3_stmt* statement) noexcept : m_Statement{ statement }
  {
  }

  const std::string_view Text(const int32_t column_index) noexcept
  {
    return reinterpret_cast<const char*>(sqlite3_column_text(m_Statement, column_index));
  }

  const int32_t Number(const int32_t column_index) noexcept
  {
    return sqlite3_column_int(m_Statement, column_index);
  }

private:
  sqlite3_stmt* m_Statement;
};

class SQLiteReader
{
public:
  using iterator_concept  /**/ = std::input_iterator_tag;
  using iterator_category /**/ = std::input_iterator_tag;
  using difference_type   /**/ = ptrdiff_t;
  using value_type        /**/ = SQLiteRow;
  using reference         /**/ = SQLiteRow&;
  using pointer           /**/ = SQLiteRow*;

  SQLiteReader(sqlite3_stmt* statement) noexcept
    : m_Statement{ statement }
    , m_StatusCode{ SQLITE_DONE }
    , m_Value{ statement }
  {
    operator++();
  }

  reference operator*()
  {
    return m_Value;
  }

  pointer operator->()
  {
    return &m_Value;
  }

  SQLiteReader& operator++()
  {
    m_StatusCode = sqlite3_step(m_Statement);
    return *this;
  }

  SQLiteReader operator++(int)
  {
    SQLiteReader temp_reader = *this;
    m_StatusCode = sqlite3_step(m_Statement);
    return temp_reader;
  }

  friend bool operator==(const SQLiteReader& reader, std::default_sentinel_t)
  {
    return reader.m_StatusCode == SQLITE_ROW;
  }

  friend bool operator==(std::default_sentinel_t, const SQLiteReader& reader)
  {
    return reader.m_StatusCode == SQLITE_ROW;
  }

  friend bool operator!=(const SQLiteReader& reader, std::default_sentinel_t)
  {
    return reader.m_StatusCode != SQLITE_DONE;
  }

  friend bool operator!=(std::default_sentinel_t, const SQLiteReader& reader)
  {
    return reader.m_StatusCode != SQLITE_DONE;
  }

private:
  int32_t m_StatusCode;
  sqlite3_stmt* m_Statement;
  value_type m_Value;
};

using SQLiteReaderIterator = std::common_iterator<SQLiteReader, std::default_sentinel_t>;

class SQLiteStatement
{
  friend class SQLiteReader;

public:
  SQLiteStatement(SQLite& sqlite) noexcept;
  ~SQLiteStatement() noexcept;
  void Prepare(const std::string_view query);
  SQLiteReader ExecuteReader();

private:
  SQLite& m_SQLite;
  sqlite3_stmt* m_Statement;
};

class SQLite
{
  friend class SQLiteStatement;

public:
  SQLite() noexcept : m_Connection{ nullptr }
  {
  }

  void Open(const std::string_view file_name)
  {
    const int32_t status_code = sqlite3_open(file_name.data(), &m_Connection);

    if (status_code != SQLITE_OK)
    {
      throw SQLiteException(status_code, sqlite3_errmsg(m_Connection));
    }
  }

  void Close() noexcept
  {
    sqlite3_close(m_Connection);
  }

  std::unique_ptr<SQLiteStatement> CreateStatement() const noexcept
  {
    return std::make_unique<SQLiteStatement>(const_cast<SQLite&>(*this));
  }

  ~SQLite() noexcept
  {
    Close();
  }

private:
  sqlite3* m_Connection;
};

SQLiteStatement::SQLiteStatement(SQLite& sqlite) noexcept
  : m_SQLite{ sqlite }
  , m_Statement{ nullptr }
{
}

SQLiteStatement::~SQLiteStatement() noexcept
{
  sqlite3_finalize(m_Statement);
}

void SQLiteStatement::Prepare(const std::string_view query)
{
  const int32_t status_code = sqlite3_prepare_v2(m_SQLite.m_Connection, query.data(), -1, &m_Statement, nullptr);

  if (status_code != SQLITE_OK)
  {
    throw SQLiteException(status_code, sqlite3_errmsg(m_SQLite.m_Connection));
  }
}

SQLiteReader SQLiteStatement::ExecuteReader()
{
  return { m_Statement };
}

int main()
{
  try
  {
    SQLite sqlite;
    sqlite.Open(":memory:");

    auto statement = sqlite.CreateStatement();
    statement->Prepare("select 'Hello world!'");

    auto reader = statement->ExecuteReader();

    int32_t index = 0;
    while (reader != std::default_sentinel)
    {
      std::cout << "Column[" << ++index << "]: " << reader->Text(0) << std::endl;
      ++reader;
    }
  }
  catch (const SQLiteException& ex)
  {
    std::clog << "Error Code:    " << ex.ErrorCode() << std::endl;
    std::clog << "Error Message: " << ex.Message() << std::endl;
  }
}