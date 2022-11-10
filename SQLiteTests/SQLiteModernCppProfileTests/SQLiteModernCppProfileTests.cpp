#include <iostream>
#include <filesystem>
#include <chrono>

#include <SQLite.h>

using namespace ModernCppSQLite;

void SaveToDisk(SQLiteConnection const& source, const char* const filename)
{
  SQLiteConnection destination{ filename };
  SQLiteBackup backup{ destination, source };
  backup.Step();
}

int32_t main()
{
  try
  {
    if (std::filesystem::exists("Backup.db"))
    {
      SQLiteConnection connection{ "Backup.db" };

      int32_t argument_context = 0;

      connection.Profile([]([[maybe_unused]] int32_t* context, const char* const statement, const sqlite3_uint64 time)
        {
          std::chrono::seconds seconds{ time };
          auto miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(seconds);

          printf_s("Seconds: %lld, %s\n", seconds.count(), statement);
          printf_s("Miliseconds: %lld, %s\n", miliseconds.count(), statement);

        }, &argument_context);

      for (int32_t index = 0; const SQLiteRow & row : SQLiteStatement{ connection, "Select * From Things" })
      {
        printf_s("[%2d]: %.2f\n", ++index, row.GetDouble());
      }

      std::cout << "\n\n";
    }

    auto connection = SQLiteConnection::Memory();

    connection.Profile([]([[maybe_unused]] int32_t* context, const char* const statement, const uint64_t time)
      {
        std::chrono::seconds seconds{ time };
        auto miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(seconds);

        printf_s("Seconds: %lld, %s\n", seconds.count(), statement);
        printf_s("Miliseconds: %lld, %s\n", miliseconds.count(), statement);
      });

    Execute(connection, "Create Table Things ( Content Read )");

    SQLiteStatement statement(connection, "Insert Into Things Values (?)");

    for (int32_t value = 1; value <= 100'000; ++value)
    {
      statement.Bind(/*Column Index*/ 1, value);
      statement.Execute();

      statement.Reset();
    }

    Execute(connection, "Delete From Things Where Content > 10");

    Execute(connection, "Vacuum");

    SaveToDisk(connection, "Backup.db");
  }
  catch (const SQLiteException& ex)
  {
    std::clog << "Error Code: " << ex.ErrorCode << std::endl;
    std::clog << "Error Message: " << ex.ErrorMessage << std::endl;
  }
}
