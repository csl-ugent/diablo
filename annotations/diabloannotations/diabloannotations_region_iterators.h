/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef REGION_ITERATORS_H
#define REGION_ITERATORS_H

typedef bool (*RegionIteratorComparer)(AbstractAnnotationInfo *i);


template<typename IterT, typename TypeT>
class RegionIterator :
  public std::iterator<std::input_iterator_tag, TypeT>
{
protected:
  void first_match()
  {
    for (; m_current != m_end; m_current++)
      if (m_matcher(*m_current))
        break;
  }

  void next_match()
  {
    if (m_current == m_end) return;

    m_current++;

    for (; m_current != m_end; m_current++)
      if (m_matcher(*m_current))
        break;
  }

  IterT m_current;
  IterT m_end;

  RegionIteratorComparer m_matcher;

public:
  RegionIterator() : m_current(), m_end(), m_matcher() {}

  RegionIterator(const IterT& current, const IterT& end, RegionIteratorComparer matcher) : m_current(current), m_end(end), m_matcher(matcher) {
    first_match();
  }

  RegionIterator(const RegionIterator& r) : m_current(r.m_current), m_end(r.m_end), m_matcher(r.m_matcher) {DEBUG(("COPY CONSTRUCTOR"));}

  RegionIterator<IterT, TypeT>& operator=(const RegionIterator& r) {
    m_current = r.m_current;
    m_end = r.m_end;
    m_matcher = r.m_matcher;

    return *this;
  }

  RegionIterator<IterT, TypeT>& operator++() {
    next_match();

    return *this;
  }

  RegionIterator<IterT, TypeT>& operator++(int) {
    RegionIterator it = *this;
    next_match();

    return it;
  }

  const TypeT& operator*() {
    return *m_current;
  }

  TypeT* operator->() {
    return &(*m_current);
  }

  bool operator!=(const IterT& r) const {
    return (m_current != r);
  }

  bool operator==(const IterT& r) const {
    return !operator!=(r);
  }
};
typedef RegionIterator<AnnotationRequests::const_iterator, AbstractAnnotationInfo *> MyRegionIterator;

template<typename T>
static bool RegionIteratorMatch(AbstractAnnotationInfo *i)
{
  return !(dynamic_cast<T*>(i) == NULL);
}

template<typename T>
static MyRegionIterator CreateRegionIterator(AnnotationRequests& r)
{
  return MyRegionIterator(r.begin(), r.end(), RegionIteratorMatch<T>);
}

#endif
