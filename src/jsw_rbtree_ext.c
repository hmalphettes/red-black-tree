/*
  Red Black balanced tree library: more APIs.

    > Created (Hugues Malphettes): May, 2010
*/
#include "jsw_rbtree.h"


/**
  <summary>
  Search for the smallest node in a red black tree
  that value is equal or greater to the specified data
  <summary>
  <param name="tree">The tree to search</param>
  <param name="data">The data value to search for</param>
  <returns>
  A pointer to the data value stored in the tree
  </returns>
*/
void *jsw_rbfind_ceiling ( jsw_rbtree_t *tree, void *data )
{
  jsw_rbnode_t *it = tree->root;
  void *closest;

  while ( it != NULL ) {
    int cmp = tree->cmp ( it->data, data );

    if (cmp == 0)
      return it->data;
    else if (cmp > 0)
      closest = it->data;

    /*
      If the tree supports duplicates, they should be
      chained to the right subtree for this to work
    */
    it = it->link[cmp < 0];
  }

  return closest;
}

/**
  <summary>
  Search for the biggest node in a red black tree
  that value is equal or lesser to the specified data
  <summary>
  <param name="tree">The tree to search</param>
  <param name="data">The data value to search for</param>
  <returns>
  A pointer to the data value stored in the tree
  </returns>
*/
void *jsw_rbfind_floor ( jsw_rbtree_t *tree, void *data )
{
  jsw_rbnode_t *it = tree->root;
  void *closest;

  while ( it != NULL ) {
    int cmp = tree->cmp ( it->data, data );

    if (cmp == 0)
      return it->data;
    else if (cmp < 0)
      closest = it->data;

    /*
      If the tree supports duplicates, they should be
      chained to the right subtree for this to work
    */
    it = it->link[cmp < 0];
  }

  return closest;
}

/**
  <summary>
  Search for the biggest node in a red black tree
  that value is equal or lesser to the specified data
  <summary>
  <param name="tree">The tree to search</param>
  <param name="data">The data value to search for</param>
  <returns>
  A pointer to the data value stored in the tree
  </returns>
*/
void jsw_rbfind_closest ( const jsw_rbtree_t *tree, const void *data, void *data0, void *data1 )
{
  jsw_rbnode_t *it = tree->root;
  void *lesser = NULL, *bigger = NULL;
  int i = 0;
  while ( it != NULL ) {
    i++;
    int cmp = tree->cmp ( it->data, data );

    if (cmp == 0) {
      lesser = it->data;
      tree->closest(data, lesser, NULL, data0, data1);
      return;
    } else if (cmp < 0) {
      lesser = it->data;
    } else {
      bigger = it->data;
    }
    /*
      If the tree supports duplicates, they should be
      chained to the right subtree for this to work
    */
    it = it->link[cmp < 0];
  }
  tree->closest(data, lesser, bigger, data0, data1);
}


static void *peek ( jsw_rbtrav_t *trav, int dir )
{
  jsw_rbnode_t *next;
  if ( trav->it->link[dir] != NULL ) {
    /* Continue down this branch */
    next = trav->it->link[dir];

    while ( next->link[!dir] != NULL ) {
      next = next->link[!dir];
    }
  }
  else {
    /* Move to the next branch */
    jsw_rbnode_t *last;

    do {
      if ( trav->top == 0 ) {
        return NULL;
      }

      last = trav->it;
      next = trav->path[trav->top - 1];
    } while ( last == trav->it->link[dir] );
  }

  return next == NULL ? NULL : next->data;
}

/**
  <summary>
  Get to the next value in ascending order. Does not change the state of the traversal.
  <summary>
  <param name="trav">The initialized traversal object</param>
  <returns>A pointer to the next value in ascending order</returns>
 */
void *jsw_rbtpeeknext ( jsw_rbtrav_t *trav )
{
  return peek(trav, 1);
}

/**
  <summary>
  Get to the next value in descending order. Does not change the state of the traversal.
  <summary>
  <param name="trav">The initialized traversal object</param>
  <returns>A pointer to the next value in descending order</returns>
 */
void *jsw_rbtpeekprev ( jsw_rbtrav_t *trav )
{
  return peek(trav, 0);
}
