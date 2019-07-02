#ifndef DIABLOSUPPORT_BITSET_HPP
#define DIABLOSUPPORT_BITSET_HPP
#include "diablosupport.hpp"

class MyBitset {
public:
  MyBitset(size_t nrbits);
  ~MyBitset();

  void Set(size_t n);
  size_t Setcount();

  void Or(MyBitset& rhs);

  class Iterator {
  public:
    Iterator(MyBitset& bs, size_t n) : m_bs(bs), m_n(n) {
      initialize();
    }

    Iterator& operator++() {
      increment();
      return *this;
    }

    size_t operator*() {
      return m_n;
    }

    bool operator!=(const Iterator& rhs) {
      return m_n != rhs.m_n;
    }

  private:
    void initialize();
    void increment();

    MyBitset& m_bs;
    size_t m_n;
  };

  Iterator begin() { return Iterator(*this, 0); };
  Iterator end() { return Iterator(*this, m_nrbits); };

private:
  inline size_t W(size_t n) { return n / (sizeof(size_t) * 8); }
  inline size_t O(size_t n) { return n % (sizeof(size_t) * 8); }
  void expand(size_t nrwords);

  size_t *m_data;
  size_t m_nrbits;
  size_t m_nrwords;
  size_t m_nrset;
  size_t first_set, last_set;
};

#endif
