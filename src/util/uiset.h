/* Sorted dynamic-array set of unsigned int and unsigned short.
 *
 * Replacement for std::set<unsigned int> / std::set<unsigned short> used
 * pervasively in SDCCtree_dec, SDCCnaddr, SDCClospre, and SDCCralloc.
 *
 * Operations preserve ascending order; binary search for membership/insert.
 */
#ifndef KCC_UISET_H
#define KCC_UISET_H

#include <stddef.h>

/* --- unsigned int set --- */

typedef struct {
    unsigned int *items;
    size_t n;
    size_t cap;
} uiset_t;

void uiset_init(uiset_t *s);
void uiset_free(uiset_t *s);
void uiset_clear(uiset_t *s);
int  uiset_contains(const uiset_t *s, unsigned int v);
int  uiset_insert(uiset_t *s, unsigned int v);        /* 1 if new, 0 if dup */
int  uiset_erase(uiset_t *s, unsigned int v);         /* 1 if removed */
void uiset_copy(uiset_t *dst, const uiset_t *src);
void uiset_move(uiset_t *dst, uiset_t *src);          /* swap-style, frees dst */
int  uiset_equal(const uiset_t *a, const uiset_t *b);
int  uiset_includes(const uiset_t *a, const uiset_t *b); /* a ⊇ b */
void uiset_intersection(const uiset_t *a, const uiset_t *b, uiset_t *out);
void uiset_difference(const uiset_t *a, const uiset_t *b, uiset_t *out);
void uiset_union_into(uiset_t *a, const uiset_t *b);

/* --- unsigned short set --- */

typedef struct {
    unsigned short *items;
    size_t n;
    size_t cap;
} usset_t;

void usset_init(usset_t *s);
void usset_free(usset_t *s);
void usset_clear(usset_t *s);
int  usset_contains(const usset_t *s, unsigned short v);
int  usset_insert(usset_t *s, unsigned short v);
int  usset_erase(usset_t *s, unsigned short v);
void usset_copy(usset_t *dst, const usset_t *src);
void usset_move(usset_t *dst, usset_t *src);
int  usset_equal(const usset_t *a, const usset_t *b);
int  usset_less(const usset_t *a, const usset_t *b);  /* lex compare, <0, 0, >0 */
void usset_union_into(usset_t *a, const usset_t *b);

/* --- signed short set (var_t from ralloc) --- */

typedef struct {
    short *items;
    size_t n;
    size_t cap;
} sssset_t;  /* name avoids clashing with anything */

void sss_init(sssset_t *s);
void sss_free(sssset_t *s);
void sss_clear(sssset_t *s);
int  sss_contains(const sssset_t *s, short v);
int  sss_insert(sssset_t *s, short v);
int  sss_erase(sssset_t *s, short v);
void sss_copy(sssset_t *dst, const sssset_t *src);
int  sss_equal(const sssset_t *a, const sssset_t *b);

/* --- int set --- */

typedef struct {
    int *items;
    size_t n;
    size_t cap;
} iset_t;

void iset_init(iset_t *s);
void iset_free(iset_t *s);
void iset_clear(iset_t *s);
int  iset_contains(const iset_t *s, int v);
int  iset_insert(iset_t *s, int v);
int  iset_erase(iset_t *s, int v);

#endif
