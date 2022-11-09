#pragma once

#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT __noop
#endif

namespace ModernCppSQLite
{
  template <typename T>
  struct SQLiteHandleTraits
  {
    using Type = T;

    static Type Invalid() noexcept
    {
      return nullptr;
    }

    // static void Close(Type value) noexcept;
  };

  template <typename Traits>
  class SQLiteHandle
  {
  private:
    using Type = decltype(Traits::Invalid());

    void Close() noexcept
    {
      if (*this)
      {
        Traits::Close(m_Value);
      }
    }

  public:
    SQLiteHandle(SQLiteHandle const&) = delete;
    SQLiteHandle& operator=(SQLiteHandle const&) = delete;

    explicit SQLiteHandle(Type value = Traits::Invalid()) noexcept : m_Value(value)
    {
    }

    SQLiteHandle(SQLiteHandle&& other) noexcept : m_Value(other.Detach())
    {
    }

    SQLiteHandle& operator=(SQLiteHandle&& other) noexcept
    {
      if (this != &other)
      {
        Reset(other.Detach());
      }

      return *this;
    }

    ~SQLiteHandle() noexcept
    {
      Close();
    }

    explicit operator bool() const noexcept
    {
      return m_Value != Traits::Invalid();
    }

    Type Get() const noexcept
    {
      return m_Value;
    }

    Type* Set() noexcept
    {
      ASSERT(!*this);
      return &m_Value;
    }

    Type Detach() noexcept
    {
      Type value = m_Value;
      m_Value = Traits::Invalid();
      return value;
    }

    bool Reset(Type value = Traits::Invalid()) noexcept
    {
      if (m_Value != value)
      {
        Close();
        m_Value = value;
      }

      return static_cast<bool>(*this);
    }

    void Swap(SQLiteHandle<Traits>& other) noexcept
    {
      Type temp = m_Value;
      m_Value = other.m_Value;
      other.m_Value = temp;
    }

  private:
    Type m_Value;
  };

  template <typename Traits>
  void swap(SQLiteHandle<Traits>& left, SQLiteHandle<Traits>& right) noexcept
  {
    left.Swap(right);
  }

  template <typename Traits>
  bool operator==(SQLiteHandle<Traits> const& left, SQLiteHandle<Traits> const& right) noexcept
  {
    return left.Get() == right.Get();
  }

  template <typename Traits>
  bool operator!=(SQLiteHandle<Traits> const& left, SQLiteHandle<Traits> const& right) noexcept
  {
    return !(left == right);
  }
}
