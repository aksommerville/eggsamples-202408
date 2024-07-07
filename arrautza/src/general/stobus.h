/* stobus.h
 * Global store and bus.
 * We record global state as fields of a fixed size in bits, and notify subscribers on any change.
 */
 
#ifndef STOBUS_H
#define STOBUS_H

struct stobus; // Defined below.

/* To aid detection of leaks, we log a warning if any listeners remain registered at cleanup.
 */
void stobus_cleanup(struct stobus *stobus);

/* Define a field of the given size.
 * The order you define them matters.
 * Size must be in 0..31. Zero is valid, for stateless signals.
 */
int stobus_define(struct stobus *stobus,int id,int size_bits);

/* Register a callback for future changes to some field.
 * (listenerid) is always >0.
 * The callback does not fire immediately.
 * For stateless fields, you'll get exactly the (v) provided to stobus_set().
 * Fields with state, you'll see the final clamped value, and only when it actually changes.
 */
int stobus_listen(struct stobus *stobus,int id,void (*cb)(int id,int v,void *userdata),void *userdata);
void stobus_unlisten(struct stobus *stobus,int listenerid);

/* Setting a field clamps to zero and its maximum.
 * eg if the size is 8 bits and you set v=300, we write out v=255.
 * Setting a zero-size field to any value triggers an event.
 * For nonzero size, events are only triggered if the value changed.
 */
int stobus_get(struct stobus *stobus,int id);
void stobus_set(struct stobus *stobus,int id,int v);

/* Serial content is base64.
 * You must define all fields first, separately. That schema is not part of the state.
 * We have a (dirty) flag that gets set when any field changes. You should clear it after encoding and saving.
 * We clear (dirty) on decode.
 */
int stobus_encode(char *dst,int dsta,const struct stobus *stobus);
int stobus_decode(struct stobus *stobus,const char *src,int srcc);

/* Private-ish.
 **********************************************************************/

struct stobus {
  int dirty;
  struct stobus_field {
    int id;
    int c; // bits
    int v;
  } *fieldv;
  int fieldc,fielda;
  struct stobus_listener {
    int listenerid;
    int fldid;
    void (*cb)(int id,int v,void *userdata);
    void *userdata;
  } *listenerv;
  int listenerc,listenera;
};

int stobus_fieldv_search(const struct stobus *stobus,int id);
struct stobus_field *stobus_fieldv_insert(struct stobus *stobus,int p,int id);

int stobus_listenerv_search(const struct stobus *stobus,int id);

#endif
