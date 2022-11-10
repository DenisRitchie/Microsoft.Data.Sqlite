#pragma once

#include "Handle.h"

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>
#elif __has_include(<winsqlite/winsqlite3.h>)
#include <winsqlite/winsqlite3.h>
#else
#error The content of <sqlite3.h> must be installed.
#endif

#include <string>
#include <string_view>

#ifdef _DEBUG
#define VERIFY ASSERT
#define VERIFY_(result, expression) ASSERT(result == expression)
#else
#define VERIFY(expression) (expression)
#define VERIFY_(result, expression) (expression)
#endif

namespace ModernCppSQLite
{
  enum class SQLiteType
  {
    Integer = SQLITE_INTEGER,
    Float = SQLITE_FLOAT,
    Blob = SQLITE_BLOB,
    Null = SQLITE_NULL,
    Text = SQLITE_TEXT,
  };

  inline constexpr const char* SQLiteTypeName(SQLiteType const type) noexcept
  {
    switch (type)
    {
      case SQLiteType::Integer: return "Integer";
      case SQLiteType::Float: return "Float";
      case SQLiteType::Blob: return "Blob";
      case SQLiteType::Null: return "Null";
      case SQLiteType::Text: return "Text";
    }

    ASSERT(false);
    return "Invalid";
  }

  inline constexpr const char8_t* SQLiteU8TypeName(SQLiteType const type) noexcept
  {
    switch (type)
    {
      case SQLiteType::Integer: return u8"Integer";
      case SQLiteType::Float: return u8"Float";
      case SQLiteType::Blob: return u8"Blob";
      case SQLiteType::Null: return u8"Null";
      case SQLiteType::Text: return u8"Text";
    }

    ASSERT(false);
    return u8"Invalid";
  }

  inline constexpr const char16_t* SQLiteU16TypeName(SQLiteType const type) noexcept
  {
    switch (type)
    {
      case SQLiteType::Integer: return u"Integer";
      case SQLiteType::Float: return u"Float";
      case SQLiteType::Blob: return u"Blob";
      case SQLiteType::Null: return u"Null";
      case SQLiteType::Text: return u"Text";
    }

    ASSERT(false);
    return u"Invalid";
  }

  struct SQLiteException
  {
    int32_t ErrorCode = 0;
    std::string ErrorMessage;

    explicit SQLiteException(sqlite3* const connection)
      : ErrorCode(sqlite3_extended_errcode(connection))
      , ErrorMessage(sqlite3_errmsg(connection))
    {
    }
  };

  class SQLiteConnection
  {
  private:
    struct SQLiteConnectionHandleTraits : SQLiteHandleTraits<sqlite3*>
    {
      static void Close(Type value) noexcept
      {
        VERIFY_(SQLITE_OK, sqlite3_close(value));
      }
    };

    using SQLiteConnectionHandle = SQLiteHandle<SQLiteConnectionHandleTraits>;

    template <typename F, typename C>
    void InternalOpen(F open, C const* const filename)
    {
      SQLiteConnection temp;

      if (SQLITE_OK != open(filename, temp.m_Handle.Set()))
      {
        temp.ThrowLastError();
      }

      swap(m_Handle, temp.m_Handle);
    }

  public:

    SQLiteConnection() noexcept = default;

    template <typename C>
    explicit SQLiteConnection(C const* const filename)
    {
      Open(filename);
    }

    static SQLiteConnection Memory()
    {
      return SQLiteConnection(":memory:");
    }

    static SQLiteConnection WideMemory()
    {
      return SQLiteConnection(L":memory:");
    }

    [[noreturn]] void ThrowLastError() const
    {
      throw SQLiteException(GetAbi());
    }

    explicit operator bool() const noexcept
    {
      return static_cast<bool>(m_Handle);
    }

    sqlite3* GetAbi() const noexcept
    {
      return m_Handle.Get();
    }

    void Open(char const* const filename)
    {
      InternalOpen(sqlite3_open, filename);
    }

    void Open(wchar_t const* const filename)
    {
      InternalOpen(sqlite3_open16, filename);
    }

