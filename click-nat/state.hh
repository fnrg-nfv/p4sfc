#ifndef STATE_HH
#define STATE_HH
#include <click/element.hh>
#include <click/ipflowid.hh>
#include <httplib.h>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>

using namespace std;

template <class K, class V> class P4StateMap {
public:
  enum { found, not_found };
  inline void add(const K &k, const V v) {
    _umap[k] = v; // TODO: add P4 part
  }
  inline void destroy(const K &k) {
    _umap.erase(k); // TODO: add P4 part
  }
  inline int find(const K &k, V *v_p) {
    auto search = _umap.find(k);
    if (search != _umap.end()) {
      *v_p = search->second;
      return found;
    }
    return not_found;
  }

  virtual char *table_name();
  virtual char *match_field(const K &k);
  virtual char *action_name();
  virtual char *action_params(const V &v);
  virtual void apply(WritablePacket *p, const V &v);

protected:
  unordered_map<K, V> _umap;
};

class P4BasicEntry {
public:
  P4BasicEntry() {}
  virtual char *table_name() const;
  virtual char *match_field() const;
  virtual char *action_name() const;
  virtual char *action_params() const;
  virtual void apply(WritablePacket *p);

  inline void p4add() {
    cout << table_name() << "\n"
         << match_field() << "\n"
         << action_name() << "\n"
         << action_params() << "\n";
  }
  inline void p4remove() {
    cout << table_name() << "\n"
         << match_field() << "\n"
         << action_name() << "\n"
         << action_params() << "\n";
  }
};

#endif