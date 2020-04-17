#ifndef STATE_HH
#define STATE_HH
#include "libswitch.h"
#include <click/bitvector.hh>
#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/ipflowid.hh>
#include <click/timer.hh>

template <class K, class V> class P4StateMap {

public:
  class P4StateEntry {
  public:
    typedef K key_type;
    typedef const K &key_const_reference;

    P4StateEntry() {}
    key_const_reference hashkey() const { return k; }

  private:
    K k;
    V *v;
    P4StateEntry *_hashnext;

    friend class HashContainer_adapter<P4StateEntry>;
  };

  void add(const K &k, const V *v);
  void destroy(const K &k);
  void lookup(const K &k, const V **v_p);

protected:
  HashContainer<P4StateEntry> _map;
};

#endif