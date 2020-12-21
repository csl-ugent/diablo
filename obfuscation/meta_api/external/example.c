#define DATASTRUCTURE _Py_hashtable_t

// Predicate 0
// key 'K' exists in table
void ht_set_predicate_0_true(_Py_hashtable_t *ht$meta_instance)
{
  $meta_setter(P0=T && P1=?);

  // a value need not necessarily be associated with a key
  $meta_var(const void *, K, "string1");
  $meta_var(const void *, V, "string2");

  // 1. to insert an entry with a certain value
  _Py_hashtable_set(ht, ht->key_size, $K, ht->data_size, $V);
  // 2. to insert an entry without a value
  //_Py_hashtable_set(ht, ht->key_size, $K, 0, NULL);
}

void ht_set_predicate_0_false(_Py_hashtable_t *ht$meta_instance)
{
  $meta_setter(P0=F && P1=F);

  $meta_var(const void *, K, "string1");

  // remove the key from the hash table
  _Py_hashtable_pop_entry(ht, ht->key_size, $K, NULL, 0);
}

int ht_get_predicate_0(_Py_hashtable_t *ht$meta_instance)
{
  $meta_getter;

  $meta_var(const void *, K, "string1");

  // '_Py_hashtable_get_entry' returns NULL when no entry was found
  return _Py_hashtable_get_entry(ht, ht->key_size, $K) != NULL;
}

// PREDICATE 1
// value for 'K1' == value for 'K2'
void ht_set_predicate_1_true1(_Py_hashtable_t *ht$meta_instance)
{
  $meta_setter(P0=T && P1=T);

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");
  $meta_var(const void *, V, "string2");

  _Py_hashtable_set(ht, ht->key_size, $K1, ht->data_size, $V);
  _Py_hashtable_set(ht, ht->key_size, $K2, ht->data_size, $V);
}

void ht_set_predicate_1_true2(_Py_hashtable_t *ht$meta_instance, const void *V$meta_arg("anything"))
{
  $meta_setter(P0=T && P1=T);

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");

  _Py_hashtable_set(ht, ht->key_size, $K1, ht->data_size, $V);
  _Py_hashtable_set(ht, ht->key_size, $K2, ht->data_size, $V);
}

void ht_set_predicate_1_true3(_Py_hashtable_t *ht$meta_instance, const void *V$meta_arg("string1"))
{
  $meta_setter(P0=T && P1=T);

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");

  _Py_hashtable_set(ht, ht->key_size, $K1, ht->data_size, $V);
  _Py_hashtable_set(ht, ht->key_size, $K2, ht->data_size, $V);
}

void ht_set_predicate_1_false1(_Py_hashtable_t *ht$meta_instance)
{
  $meta_setter(P0=F && P1=F);

  $meta_var(const void *, K1, "string1");

  /* remove K1 from the hash table */
  _Py_hashtable_pop_entry(ht, ht->key_size, $K1, NULL, 0);
}

void ht_set_predicate_1_false2(_Py_hashtable_t *ht$meta_instance)
{
  $meta_setter(P0=U && P1=F);

  $meta_var(const void *, K2, "string3");

  /* remove K2 from the hash table */
  _Py_hashtable_pop_entry(ht, ht->key_size, $K2, NULL, 0);
}

void ht_set_predicate_1_false3(_Py_hashtable_t *ht$meta_instance, const void *V1$meta_arg("anything"), const void *V2$meta_arg("anything"))
//META_API_ARGUMENT_CONSTRAINT(V1 != V2)
{
  $meta_setter(P0=T && P1=F);

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");

  /* set K1 and K2 to different values */
  _Py_hashtable_set(ht, ht->key_size, $K1, ht->data_size, $V1);
  _Py_hashtable_set(ht, ht->key_size, $K2, ht->data_size, $V2);
}

void ht_set_predicate_1(_Py_hashtable_t *ht$meta_instance, const void *V1$meta_arg("anything"), const void *V2$meta_arg("anything"))
{
  $meta_setter(V1 != V2 => P0=T && P1=F);
  $meta_setter(V1 == V2 => P0=T && P1=T);

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");

  /* set K1 and K2 to different values */
  _Py_hashtable_set(ht, ht->key_size, $K1, ht->data_size, $V1);
  _Py_hashtable_set(ht, ht->key_size, $K2, ht->data_size, $V2);
}

int ht_get_predicate_1(_Py_hashtable_t *ht$meta_instance)
{
  $meta_getter;

  $meta_var(const void *, K1, "string1");
  $meta_var(const void *, K2, "string3");

  void *V1;
  int result1 = _Py_hashtable_get(ht, ht->key_size, $K1, ht->data_size, V1);

  void *V2;
  int result2 = _Py_hashtable_get(ht, ht->key_size, $K2, ht->data_size, V2);

  /* Both keys should exist in the hash table,
   * and both should have an identical value associated with them. */
  return (result1 == result2) && result1 && (V1 == V2);
}