    void Open(char8_t const* const filename)
    {
      InternalOpen(sqlite3_open, (char const* const)filename);
    }

    void Open(char16_t const* const filename)
    {
      InternalOpen(sqlite3_open16, filename);
    }

    sqlite3_int64 RowId() const noexcept
    {
      return sqlite3_last_insert_rowid(GetAbi());
    }

    template <typename F>
    void Profile(F callback, void* const context = nullptr)
    {
      ::sqlite3_profile(GetAbi(), callback, context);
    }

  private:
    SQLiteConnectionHandle m_Handle;
  };


  class SQLiteBackup
  {
  private:
    struct SQLiteBackupHandleTraits : SQLiteHandleTraits<sqlite3_backup*>
    {
      static void Close(Type value) noexcept
      {
        sqlite3_backup_finish(value);
      }
    };

    using SQLiteBackupHandle = SQLiteHandle<SQLiteBackupHandleTraits>;

  public:
    SQLiteBackup(
      SQLiteConnection const& destination,
      SQLiteConnection const& source,
      char const* const destinationName = "main",
      char const* const sourceName = "main")
      : m_Handle(sqlite3_backup_init(destination.GetAbi(), destinationName, source.GetAbi(), sourceName))
      , m_Destination(&destination)
    {
      if (!m_Handle)
      {
        destination.ThrowLastError();
      }
    }

    sqlite3_backup* GetAbi() const noexcept
    {
      return m_Handle.Get();
    }

    bool Step(int32_t const pages = -1)
    {
      int32_t const result = sqlite3_backup_step(GetAbi(), pages);

      if (result == SQLITE_OK) return true;
      if (result == SQLITE_DONE) return false;

      // Reset() calls sqlite3_backup_finish() so that error information 
      // will be made available through the destination connection.
      m_Handle.Reset();
      m_Destination->ThrowLastError();

      // throw Exception(result);
    }

  private:
    SQLiteBackupHandle m_Handle;
    SQLiteConnection const* m_Destination = nullptr;
  };

  template <typename T>
  struct SQLiteReader
  {
    int32_t GetInt32(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_int(static_cast<T const*>(this)->GetAbi(), column);
    }

    sqlite3_int64 GetInt64(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_int64(static_cast<T const*>(this)->GetAbi(), column);
    }

    double GetDouble(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_double(static_cast<T const*>(this)->GetAbi(), column);
    }

    std::byte const* GetBlob(int32_t const column = 0) const noexcept
    {
      return static_cast<std::byte const*>(::sqlite3_column_blob(static_cast<T const*>(this)->GetAbi(), column));
    }

    char const* GetString(int32_t const column = 0) const noexcept
    {
      return reinterpret_cast<char const*>(::sqlite3_column_text(static_cast<T const*>(this)->GetAbi(), column));
    }

    wchar_t const* GetWideString(int32_t const column = 0) const noexcept
    {
      return static_cast<wchar_t const*>(::sqlite3_column_text16(static_cast<T const*>(this)->GetAbi(), column));
    }

    char8_t const* GetU8String(int32_t const column = 0) const noexcept
    {
      return static_cast<char8_t const*>(::sqlite3_column_text(static_cast<T const*>(this)->GetAbi(), column));
    }

    char16_t const* GetU16String(int32_t const column = 0) const noexcept
    {
      return static_cast<char16_t const*>(::sqlite3_column_text16(static_cast<T const*>(this)->GetAbi(), column));
    }

    int32_t GetBlobLength(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_bytes(static_cast<T const*>(this)->GetAbi(), column);
    }

    int32_t GetStringLength(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_bytes(static_cast<T const*>(this)->GetAbi(), column);
    }

    int32_t GetWideStringLength(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_bytes16(static_cast<T const*>(this)->GetAbi(), column) / sizeof(wchar_t);
    }

    int32_t GetU8StringLength(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_bytes(static_cast<T const*>(this)->GetAbi(), column) / sizeof(char8_t);
    }

