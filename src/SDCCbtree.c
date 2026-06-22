/* Philipp Klaus Krause, philipp@informatik.uni-frankfurt.de, pkk@spth.de, 2011
 *
 * (c) 2011 Goethe-Universitaet Frankfurt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Pure C port of the former SDCCbtree.cc.  The original used boost's
 * adjacency_list, but the graph is always a rooted in-tree (each node
 * except the root has exactly one parent set via btree_add_child), so
 * we represent it as a flat array of nodes.  Node 0 is the root, and
 * children are always appended after their parent -- matching boost's
 * vecS vertex numbering -- so the "walk the larger id upward" trick
 * used by btree_lowest_common_ancestor still works.
 */

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "SDCCbtree.h"

/* A node of the block tree. */
typedef struct btree_node {
    int parent;           /* index of parent node; -1 for root */
    struct symbol **syms; /* dynamic array of symbol pointers (the "set") */
    int nsyms;
    int syms_cap;
    int cssize;           /* accumulated subtree size (was btree[v].second) */
    int block;            /* original block id (bmaprev[v]) */
} btree_node_t;

static btree_node_t *nodes = NULL;
static int nnodes = 0;
static int nodes_cap = 0;

/* bmap: block id -> node index. Dynamic array indexed by block id; -1 means
 * the block is not present.  Block ids are small non-negative shorts. */
static int *bmap = NULL;
static int bmap_cap = 0;

static int
btree_new_node (int parent, int block)
{
    int idx;
    if (nnodes == nodes_cap)
        {
            int newcap = nodes_cap ? nodes_cap * 2 : 16;
            btree_node_t *n = (btree_node_t *) realloc (nodes, (size_t) newcap * sizeof (btree_node_t));
            wassert (n);
            nodes = n;
            nodes_cap = newcap;
        }
    idx = nnodes++;
    nodes[idx].parent = parent;
    nodes[idx].syms = NULL;
    nodes[idx].nsyms = 0;
    nodes[idx].syms_cap = 0;
    nodes[idx].cssize = 0;
    nodes[idx].block = block;
    return idx;
}

static void
bmap_ensure (int block)
{
    if (block < bmap_cap)
        return;
    {
        int newcap = bmap_cap ? bmap_cap : 16;
        int i;
        int *nb;
        while (newcap <= block)
            newcap *= 2;
        nb = (int *) realloc (bmap, (size_t) newcap * sizeof (int));
        wassert (nb);
        for (i = bmap_cap; i < newcap; i++)
            nb[i] = -1;
        bmap = nb;
        bmap_cap = newcap;
    }
}

static int
bmap_get (int block)
{
    if (block < 0 || block >= bmap_cap)
        return -1;
    return bmap[block];
}

static void
btree_clear_subtree (int v)
{
    int i;
    nodes[v].nsyms = 0;
    /* Walk children: any node whose parent is v. */
    for (i = v + 1; i < nnodes; i++)
        if (nodes[i].parent == v)
            btree_clear_subtree (i);
}

void
btree_clear (void)
{
    if (nnodes == 0)
        return;
    btree_clear_subtree (0);
}

void
btree_add_child (short parent, short child)
{
    int pidx, cidx;

    if (nnodes == 0)
        {
            (void) btree_new_node (-1, 0);
            bmap_ensure (0);
            bmap[0] = 0;
        }

    wassert (parent != child);
    pidx = bmap_get (parent);
    wassert (pidx != -1);

    cidx = btree_new_node (pidx, child);
    bmap_ensure (child);
    bmap[child] = cidx;

    wassert (pidx != cidx);
}

static int
btree_lowest_common_ancestor_impl (int a, int b)
{
    if (a == b)
        return a;
    else if (a > b)
        a = nodes[a].parent;
    else
        b = nodes[b].parent;
    return btree_lowest_common_ancestor_impl (a, b);
}

short
btree_lowest_common_ancestor (short a, short b)
{
    int ai = bmap_get (a);
    int bi = bmap_get (b);
    int anc;
    wassert (ai != -1 && bi != -1);
    anc = btree_lowest_common_ancestor_impl (ai, bi);
    return (short) nodes[anc].block;
}

void
btree_add_symbol (struct symbol *s)
{
    int block;
    int v;
    int i;
    btree_node_t *n;

    wassert (s);
    /* This is essentially a workaround.  TODO: Ensure that the parameter
     * block is placed correctly in the btree instead! */
    block = s->_isparm ? 0 : s->block;

    v = bmap_get (block);
    wassert (v != -1);
    wassert (v < nnodes);
    n = &nodes[v];

    /* Set semantics: skip duplicates. */
    for (i = 0; i < n->nsyms; i++)
        if (n->syms[i] == s)
            return;

    if (n->nsyms == n->syms_cap)
        {
            int newcap = n->syms_cap ? n->syms_cap * 2 : 4;
            struct symbol **ns = (struct symbol **) realloc (n->syms, (size_t) newcap * sizeof (struct symbol *));
            wassert (ns);
            n->syms = ns;
            n->syms_cap = newcap;
        }
    n->syms[n->nsyms++] = s;
}

static void
btree_alloc_subtree (int v, int sPtr, int cssize, int *ssize)
{
    int i;
    btree_node_t *n;

    wassert (v < nnodes);
    n = &nodes[v];

    for (i = 0; i < n->nsyms; i++)
        {
            struct symbol *const sym = n->syms[i];
            const int size = getSize (sym->type);

            if (port->stack.direction > 0)
                {
                    SPEC_STAK (sym->etype) = sym->stack = (sPtr + 1);
                    sPtr += size;
                }
            else
                {
                    sPtr -= size;
                    SPEC_STAK (sym->etype) = sym->stack = sPtr;
                }

            cssize += size;
        }
    nodes[v].cssize = cssize;
    if (cssize > *ssize)
        *ssize = cssize;

    /* Recurse into children. */
    for (i = v + 1; i < nnodes; i++)
        if (nodes[i].parent == v)
            btree_alloc_subtree (i, sPtr, cssize, ssize);
}

void
btree_alloc (void)
{
    int ssize = 0;

    if (nnodes == 0)
        return;

    btree_alloc_subtree (0, 0, 0, &ssize);

    if (currFunc)
        {
            currFunc->stack += ssize;
            SPEC_STAK (currFunc->etype) += ssize;
        }
}
