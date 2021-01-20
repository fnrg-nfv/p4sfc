#ifndef IPS_HH
#define IPS_HH
#include <click/batchelement.hh>
CLICK_DECLS

#define TRIENODESIZE 256
struct TrieNode
{
  bool flag;
  TrieNode *failure;
  TrieNode *child[TRIENODESIZE];
  TrieNode()
  {
    flag = false;
    for (int i = 0; i < TRIENODESIZE; ++i)
      child[i] = NULL;
  }
};

class Trie
{
private:
  TrieNode *root = new TrieNode();

public:
  void insert(unsigned char *pattern, int len)
  {
    TrieNode *cur = root;
    for (int i = 0; i < len; ++i)
    {
      uint8_t cid = pattern[i];
      if (cur->child[cid] == NULL)
        cur->child[cid] = new TrieNode();
      cur = cur->child[cid];
    }
    cur->flag = true;
  }
  bool search(const unsigned char *big_str, int len)
  {
    TrieNode *cur = root;
    for (int i = 0; i < len; ++i)
    {
      int cid = big_str[i];
      if (cur->flag)
        return true;
      if (cur->child[cid] == NULL)
        return false;
      cur = cur->child[cid];
    }
    if (cur->flag)
      return true;
    return false;
  }
  bool iterSearch(const unsigned char *big_str, int len)
  {
    for (int i = 0; i < len; ++i)
    {
      if (search(big_str + i, len - i))
        return true;
    }
    return false;
  }
};

class SampleIPS : public BatchElement
{
public:
  SampleIPS();
  ~SampleIPS();

  const char *class_name() const { return "SampleIPS"; }
  const char *port_count() const { return "1/2"; }
  const char *processing() const { return PUSH; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  void push(int, Packet *);
  void push_batch(int, PacketBatch *);

  struct IPSPattern
  {
    uint32_t len;
    unsigned char *data;
  };
  bool pattern_match(IPSPattern &pt, Packet *p);
  // bool pattern_match(String &pt, Packet *p);

protected:
  int process(Packet *p);
  void print_patterns(void);
  IPSPattern parse_pattern(String &);
  Trie pattern_trie;

  bool _debug;
  uint32_t _depth;
  Vector<IPSPattern> patterns;
};

CLICK_ENDDECLS
#endif
