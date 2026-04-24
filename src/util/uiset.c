#include "uiset.h"

#include <stdlib.h>
#include <string.h>

/* The body is repeated for each element type via a macro. Binary search +
 * insertion preserving ascending order. */

#define DEFINE_SET_OPS(PFX, STRUCT, T)                                        \
    void PFX##_init(STRUCT *s) {                                              \
        s->items = NULL; s->n = 0; s->cap = 0;                                \
    }                                                                         \
    void PFX##_free(STRUCT *s) {                                              \
        free(s->items); s->items = NULL; s->n = 0; s->cap = 0;                \
    }                                                                         \
    void PFX##_clear(STRUCT *s) { s->n = 0; }                                 \
    static size_t PFX##_lb(const STRUCT *s, T v) {                            \
        size_t lo = 0, hi = s->n;                                             \
        while (lo < hi) {                                                     \
            size_t mid = lo + ((hi - lo) >> 1);                               \
            if (s->items[mid] < v) lo = mid + 1; else hi = mid;               \
        }                                                                     \
        return lo;                                                            \
    }                                                                         \
    int PFX##_contains(const STRUCT *s, T v) {                                \
        size_t i = PFX##_lb(s, v);                                            \
        return i < s->n && s->items[i] == v;                                  \
    }                                                                         \
    static void PFX##_reserve(STRUCT *s, size_t need) {                       \
        if (need <= s->cap) return;                                           \
        size_t nc = s->cap ? s->cap * 2 : 4;                                  \
        while (nc < need) nc *= 2;                                            \
        s->items = (T *)realloc(s->items, nc * sizeof(T));                    \
        s->cap = nc;                                                          \
    }                                                                         \
    int PFX##_insert(STRUCT *s, T v) {                                        \
        size_t i = PFX##_lb(s, v);                                            \
        if (i < s->n && s->items[i] == v) return 0;                           \
        PFX##_reserve(s, s->n + 1);                                           \
        memmove(&s->items[i + 1], &s->items[i], (s->n - i) * sizeof(T));      \
        s->items[i] = v; s->n++; return 1;                                    \
    }                                                                         \
    int PFX##_erase(STRUCT *s, T v) {                                         \
        size_t i = PFX##_lb(s, v);                                            \
        if (i >= s->n || s->items[i] != v) return 0;                          \
        memmove(&s->items[i], &s->items[i + 1], (s->n - i - 1) * sizeof(T));  \
        s->n--; return 1;                                                     \
    }

DEFINE_SET_OPS(uiset, uiset_t,   unsigned int)
DEFINE_SET_OPS(usset, usset_t,   unsigned short)
DEFINE_SET_OPS(sss,   sssset_t,  short)
DEFINE_SET_OPS(iset,  iset_t,    int)

/* ---- extras for uiset ---- */

void uiset_copy(uiset_t *dst, const uiset_t *src) {
    if (dst == src) return;
    if (dst->cap < src->n) {
        free(dst->items);
        dst->cap = src->n ? src->n : 4;
        dst->items = (unsigned int *)malloc(dst->cap * sizeof(unsigned int));
    }
    memcpy(dst->items, src->items, src->n * sizeof(unsigned int));
    dst->n = src->n;
}

void uiset_move(uiset_t *dst, uiset_t *src) {
    if (dst == src) return;
    free(dst->items);
    *dst = *src;
    src->items = NULL; src->n = 0; src->cap = 0;
}

int uiset_equal(const uiset_t *a, const uiset_t *b) {
    if (a->n != b->n) return 0;
    return memcmp(a->items, b->items, a->n * sizeof(unsigned int)) == 0;
}

int uiset_includes(const uiset_t *a, const uiset_t *b) {
    size_t i = 0, j = 0;
    while (j < b->n) {
        while (i < a->n && a->items[i] < b->items[j]) i++;
        if (i == a->n || a->items[i] != b->items[j]) return 0;
        i++; j++;
    }
    return 1;
}

void uiset_intersection(const uiset_t *a, const uiset_t *b, uiset_t *out) {
    uiset_clear(out);
    size_t i = 0, j = 0;
    while (i < a->n && j < b->n) {
        if (a->items[i] < b->items[j]) i++;
        else if (a->items[i] > b->items[j]) j++;
        else { uiset_insert(out, a->items[i]); i++; j++; }
    }
}

void uiset_difference(const uiset_t *a, const uiset_t *b, uiset_t *out) {
    uiset_clear(out);
    size_t i = 0, j = 0;
    while (i < a->n) {
        if (j >= b->n || a->items[i] < b->items[j]) {
            uiset_insert(out, a->items[i]); i++;
        } else if (a->items[i] > b->items[j]) {
            j++;
        } else { i++; j++; }
    }
}

void uiset_union_into(uiset_t *a, const uiset_t *b) {
    for (size_t i = 0; i < b->n; i++) uiset_insert(a, b->items[i]);
}

/* ---- extras for usset ---- */

void usset_copy(usset_t *dst, const usset_t *src) {
    if (dst == src) return;
    if (dst->cap < src->n) {
        free(dst->items);
        dst->cap = src->n ? src->n : 4;
        dst->items = (unsigned short *)malloc(dst->cap * sizeof(unsigned short));
    }
    memcpy(dst->items, src->items, src->n * sizeof(unsigned short));
    dst->n = src->n;
}

void usset_move(usset_t *dst, usset_t *src) {
    if (dst == src) return;
    free(dst->items);
    *dst = *src;
    src->items = NULL; src->n = 0; src->cap = 0;
}

int usset_equal(const usset_t *a, const usset_t *b) {
    if (a->n != b->n) return 0;
    return memcmp(a->items, b->items, a->n * sizeof(unsigned short)) == 0;
}

int usset_less(const usset_t *a, const usset_t *b) {
    size_t n = a->n < b->n ? a->n : b->n;
    for (size_t i = 0; i < n; i++) {
        if (a->items[i] < b->items[i]) return -1;
        if (a->items[i] > b->items[i]) return 1;
    }
    if (a->n < b->n) return -1;
    if (a->n > b->n) return 1;
    return 0;
}

void usset_union_into(usset_t *a, const usset_t *b) {
    for (size_t i = 0; i < b->n; i++) usset_insert(a, b->items[i]);
}

/* ---- extras for sssset ---- */

void sss_copy(sssset_t *dst, const sssset_t *src) {
    if (dst == src) return;
    if (dst->cap < src->n) {
        free(dst->items);
        dst->cap = src->n ? src->n : 4;
        dst->items = (short *)malloc(dst->cap * sizeof(short));
    }
    memcpy(dst->items, src->items, src->n * sizeof(short));
    dst->n = src->n;
}

int sss_equal(const sssset_t *a, const sssset_t *b) {
    if (a->n != b->n) return 0;
    return memcmp(a->items, b->items, a->n * sizeof(short)) == 0;
}
