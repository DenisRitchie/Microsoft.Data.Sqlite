#include <iostream>
#include <SQLite.h>

using namespace std;
using namespace ModernCppSQLite;

int32_t main()
{
  try
  {
    SQLiteConnection connection = SQLiteConnection::WideMemory();

    Execute(connection, "Create Table Things (Content Text)");
    Execute(connection, "Insert Into Things Values (?)", "Joe");
    Execute(connection, "Insert Into Things Values (?)", u8"Joe UTF-8");
    Execute(connection, "Insert Into Things Values (?)", u"Joe UTF-16");
    Execute(connection, "Insert Into Things Values (?)", 123);
    Execute(connection, "Insert Into Things Values (?)", nullptr);
    Execute(connection, "Insert Into Things Values (?)", std::nullopt);
    Execute(connection, "Insert Into Things Values (?)", std::optional<std::wstring>(std::nullopt));

    for (SQLiteRow row : SQLiteStatement(connection, "Select Content As Value From Things"))
    {
      printf_s("%s (%s)\n", row.GetU8String(0), SQLiteTypeName(row.GetType()));
      printf_s("Column Database Name: %s\n", row.GetColumnDatabaseName().data());
      printf_s("Column Origin Name: %s\n", row.GetColumnOriginName().data());
      printf_s("Column Table Name: %s\n", row.GetColumnTableName().data());
      printf_s("Column Name: %s\n", row.GetColumnU8Name().data());
    }
  }
  catch (const SQLiteException& ex)
  {
    clog << "Error Code:    " << ex.ErrorCode << endl;
    clog << "Error Message: " << ex.ErrorMessage << endl;
  }
}
