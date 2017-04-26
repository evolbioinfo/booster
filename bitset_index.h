#ifndef _BITSET_INDEX_H_
#define _BITSET_INDEX_H_

#include "hashtables_bfields.h"

typedef struct bitset_keyvalue{
id_hash_table_t* key;
int value;
} bitset_keyvalue;

typedef struct bitset_bucket{
int size;
int capacity;
struct bitset_keyvalue **values;
} bitset_bucket;


typedef  struct bitset_hashmap{
struct bitset_bucket **map_array; 
int capacity;
float loadfactor;
int total;
} bitset_hashmap;


// Allocates a new bitset hasmap
bitset_hashmap* new_bitset_hashmap(int size, float loadfactor);
// Free the whole bitset hashmap
void free_bitset_hashmap(bitset_hashmap *hm);
// Free a map_array
void bitset_hash_map_free_map_array(bitset_bucket **map_array, int total);
// Free a set of bitset_keyvalue
void bitset_hash_map_free_buckets(bitset_keyvalue ** values, int total);
// returns the index in the hash map, given a hashcode
int bitset_hashmap_indexfor(int hashcode, int capacity);
// Returns the count for the given Edge
// If the edge is not present, returns -1
// If the edge is present, returns the value
int bitset_hashmap_value(bitset_hashmap *hm, id_hash_table_t *bitset, int nb_taxa);
// Inserts a value in the hashmap
void bitset_hashmap_putvalue(bitset_hashmap *hm, id_hash_table_t *bitset, int nb_taxa, int value);
// Computes a hash code for the bitset associated with an edge
int bitset_hashcode(id_hash_table_t *hashtable, int nb_taxa);
// HashCode for an edge bitset.
// Used for insertion in an EdgeMap
int bitset_hashEquals(id_hash_table_t *tbl1, id_hash_table_t *tbl2, int nb_taxa);
// Reconstructs the HashMap if the capacity is almost attained (loadfactor)
void bitset_hashmap_rehash(bitset_hashmap *hm, int nb_taxa);

#endif