    int32_t GetU16StringLength(int32_t const column = 0) const noexcept
    {
      return ::sqlite3_column_bytes16(static_cast<T const*>(this)->GetAbi(), column) / sizeof(char16_t);
    }

    SQLiteType GetType(int32_t const column = 0) const noexcept
    {
      return static_cast<SQLiteType>(::sqlite3_column_type(static_cast<T const*>(this)->GetAbi(), column));
    }
  };

  class SQLiteStatement : public SQLiteReader<SQLiteStatement>
  {
  private:
    struct SQLiteStatementHandleTraits : SQLiteHandleTraits<sqlite3_stmt*>
    {
      static void Close(Type value) noexcept
      {
        sqlite3_finalize(value);
      }
    };

    using SQLiteStatementHandle = SQLiteHandle<SQLiteStatementHandleTraits>;

    template <typename F, typename C, typename ... Values>
    void InternalPrepare(SQLiteConnection const& connection, F prepare, C const* const text, Values && ... values)
    {
      ASSERT(connection);

      if (SQLITE_OK != prepare(connection.GetAbi(), text, -1, m_Handle.Set(), nullptr))
      {
        connection.ThrowLastError();
      }

      BindAll(std::forward<Values>(values) ...);
    }

    [[deprecated("Use the InternalBindCpp17 method.")]]
    void InternalBind(int) const noexcept
    {
    }

    template <typename First, typename ... Rest>
    [[deprecated("Use the InternalBindCpp17 method.")]]
    void InternalBind(int32_t const index, First&& first, Rest && ... rest) const
    {
      Bind(index, std::forward<First>(first));
      InternalBind(index + 1, std::forward<Rest>(rest) ...);
    }

    template <typename... Values>
    inline void InternalBindCpp17(Values &&... values) const
    {
      [&] <size_t... Index>(std::index_sequence<Index...>)
      {
        (Bind(Index + 1, std::forward<Values>(values)), ...);
      }(std::index_sequence_for<Values...>{ });
    }

  public:
    SQLiteStatement() noexcept = default;

    template <typename C, typename ... Values>
    SQLiteStatement(SQLiteConnection const& connection, C const* const text, Values && ... values)
    {
      Prepare(connection, text, std::forward<Values>(values) ...);
    }

    explicit operator bool() const noexcept
    {
      return static_cast<bool>(m_Handle);
    }

    sqlite3_stmt* GetAbi() const noexcept
    {
      return m_Handle.Get();
    }

    [[noreturn]] void ThrowLastError() const
    {
      throw SQLiteException(sqlite3_db_handle(GetAbi()));
    }

    template <typename ... Values>
    void Prepare(SQLiteConnection const& connection, char const* const text, Values && ... values)
    {
      InternalPrepare(connection, sqlite3_prepare_v2, text, std::forward<Values>(values) ...);
    }

    template <typename ... Values>
    void Prepare(SQLiteConnection const& connection, wchar_t const* const text, Values && ... values)
    {
      InternalPrepare(connection, sqlite3_prepare16_v2, text, std::forward<Values>(values) ...);
    }

    template <typename ... Values>
    void Prepare(SQLiteConnection const& connection, char8_t const* const text, Values && ... values)
    {
      InternalPrepare(connection, sqlite3_prepare_v2, (char const* const)text, std::forward<Values>(values) ...);
    }

    template <typename ... Values>
    void Prepare(SQLiteConnection const& connection, char16_t const* const text, Values && ... values)
    {
      InternalPrepare(connection, sqlite3_prepare16_v2, text, std::forward<Values>(values) ...);
    }

    bool Step() const noexcept(noexcept(sqlite3_step(GetAbi()) == SQLITE_ROW || sqlite3_step(GetAbi()) == SQLITE_DONE))
    {
      int32_t const result = sqlite3_step(GetAbi());

      if (result == SQLITE_ROW) return true;
      if (result == SQLITE_DONE) return false;

      ThrowLastError();
    }

    void Execute() const
    {
      VERIFY(!Step());
    }

