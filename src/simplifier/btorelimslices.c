/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2014 Armin Biere.
 *  Copyright (C) 2012-2016 Mathias Preiner.
 *  Copyright (C) 2012-2016 Aina Niemetz.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "simplifier/btorelimslices.h"
#include "btorcore.h"
#include "utils/btornodeiter.h"
#include "utils/btorutil.h"

struct BtorSlice
{
  int upper;
  int lower;
};

typedef struct BtorSlice BtorSlice;

static BtorSlice *
new_slice (Btor *btor, int upper, int lower)
{
  BtorSlice *result;

  assert (btor != NULL);
  assert (upper >= lower);
  assert (lower >= 0);

  BTOR_NEW (btor->mm, result);
  result->upper = upper;
  result->lower = lower;
  return result;
}

static void
delete_slice (Btor *btor, BtorSlice *slice)
{
  assert (btor != NULL);
  assert (slice != NULL);
  BTOR_DELETE (btor->mm, slice);
}

static unsigned int
hash_slice (BtorSlice *slice)
{
  unsigned int result;

  assert (slice != NULL);
  assert (slice->upper >= slice->lower);
  assert (slice->lower >= 0);

  result = (unsigned int) slice->upper;
  result += (unsigned int) slice->lower;
  result *= 7334147u;
  return result;
}

static int
compare_slices (BtorSlice *s1, BtorSlice *s2)
{
  assert (s1 != NULL);
  assert (s2 != NULL);
  assert (s1->upper >= s1->lower);
  assert (s1->lower >= 0);
  assert (s2->upper >= s2->lower);
  assert (s2->lower >= 0);

  if (s1->upper < s2->upper) return -1;

  if (s1->upper > s2->upper) return 1;

  assert (s1->upper == s1->upper);
  if (s1->lower < s2->lower) return -1;

  if (s1->lower > s2->lower) return 1;

  assert (s1->upper == s2->upper && s1->lower == s2->lower);
  return 0;
}

static int
compare_slices_qsort (const void *p1, const void *p2)
{
  return compare_slices (*((BtorSlice **) p1), *((BtorSlice **) p2));
}

static int
compare_int_ptr (const void *p1, const void *p2)
{
  int v1 = *((int *) p1);
  int v2 = *((int *) p2);
  if (v1 < v2) return -1;

  if (v1 > v2) return 1;

  return 0;
}

