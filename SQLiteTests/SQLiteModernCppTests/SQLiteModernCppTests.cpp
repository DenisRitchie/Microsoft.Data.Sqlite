#include <iostream>
#include <filesystem>

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

      for (int32_t index = 0; const SQLiteRow & row : SQLiteStatement{ connection, "Select * From Things" })
      {
        printf_s("[%2d]: %.2f\n", ++index, row.GetDouble());
      }

      std::cout << "\n\n";
    }

    auto connection = SQLiteConnection::Memory();

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
