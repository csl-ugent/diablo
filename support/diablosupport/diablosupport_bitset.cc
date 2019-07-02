#include "diablosupport.hpp"

/*FUNCTION_DYNAMIC_MEMBER_GLOBAL_BODY(objectfiles, OBJECTFILES, ObjectFiles, MyBitset*, { *valp = NULL; }, { if (*valp) delete *valp; }, { *valp = NULL; });
FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(objectfiles);*/

#define BITS_PER_WORD (sizeof(size_t) * 8)

MyBitset::MyBitset(size_t nrbits) {
  m_nrbits = nrbits;
  m_nrset = 0;
  first_set = m_nrbits;
  last_set = m_nrbits;
  m_nrwords = 0;
  m_data = NULL;
}

MyBitset::~MyBitset() {
  if (m_data != NULL)
    free(m_data);
}

size_t MyBitset::Setcount() {
  return m_nrset;
}

void MyBitset::Set(size_t n) {
  ASSERT(n < m_nrbits, ("bit %d out of range (%d)", n, m_nrbits));
  size_t widx = W(n);
  if (widx >= m_nrwords)
    expand(widx+1);
  //cout << "setting " << n << " (" << W(n) << ":" << O(n) << ")" << endl;

  if ((m_data[W(n)] & 1ULL<<O(n)) == 0)
    m_nrset++;
  m_data[W(n)] |= 1ULL<<O(n);

  if (n < first_set || first_set == m_nrbits) first_set = n;
  if (n > last_set || last_set == m_nrbits) last_set = n;
}

void MyBitset::Or(MyBitset& rhs) {
  if (rhs.m_nrset == 0)
    return;

  bool old_had_bits = (m_nrset != 0);
  if (rhs.m_nrwords > m_nrwords) {
    expand(rhs.m_nrwords);
    m_nrbits = rhs.m_nrbits;
  }

  for (size_t i = rhs.W(rhs.first_set); i <= rhs.W(rhs.last_set); i++) {
    // count number of added bits
    size_t v = (m_data[i] ^ rhs.m_data[i]) & ~(m_data[i]);
    // ... using 'Brian Kernighan' algorithm
    while (v > 0) {
      m_nrset++;
      v &= v-1;
    }

    m_data[i] |= rhs.m_data[i];
  }

  /* adapt first and last sets */
  if (!old_had_bits || rhs.first_set < first_set) first_set = rhs.first_set;
  if (!old_had_bits || rhs.last_set > last_set) last_set = rhs.last_set;
}

void MyBitset::Iterator::initialize() {
  /* no need to do anything if this is the end iterator */
  if (m_n == m_bs.m_nrbits)
    return;

  m_n = m_bs.first_set;
}

void MyBitset::Iterator::increment() {
  /* early exit if this is the last set bit */
  if (m_n == m_bs.last_set) {
    m_n = m_bs.m_nrbits;
    return;
  }

  size_t offset = m_n % BITS_PER_WORD;
  size_t widx = m_n / BITS_PER_WORD;
  //cout << "current " << m_n << " (" << widx << ":" << offset << ")" << endl;

  size_t mask = (1ULL<<offset) | ((1ULL<<offset)-1);
  bool new_word = false;

  if ((m_bs.m_data[widx] & ~mask) == 0) {
    /* look for the next word */
    widx++;
    m_n = widx * BITS_PER_WORD;
    new_word = true;

    for (; widx < m_bs.m_nrwords; widx++) {
      if (m_bs.m_data[widx] != 0ULL)
        break;
      else
        m_n += BITS_PER_WORD;
    }
  }
  //cout << " next found in word " << widx << "(" << m_n << ")" << endl;

  if (m_n >= m_bs.m_nrbits || widx == m_bs.m_nrwords
      || m_n > m_bs.last_set) {
    m_n = m_bs.m_nrbits;
  }
  else {
    /* if we get here, some bit is set in word 'widx' */
    if (!new_word) {
      //cout << "not a new word!" << endl;
      m_n++;
      offset++;
    }
    else {
      offset = 0;
    }
    //cout << "start looking at offset " << offset << endl;

    for (size_t i = offset; i < BITS_PER_WORD; i++, m_n++)
      if (m_bs.m_data[widx] & (1ULL<<i))
        break;

    if (m_n > m_bs.m_nrbits
        || m_n > m_bs.last_set)
      m_n = m_bs.m_nrbits;
  }

  //cout << "next: " << m_n << endl;
}

void MyBitset::expand(size_t nwords) {
  ASSERT(nwords < 10000, ("can't expand to %lld (%llx) words!", nwords, nwords));
  size_t *newdata = static_cast<size_t *>(realloc(m_data, sizeof(size_t) * nwords));
  ASSERT(newdata != NULL, ("could not realloc!"));
  m_data = newdata;

  /* clear allocated (extra) memory */
  for (size_t i = m_nrwords; i < nwords; i++)
    m_data[i] = 0ULL;

  m_nrwords = nwords;
}