void
btor_eliminate_slices_on_bv_vars (Btor *btor)
{
  BtorNode *var, *cur, *result, *lambda_var, *temp;
  BtorSortId sort;
  BtorSlice *s1, *s2, *new_s1, *new_s2, *new_s3, **sorted_slices;
  BtorPtrHashBucket *b_var, *b1, *b2;
  BtorNodeIterator it;
  BtorPtrHashTable *slices;
  int i, min, max, count;
  BtorNodePtrStack vars;
  double start, delta;
  BtorMemMgr *mm;
  int vals[4];

  assert (btor != NULL);

  start = btor_time_stamp ();
  count = 0;

  mm = btor->mm;
  BTOR_INIT_STACK (mm, vars);
  for (b_var = btor->bv_vars->first; b_var != NULL; b_var = b_var->next)
  {
    var = (BtorNode *) b_var->key;
    BTOR_PUSH_STACK (vars, var);
  }

  while (!BTOR_EMPTY_STACK (vars))
  {
    slices = btor_hashptr_table_new (
        mm, (BtorHashPtr) hash_slice, (BtorCmpPtr) compare_slices);
    var = BTOR_POP_STACK (vars);
    assert (BTOR_IS_REGULAR_NODE (var));
    assert (btor_is_bv_var_node (var));
    btor_init_parent_iterator (&it, var);
    /* find all slices on variable */
    while (btor_has_next_parent_iterator (&it))
    {
      cur = btor_next_parent_iterator (&it);
      assert (BTOR_IS_REGULAR_NODE (cur));
      if (cur->kind == BTOR_SLICE_NODE)
      {
        s1 = new_slice (
            btor, btor_slice_get_upper (cur), btor_slice_get_lower (cur));
        assert (!btor_hashptr_table_get (slices, s1));
        btor_hashptr_table_add (slices, s1);
      }
    }

    /* no splitting necessary? */
    if (slices->count == 0u)
    {
      btor_hashptr_table_delete (slices);
      continue;
    }

    /* add full slice */
    s1 = new_slice (btor, btor_get_exp_width (btor, var) - 1, 0);
    assert (!btor_hashptr_table_get (slices, s1));
    btor_hashptr_table_add (slices, s1);

  BTOR_SPLIT_SLICES_RESTART:
    for (b1 = slices->last; b1 != NULL; b1 = b1->prev)
    {
      s1 = (BtorSlice *) b1->key;
      for (b2 = b1->prev; b2 != NULL; b2 = b2->prev)
      {
        s2 = (BtorSlice *) b2->key;

        assert (compare_slices (s1, s2));

        /* not overlapping? */
        if ((s1->lower > s2->upper) || (s1->upper < s2->lower)
            || (s2->lower > s1->upper) || (s2->upper < s1->lower))
          continue;

        if (s1->upper == s2->upper)
        {
          assert (s1->lower != s2->lower);
          max    = BTOR_MAX_UTIL (s1->lower, s2->lower);
          min    = BTOR_MIN_UTIL (s1->lower, s2->lower);
          new_s1 = new_slice (btor, max - 1, min);
          if (!btor_hashptr_table_get (slices, new_s1))
            btor_hashptr_table_add (slices, new_s1);
          else
            delete_slice (btor, new_s1);

          if (min == s1->lower)
          {
            btor_hashptr_table_remove (slices, s1, 0, 0);
            delete_slice (btor, s1);
          }
          else
          {
            btor_hashptr_table_remove (slices, s2, 0, 0);
            delete_slice (btor, s2);
          }
          goto BTOR_SPLIT_SLICES_RESTART;
        }

        if (s1->lower == s2->lower)
        {
          assert (s1->upper != s2->upper);
          max    = BTOR_MAX_UTIL (s1->upper, s2->upper);
          min    = BTOR_MIN_UTIL (s1->upper, s2->upper);
          new_s1 = new_slice (btor, max, min + 1);
          if (!btor_hashptr_table_get (slices, new_s1))
            btor_hashptr_table_add (slices, new_s1);
          else
            delete_slice (btor, new_s1);
          if (max == s1->upper)
          {
            btor_hashptr_table_remove (slices, s1, 0, 0);
            delete_slice (btor, s1);
          }
          else
          {
            btor_hashptr_table_remove (slices, s2, NULL, NULL);
            delete_slice (btor, s2);
          }
          goto BTOR_SPLIT_SLICES_RESTART;
        }

        /* regular overlapping case (overlapping at both ends) */
        vals[0] = s1->upper;
        vals[1] = s1->lower;
        vals[2] = s2->upper;
        vals[3] = s2->lower;
        qsort (vals, 4, sizeof (int), compare_int_ptr);
        new_s1 = new_slice (btor, vals[3], vals[2] + 1);
        new_s2 = new_slice (btor, vals[2], vals[1]);
        new_s3 = new_slice (btor, vals[1] - 1, vals[0]);
        btor_hashptr_table_remove (slices, s1, 0, 0);
        btor_hashptr_table_remove (slices, s2, NULL, NULL);
        delete_slice (btor, s1);
        delete_slice (btor, s2);
        if (!btor_hashptr_table_get (slices, new_s1))
          btor_hashptr_table_add (slices, new_s1);
        else
          delete_slice (btor, new_s1);
        if (!btor_hashptr_table_get (slices, new_s2))
          btor_hashptr_table_add (slices, new_s2);
        else
          delete_slice (btor, new_s2);
        if (!btor_hashptr_table_get (slices, new_s3))
          btor_hashptr_table_add (slices, new_s3);
        else
          delete_slice (btor, new_s3);
        goto BTOR_SPLIT_SLICES_RESTART;
      }
    }

    /* copy slices to sort them */
    assert (slices->count > 1u);
    BTOR_NEWN (mm, sorted_slices, slices->count);
    i = 0;
    for (b1 = slices->first; b1 != NULL; b1 = b1->next)
    {
      s1                 = (BtorSlice *) b1->key;
      sorted_slices[i++] = s1;
    }
    qsort (sorted_slices,
           slices->count,
           sizeof (BtorSlice *),
           compare_slices_qsort);

    s1     = sorted_slices[(int) slices->count - 1];
    sort   = btor_sort_bitvec (btor, s1->upper - s1->lower + 1);
    result = btor_var_exp (btor, sort, 0);
    btor_sort_release (btor, sort);
    delete_slice (btor, s1);
    for (i = (int) slices->count - 2; i >= 0; i--)
    {
      s1         = sorted_slices[i];
      sort       = btor_sort_bitvec (btor, s1->upper - s1->lower + 1);
      lambda_var = btor_var_exp (btor, sort, 0);
      btor_sort_release (btor, sort);
      temp = btor_concat_exp (btor, result, lambda_var);
      btor_release_exp (btor, result);
      result = temp;
      btor_release_exp (btor, lambda_var);
      delete_slice (btor, s1);
    }
    BTOR_DELETEN (mm, sorted_slices, slices->count);
    btor_hashptr_table_delete (slices);

    count++;
    btor->stats.eliminated_slices++;
    temp = btor_eq_exp (btor, var, result);
    btor_assert_exp (btor, temp);
    btor_release_exp (btor, temp);
    btor_release_exp (btor, result);
  }

  BTOR_RELEASE_STACK (vars);

  delta = btor_time_stamp () - start;
  btor->time.slicing += delta;
  BTOR_MSG (btor->msg, 1, "sliced %d variables in %1.f seconds", count, delta);
}
