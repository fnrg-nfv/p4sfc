#include <bitset>
#include <click/config.h>
#include <iostream>

#include "p4sfcencap.hh"

CLICK_DECLS

int P4SFCEncap::configure(Vector<String> &conf, ErrorHandler *errh)
{
  in_batch_mode = BATCH_MODE_YES;
}

inline bool P4SFCEncap::smaction(int input, Packet *p)
{
  p4sfc_header_t *sfch;
  if (input == 0)
  {
    int result = pull_p4sfc_header(p, sfch);
    if (result == pull_fail)
      return false;
    _sfch_queue.push(sfch);
    return true;
  }
  else if (input == 1)
  {
    // assert(!_sfch_queue.empty());
    if (_sfch_queue.empty())
      return false;

    sfch = _sfch_queue.front();
    _sfch_queue.pop();
    p = push_p4sfc_header(p, sfch);
    free(sfch);
    return true;
  }
  return false;
}

void P4SFCEncap::push(int input, Packet *p)
{
  if (smaction(input, p))
    output(0).push(p);
  else
    p->kill();
}

void P4SFCEncap::push_batch(int input, PacketBatch *batch)
{
  Packet *head = batch;
  Packet *current = head;
  Packet *last = NULL;
  while (current != NULL)
  {
    if (smaction(input, current))
    {
      last = current;
    }
    else
    {
      if (current == head)
        head = current->next();
      else
        last->set_next(current->next());
      current->kill();
    }
    current = current->next();
  }

  if (likely(head == batch))
    output(input).push_batch(batch);
  else
    output(input).push_batch(static_cast<PacketBatch *>(head));
}

inline int P4SFCEncap::pull_p4sfc_header(Packet *p, p4sfc_header_t *&sfch)
{
  const unsigned char *data = p->data();
  uint16_t len = ntohs(*(uint16_t *)(data + 2));

  if (len == 0 || len > 100)
  {
    click_chatter("drop(sfc len: %d)\n", len);
    return pull_fail;
  }

  size_t size = sizeof(p4sfc_header_t) + len * sizeof(nf_header_t);

  sfch = (p4sfc_header_t *)malloc(size);
  sfch->id = ntohs(*(uint16_t *)data);
  sfch->len = len;
  data += sizeof(p4sfc_header_t);

  for (int i = 0; i < len; i++, data += sizeof(nf_header_t))
  {
    sfch->nfs[i].data = ntohs(*(const uint16_t *)data);
  }

  p->pull(size);
  return pull_success;
}

inline Packet *P4SFCEncap::push_p4sfc_header(Packet *p, p4sfc_header_t *sfch)
{
  size_t push_size = sizeof(p4sfc_header_t) + (sfch->len - 1) * sizeof(nf_header_t);
  WritablePacket *wp = p->push(push_size);
  const unsigned char *data = wp->data();

  *(uint16_t *)data = htons(sfch->id);
  data += 2;

  *(uint16_t *)data = htons(sfch->len - 1);
  data += 2;

  for (int i = 1; i < sfch->len; i++, data += sizeof(nf_header_t))
  {
    *(uint16_t *)data = htons(sfch->nfs[i].data);
  }

  return wp;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCEncap)
ELEMENT_MT_SAFE(P4SFCEncap)