    void Reset() const
    {
      if (SQLITE_OK != sqlite3_reset(GetAbi()))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, int32_t const value) const
    {
      if (SQLITE_OK != sqlite3_bind_int(GetAbi(), index, value))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, double const value) const
    {
      if (SQLITE_OK != sqlite3_bind_double(GetAbi(), index, value))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, char const* const value, int32_t const size = -1) const
    {
      if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, value, size, SQLITE_STATIC))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, wchar_t const* const value, int32_t const size = -1) const
    {
      if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value, size, SQLITE_STATIC))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, char8_t const* const value, int32_t const size = -1) const
    {
      if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, (char const* const)value, size, SQLITE_STATIC))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, char16_t const* const value, int32_t const size = -1) const
    {
      if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value, size, SQLITE_STATIC))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, std::string const& value) const
    {
      Bind(index, value.c_str(), static_cast<int32_t>(value.size()));
    }

    void Bind(int32_t const index, std::wstring const& value) const
    {
      Bind(index, value.c_str(), static_cast<int32_t>(value.size() * sizeof(wchar_t)));
    }

    void Bind(int32_t const index, std::u8string const& value) const
    {
      Bind(index, value.c_str(), static_cast<int32_t>(value.size() * sizeof(char8_t)));
    }

    void Bind(int32_t const index, std::u16string const& value) const
    {
      Bind(index, value.c_str(), static_cast<int32_t>(value.size() * sizeof(char16_t)));
    }

    void Bind(int32_t const index, std::string&& value) const
    {
      if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, value.c_str(), static_cast<int32_t>(value.size()), SQLITE_TRANSIENT))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, std::wstring&& value) const
    {
      if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value.c_str(), static_cast<int32_t>(value.size() * sizeof(wchar_t)), SQLITE_TRANSIENT))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, std::u8string&& value) const
    {
      if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, (char const* const)value.c_str(), static_cast<int32_t>(value.size() * sizeof(char8_t)), SQLITE_TRANSIENT))
      {
        ThrowLastError();
      }
    }

    void Bind(int32_t const index, std::u16string&& value) const
    {
      if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value.c_str(), static_cast<int32_t>(value.size() * sizeof(char16_t)), SQLITE_TRANSIENT))
      {
        ThrowLastError();
      }
    }

    template <typename ... Values>
    void BindAll(Values && ... values)
    {
      InternalBindCpp17(std::forward<Values>(values)...);
    }

  private:
    SQLiteStatementHandle m_Handle;
  };

  class SQLiteRow : public SQLiteReader<SQLiteRow>
  {
  public:
    sqlite3_stmt* GetAbi() const noexcept
    {
      return m_Statement;
    }

    SQLiteRow(sqlite3_stmt* const statement) noexcept : m_Statement(statement)
    {
    }

  private:
    sqlite3_stmt* m_Statement = nullptr;
  };


  class SQLiteRowIterator
  {
  public:
    SQLiteRowIterator() noexcept = default;

    SQLiteRowIterator(SQLiteStatement const& statement) noexcept
    {
      if (statement.Step())
      {
        m_Statement = &statement;
      }
    }

    SQLiteRowIterator& operator++() noexcept
    {
      if (!m_Statement->Step())
      {
        m_Statement = nullptr;
      }

      return *this;
    }

    bool operator !=(SQLiteRowIterator const& other) const noexcept
    {
      return m_Statement != other.m_Statement;
    }

    SQLiteRow operator *() const noexcept
    {
      return SQLiteRow(m_Statement->GetAbi());
    }

  private:
    SQLiteStatement const* m_Statement = nullptr;
  };

  inline SQLiteRowIterator begin(SQLiteStatement const& statement) noexcept
  {
    return SQLiteRowIterator(statement);
  }

  inline SQLiteRowIterator end(SQLiteStatement const&) noexcept
  {
    return SQLiteRowIterator();
  }

  template <typename C, typename ... Values>
  inline void Execute(SQLiteConnection const& connection, C const* const text, Values && ... values)
  {
    SQLiteStatement(connection, text, std::forward<Values>(values) ...).Execute();
  }
}
