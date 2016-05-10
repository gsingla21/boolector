/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2013 Mathias Preiner.
 *  Copyright (C) 2015-2016 Aina Niemetz.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "testbitvec.h"
#include "btorbitvec.h"
#include "btorcore.h"
#include "btoropt.h"
#include "testrunner.h"
#include "utils/btormem.h"
#include "utils/btorstack.h"
#include "utils/btorutil.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------*/

#define BTOR_TEST_BITVEC_NUM_BITS 65
#define BTOR_TEST_BITVEC_TESTS 100000
#define BTOR_TEST_BITVEC_PERF_TESTS 1000000

static Btor *g_btor;
static BtorMemMgr *g_mm;
static BtorRNG *g_rng;

/*------------------------------------------------------------------------*/

void
init_bitvec_tests (void)
{
  g_btor = btor_new_btor ();
  g_mm   = btor_get_mem_mgr_btor (g_btor);
  btor_init_rng (&g_btor->rng, btor_get_opt (g_btor, BTOR_OPT_SEED));
  g_rng = &g_btor->rng;
}

/*------------------------------------------------------------------------*/

static void
test_new_bitvec (void)
{
  BtorBitVector *bv;

  bv = btor_new_bv (g_mm, BTOR_BV_TYPE_BW);
  assert (bv->len == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_new_bv (g_mm, BTOR_BV_TYPE_BW - 1);
  assert (bv->len == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_new_bv (g_mm, BTOR_BV_TYPE_BW + 1);
  assert (bv->len == 2);
  btor_free_bv (g_mm, bv);
}

static BtorBitVector *
random_bv (uint32_t bw)
{
  uint32_t i;
  BtorBitVector *res;
  res = btor_new_bv (g_mm, bw);

  for (i = 0; i < res->len; i++) res->bits[i] = (BTOR_BV_TYPE) rand ();

  if (bw != BTOR_BV_TYPE_BW * res->len)
    res->bits[0] &= ((((BTOR_BV_TYPE) 1 << (BTOR_BV_TYPE_BW - 1)) - 1)
                     >> (BTOR_BV_TYPE_BW - 1 - (bw % BTOR_BV_TYPE_BW)));

  return res;
}

static void
test_new_random_range_bitvec (void)
{
  uint32_t bw;
  uint64_t val;
  BtorBitVector *bv, *from, *to, *tmp;

  for (bw = 1; bw <= 64; bw++)
  {
    from = random_bv (bw);
    // from == to
    bv  = btor_new_random_range_bv (g_mm, g_rng, bw, from, from);
    val = btor_bv_to_uint64_bv (bv);
    assert (val == btor_bv_to_uint64_bv (from));
    btor_free_bv (g_mm, bv);
    // from < to
    to = random_bv (bw);
    while (!btor_compare_bv (from, to))
    {
      btor_free_bv (g_mm, to);
      to = random_bv (bw);
    }
    if (btor_bv_to_uint64_bv (to) < btor_bv_to_uint64_bv (from))
    {
      tmp  = to;
      to   = from;
      from = tmp;
    }
    bv  = btor_new_random_range_bv (g_mm, g_rng, bw, from, to);
    val = btor_bv_to_uint64_bv (bv);
    assert (val >= btor_bv_to_uint64_bv (from));
    assert (val <= btor_bv_to_uint64_bv (to));
    btor_free_bv (g_mm, from);
    btor_free_bv (g_mm, to);
    btor_free_bv (g_mm, bv);
  }
}

/*------------------------------------------------------------------------*/

static void
test_uint64_to_bitvec (void)
{
  uint64_t i, j, k, l;
  BtorBitVector *bv;

  bv = btor_uint64_to_bv (g_mm, 0, 32);
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_uint64_to_bv (g_mm, UINT_MAX, 32);
  assert (btor_bv_to_uint64_bv (bv) == UINT_MAX);
  btor_free_bv (g_mm, bv);

  for (i = 0; i < 10; i++)
  {
    for (j = 0; j < 5; j++)
    {
      l  = rand () % 32 + 1;
      bv = random_bv (l);
      k  = btor_bv_to_uint64_bv (bv);
      btor_free_bv (g_mm, bv);
      bv = btor_uint64_to_bv (g_mm, k, l);
      assert (btor_bv_to_uint64_bv (bv) == k);
      btor_free_bv (g_mm, bv);
    }
  }
}

static void
test_uint64_to_bv_to_uint64_bitvec (void)
{
  uint64_t i, x, y;
  BtorBitVector *a;

  for (i = 0; i < 10000000; i++)
  {
    x = ((uint64_t) rand ()) << 32;
    x |= (uint64_t) rand ();
    a = btor_uint64_to_bv (g_mm, x, 64);
    y = btor_bv_to_uint64_bv (a);
    assert (x == y);
    btor_free_bv (g_mm, a);
  }
}

/*------------------------------------------------------------------------*/

static void
test_char_to_bitvec (void)
{
  BtorBitVector *bv;

  bv = btor_char_to_bv (g_mm, "0");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "01");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10");
  assert (btor_bv_to_uint64_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11");
  assert (btor_bv_to_uint64_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "010");
  assert (btor_bv_to_uint64_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "011");
  assert (btor_bv_to_uint64_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "100");
  assert (btor_bv_to_uint64_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "101");
  assert (btor_bv_to_uint64_bv (bv) == 5);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "110");
  assert (btor_bv_to_uint64_bv (bv) == 6);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111");
  assert (btor_bv_to_uint64_bv (bv) == 7);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000000001");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000000010");
  assert (btor_bv_to_uint64_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000000100");
  assert (btor_bv_to_uint64_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000001000");
  assert (btor_bv_to_uint64_bv (bv) == 8);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000010000");
  assert (btor_bv_to_uint64_bv (bv) == 16);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000000100000");
  assert (btor_bv_to_uint64_bv (bv) == 32);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000001000000");
  assert (btor_bv_to_uint64_bv (bv) == 64);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000010000000");
  assert (btor_bv_to_uint64_bv (bv) == 128);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000000100000000");
  assert (btor_bv_to_uint64_bv (bv) == 256);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000001000000000");
  assert (btor_bv_to_uint64_bv (bv) == 512);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000010000000000");
  assert (btor_bv_to_uint64_bv (bv) == 1024);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000000100000000000");
  assert (btor_bv_to_uint64_bv (bv) == 2048);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000001000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 4096);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000010000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 8192);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000000100000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 16384);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000001000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 32768);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000010000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 65536);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000000100000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 131072);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000001000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 262144);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000010000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 524288);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000000100000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 1048576);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000001000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 2097152);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000010000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 4194304);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000000100000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 8388608);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000001000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 16777216);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000010000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 33554432);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00000100000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 67108864);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00001000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 134217728);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00010000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 268435456);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00100000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 536870912);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "01000000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 1073741824);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 2147483648);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11111111111111111111111111111111");
  assert (btor_bv_to_uint64_bv (bv) == UINT_MAX);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000000000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000000000000000000000000000000001");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111111111111111111111111111111111");
  assert (btor_bv_to_uint64_bv (bv) == 8589934591);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000000000000000000000000000000000");
  assert (btor_bv_to_uint64_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000000000000000000000000000000001");
  assert (btor_bv_to_uint64_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1111111111111111111111111111111111");
  assert (btor_bv_to_uint64_bv (bv) == 17179869183);
  btor_free_bv (g_mm, bv);
}

#define CHECK_CHAR_TO_BV(bv, i)              \
  do                                         \
  {                                          \
    s = btor_bv_to_char_bv (g_mm, bv);       \
    assert (strlen (s) == bv->width);        \
    for (k = 0; k < i; k++)                  \
    {                                        \
      b = s[i - k - 1] == '0' ? 0 : 1;       \
      assert (b == btor_get_bit_bv (bv, k)); \
    }                                        \
    btor_freestr (g_mm, s);                  \
    btor_free_bv (g_mm, bv);                 \
  } while (0)

static void
test_bv_to_char_bitvec (void)
{
  uint32_t i, j, k;
  int b;
  char *s;
  BtorBitVector *bv;

  for (i = 1; i < 4; i++)
  {
    for (j = 0; j < (1u << i); j++)
    {
      bv = btor_uint64_to_bv (g_mm, j, i);
      CHECK_CHAR_TO_BV (bv, i);
    }
  }

  bv = btor_uint64_to_bv (g_mm, UINT_MAX, 32);
  CHECK_CHAR_TO_BV (bv, 32);

  bv = btor_uint64_to_bv (g_mm, 0, 32);
  CHECK_CHAR_TO_BV (bv, 32);

  bv = btor_uint64_to_bv (g_mm, 1, 32);
  CHECK_CHAR_TO_BV (bv, 32);

  for (i = 0; i < 20; i++)
  {
    bv = btor_new_random_bv (g_mm, g_rng, 32);
    CHECK_CHAR_TO_BV (bv, 32);
  }

  bv = btor_uint64_to_bv (g_mm, 8589934591, 33);
  CHECK_CHAR_TO_BV (bv, 33);

  bv = btor_uint64_to_bv (g_mm, 0, 33);
  CHECK_CHAR_TO_BV (bv, 33);

  bv = btor_uint64_to_bv (g_mm, 1, 33);
  CHECK_CHAR_TO_BV (bv, 33);

  bv = btor_uint64_to_bv (g_mm, 17179869183, 34);
  CHECK_CHAR_TO_BV (bv, 34);

  bv = btor_uint64_to_bv (g_mm, 0, 34);
  CHECK_CHAR_TO_BV (bv, 34);

  bv = btor_uint64_to_bv (g_mm, 1, 34);
  CHECK_CHAR_TO_BV (bv, 34);
}

/*------------------------------------------------------------------------*/

static void
bv_to_hex_char_bitvec (FILE *g_logfile, char *c)
{
  BtorBitVector *bv = btor_char_to_bv (g_mm, c);
  char *h           = btor_bv_to_hex_char_bv (g_mm, bv);
  fprintf (g_logfile, "2'%s = 16'%s\n", c, h);
  btor_freestr (g_mm, h);
  btor_free_bv (g_mm, bv);
}

static void
test_bv_to_hex_char_bitvec (void)
{
  bv_to_hex_char_bitvec (g_logfile, "1");
  bv_to_hex_char_bitvec (g_logfile, "10");
  bv_to_hex_char_bitvec (g_logfile, "11");
  bv_to_hex_char_bitvec (g_logfile, "100");
  bv_to_hex_char_bitvec (g_logfile, "101");
  bv_to_hex_char_bitvec (g_logfile, "110");
  bv_to_hex_char_bitvec (g_logfile, "111");
  bv_to_hex_char_bitvec (g_logfile, "1000");
  bv_to_hex_char_bitvec (g_logfile, "1001");
  bv_to_hex_char_bitvec (g_logfile, "1010");
  bv_to_hex_char_bitvec (g_logfile, "1011");
  bv_to_hex_char_bitvec (g_logfile, "1100");
  bv_to_hex_char_bitvec (g_logfile, "1101");
  bv_to_hex_char_bitvec (g_logfile, "1110");
  bv_to_hex_char_bitvec (g_logfile, "1111");
  bv_to_hex_char_bitvec (g_logfile, "10000");
  bv_to_hex_char_bitvec (g_logfile, "10001");
  bv_to_hex_char_bitvec (g_logfile, "1111111111111111");
  bv_to_hex_char_bitvec (g_logfile, "11111111111111111");
  bv_to_hex_char_bitvec (g_logfile, "00001111111111111111");
  bv_to_hex_char_bitvec (g_logfile, "000011111111111111111");
}

/*------------------------------------------------------------------------*/

static void
bv_to_dec_char_bitvec (FILE *g_logfile, char *c)
{
  BtorBitVector *bv = btor_char_to_bv (g_mm, c);
  char *d           = btor_bv_to_dec_char_bv (g_mm, bv);
  fprintf (g_logfile, "2'%s = 10'%s\n", c, d);
  btor_freestr (g_mm, d);
  btor_free_bv (g_mm, bv);
}

static void
test_bv_to_dec_char_bitvec (void)
{
  bv_to_dec_char_bitvec (g_logfile, "1");
  bv_to_dec_char_bitvec (g_logfile, "10");
  bv_to_dec_char_bitvec (g_logfile, "11");
  bv_to_dec_char_bitvec (g_logfile, "100");
  bv_to_dec_char_bitvec (g_logfile, "101");
  bv_to_dec_char_bitvec (g_logfile, "110");
  bv_to_dec_char_bitvec (g_logfile, "111");
  bv_to_dec_char_bitvec (g_logfile, "1000");
  bv_to_dec_char_bitvec (g_logfile, "1001");
  bv_to_dec_char_bitvec (g_logfile, "1010");
  bv_to_dec_char_bitvec (g_logfile, "1011");
  bv_to_dec_char_bitvec (g_logfile, "1100");
  bv_to_dec_char_bitvec (g_logfile, "1101");
  bv_to_dec_char_bitvec (g_logfile, "1110");
  bv_to_dec_char_bitvec (g_logfile, "1111");
  bv_to_dec_char_bitvec (g_logfile, "10000");
  bv_to_dec_char_bitvec (g_logfile, "10001");
  bv_to_dec_char_bitvec (g_logfile, "10000000000000000");
  bv_to_dec_char_bitvec (g_logfile,
                         "1"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000");
  bv_to_dec_char_bitvec (g_logfile,
                         "1"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000");
  bv_to_dec_char_bitvec (g_logfile,
                         "1"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000"
                         "00000000");
}

/*------------------------------------------------------------------------*/

static uint64_t not(uint64_t x, uint32_t bw)
{
  return ~x % (uint64_t) pow (2, bw);
}

static uint64_t
neg (uint64_t x, uint32_t bw)
{
  return -x % (uint64_t) pow (2, bw);
}

static uint64_t
inc (uint64_t x, uint32_t bw)
{
  return (x + 1) % (uint64_t) pow (2, bw);
}

static uint64_t
dec (uint64_t x, uint32_t bw)
{
  return (x - 1) % (uint64_t) pow (2, bw);
}

static void
unary_bitvec (uint64_t (*int_func) (uint64_t, uint32_t),
              BtorBitVector *(*bitvec_func) (BtorMemMgr *, BtorBitVector *),
              uint32_t num_tests,
              uint32_t bit_width)
{
  uint32_t i;
  BtorBitVector *bv, *res;
  uint64_t a, ares, bres;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    bv   = random_bv (bit_width);
    res  = bitvec_func (g_mm, bv);
    a    = btor_bv_to_uint64_bv (bv);
    ares = int_func (a, bit_width);
    bres = btor_bv_to_uint64_bv (res);
    assert (ares == bres);
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv);
  }
}

static uint64_t
add (uint64_t x, uint64_t y, uint32_t bw)
{
  return (x + y) % (uint64_t) pow (2, bw);
}

static uint64_t
sub (uint64_t x, uint64_t y, uint32_t bw)
{
  return (x - y) % (uint64_t) pow (2, bw);
}

static uint64_t and (uint64_t x, uint64_t y, uint32_t bw)
{
  (void) bw;
  return x & y;
}

static uint64_t
xor (uint64_t x, uint64_t y, uint32_t bw)
{
  (void) bw;
  return x ^ y;
}

static uint64_t
eq (uint64_t x, uint64_t y, uint32_t bw)
{
  (void) bw;
  return x == y;
}

static uint64_t
ult (uint64_t x, uint64_t y, uint32_t bw)
{
  (void) bw;
  return x < y;
}

static uint64_t
sll (uint64_t x, uint64_t y, uint32_t bw)
{
  return (x << y) % (uint64_t) pow (2, bw);
}

static uint64_t
srl (uint64_t x, uint64_t y, uint32_t bw)
{
  return (x >> y) % (uint64_t) pow (2, bw);
}

static uint64_t
mul (uint64_t x, uint64_t y, uint32_t bw)
{
  return (x * y) % (uint64_t) pow (2, bw);
}

static uint64_t
udiv (uint64_t x, uint64_t y, uint32_t bw)
{
  if (y == 0) return UINT_MAX % (uint64_t) pow (2, bw);
  return (x / y) % (uint64_t) pow (2, bw);
}

static uint64_t
urem (uint64_t x, uint64_t y, uint32_t bw)
{
  if (y == 0) return x;
  return (x % y) % (uint64_t) pow (2, bw);
}

static void
binary_bitvec (uint64_t (*int_func) (uint64_t, uint64_t, uint32_t),
               BtorBitVector *(*bitvec_func) (BtorMemMgr *,
                                              BtorBitVector *,
                                              BtorBitVector *),
               uint32_t num_tests,
               uint32_t bit_width)
{
  uint32_t i;
  BtorBitVector *bv1, *bv2, *res;
  uint64_t a1, a2, ares, bres;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    bv1 = random_bv (bit_width);
    if (int_func == sll || int_func == srl)
      bv2 = random_bv (btor_log_2_util (bit_width));
    else
      bv2 = random_bv (bit_width);
    res  = bitvec_func (g_mm, bv1, bv2);
    a1   = btor_bv_to_uint64_bv (bv1);
    a2   = btor_bv_to_uint64_bv (bv2);
    ares = int_func (a1, a2, bit_width);
    bres = btor_bv_to_uint64_bv (res);
    assert (ares == bres);
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
  }
}

static void
test_one_bitvec (void)
{
  int i;
  char *s, *sbv;
  BtorBitVector *bv;

  for (i = 1; i < 32; i++)
  {
    bv = btor_one_bv (g_mm, i);
    BTOR_CNEWN (g_mm, s, i + 1);
    memset (s, '0', i - 1);
    s[i - 1] = '1';
    sbv      = btor_bv_to_char_bv (g_mm, bv);
    assert (!strcmp (s, sbv));
    btor_free_bv (g_mm, bv);
    BTOR_DELETEN (g_mm, s, i + 1);
    btor_freestr (g_mm, sbv);
  }
}

static void
test_ones_bitvec (void)
{
  int i;
  char *s, *sbv;
  BtorBitVector *bv;

  for (i = 1; i < 32; i++)
  {
    bv = btor_ones_bv (g_mm, i);
    BTOR_CNEWN (g_mm, s, i + 1);
    memset (s, '1', i);
    sbv = btor_bv_to_char_bv (g_mm, bv);
    assert (!strcmp (s, sbv));
    btor_free_bv (g_mm, bv);
    BTOR_DELETEN (g_mm, s, i + 1);
    btor_freestr (g_mm, sbv);
  }
}

static void
test_not_bitvec (void)
{
  unary_bitvec (not, btor_not_bv, BTOR_TEST_BITVEC_TESTS, 1);
  unary_bitvec (not, btor_not_bv, BTOR_TEST_BITVEC_TESTS, 7);
  unary_bitvec (not, btor_not_bv, BTOR_TEST_BITVEC_TESTS, 31);
  unary_bitvec (not, btor_not_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_neg_bitvec (void)
{
  unary_bitvec (neg, btor_neg_bv, BTOR_TEST_BITVEC_TESTS, 1);
  unary_bitvec (neg, btor_neg_bv, BTOR_TEST_BITVEC_TESTS, 7);
  unary_bitvec (neg, btor_neg_bv, BTOR_TEST_BITVEC_TESTS, 31);
  unary_bitvec (neg, btor_neg_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_inc_bitvec (void)
{
  unary_bitvec (inc, btor_inc_bv, BTOR_TEST_BITVEC_TESTS, 1);
  unary_bitvec (inc, btor_inc_bv, BTOR_TEST_BITVEC_TESTS, 7);
  unary_bitvec (inc, btor_inc_bv, BTOR_TEST_BITVEC_TESTS, 31);
  unary_bitvec (inc, btor_inc_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_dec_bitvec (void)
{
  unary_bitvec (dec, btor_dec_bv, BTOR_TEST_BITVEC_TESTS, 1);
  unary_bitvec (dec, btor_dec_bv, BTOR_TEST_BITVEC_TESTS, 7);
  unary_bitvec (dec, btor_dec_bv, BTOR_TEST_BITVEC_TESTS, 31);
  unary_bitvec (dec, btor_dec_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_add_bitvec (void)
{
  binary_bitvec (add, btor_add_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (add, btor_add_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (add, btor_add_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (add, btor_add_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_sub_bitvec (void)
{
  binary_bitvec (sub, btor_sub_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (sub, btor_sub_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (sub, btor_sub_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (sub, btor_sub_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_and_bitvec (void)
{
  binary_bitvec (and, btor_and_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (and, btor_and_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (and, btor_and_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (and, btor_and_bv, BTOR_TEST_BITVEC_TESTS, 33);
  binary_bitvec (and, btor_and_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
test_xor_bitvec (void)
{
  binary_bitvec (xor, btor_xor_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (xor, btor_xor_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (xor, btor_xor_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (xor, btor_xor_bv, BTOR_TEST_BITVEC_TESTS, 33);
  binary_bitvec (xor, btor_xor_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
test_eq_bitvec (void)
{
  binary_bitvec (eq, btor_eq_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (eq, btor_eq_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (eq, btor_eq_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (eq, btor_eq_bv, BTOR_TEST_BITVEC_TESTS, 33);
  binary_bitvec (eq, btor_eq_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
test_ult_bitvec (void)
{
  binary_bitvec (ult, btor_ult_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (ult, btor_ult_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (ult, btor_ult_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (ult, btor_ult_bv, BTOR_TEST_BITVEC_TESTS, 33);
  binary_bitvec (ult, btor_ult_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
test_sll_bitvec (void)
{
  binary_bitvec (sll, btor_sll_bv, BTOR_TEST_BITVEC_TESTS, 2);
  binary_bitvec (sll, btor_sll_bv, BTOR_TEST_BITVEC_TESTS, 8);
  binary_bitvec (sll, btor_sll_bv, BTOR_TEST_BITVEC_TESTS, 16);
  binary_bitvec (sll, btor_sll_bv, BTOR_TEST_BITVEC_TESTS, 32);
}

static void
test_srl_bitvec (void)
{
  binary_bitvec (srl, btor_srl_bv, BTOR_TEST_BITVEC_TESTS, 2);
  binary_bitvec (srl, btor_srl_bv, BTOR_TEST_BITVEC_TESTS, 8);
  binary_bitvec (srl, btor_srl_bv, BTOR_TEST_BITVEC_TESTS, 16);
  binary_bitvec (srl, btor_srl_bv, BTOR_TEST_BITVEC_TESTS, 32);
}

static void
test_mul_bitvec (void)
{
  binary_bitvec (mul, btor_mul_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (mul, btor_mul_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (mul, btor_mul_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (mul, btor_mul_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_udiv_bitvec (void)
{
  binary_bitvec (udiv, btor_udiv_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (udiv, btor_udiv_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (udiv, btor_udiv_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (udiv, btor_udiv_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
test_urem_bitvec (void)
{
  binary_bitvec (urem, btor_urem_bv, BTOR_TEST_BITVEC_TESTS, 1);
  binary_bitvec (urem, btor_urem_bv, BTOR_TEST_BITVEC_TESTS, 7);
  binary_bitvec (urem, btor_urem_bv, BTOR_TEST_BITVEC_TESTS, 31);
  binary_bitvec (urem, btor_urem_bv, BTOR_TEST_BITVEC_TESTS, 33);
}

static void
concat_bitvec (int32_t num_tests, uint32_t bit_width)
{
  int i;
  uint32_t bw1, bw2;
  BtorBitVector *bv1, *bv2, *res;
  uint64_t a1, a2, ares, bres;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    bw1 = btor_pick_rand_rng (g_rng, 1, bit_width - 1);
    bw2 = bit_width - bw1;
    bv1 = random_bv (bw1);
    bv2 = random_bv (bw2);
    res = btor_concat_bv (g_mm, bv1, bv2);
    assert (res->width == bw1 + bw2);
    a1   = btor_bv_to_uint64_bv (bv1);
    a2   = btor_bv_to_uint64_bv (bv2);
    ares = (a1 << bw2) | a2;
    bres = btor_bv_to_uint64_bv (res);
    assert (ares == bres);
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
  }
}

static void
test_concat_bitvec (void)
{
  concat_bitvec (BTOR_TEST_BITVEC_TESTS, 2);
  concat_bitvec (BTOR_TEST_BITVEC_TESTS, 7);
  concat_bitvec (BTOR_TEST_BITVEC_TESTS, 31);
  concat_bitvec (BTOR_TEST_BITVEC_TESTS, 33);
  concat_bitvec (BTOR_TEST_BITVEC_TESTS, 64);
}

static void
slice_bitvec (uint32_t num_tests, uint32_t bit_width)
{
  uint32_t i, upper, lower;
  char *sbv, *sres;
  BtorBitVector *bv, *res;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    bv    = random_bv (bit_width);
    lower = rand () % bit_width;
    upper = rand () % (bit_width - lower) + lower;
    assert (upper >= lower);
    assert (upper < bit_width);
    assert (lower < bit_width);

    res = btor_slice_bv (g_mm, bv, upper, lower);
    assert (res->width == upper - lower + 1);
    sres = btor_bv_to_char_bv (g_mm, res);
    sbv  = btor_bv_to_char_bv (g_mm, bv);

    assert (!strncmp (sbv + bit_width - upper - 1, sres, upper - lower + 1));

    btor_freestr (g_mm, sbv);
    btor_freestr (g_mm, sres);
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv);
  }
}

static void
test_slice_bitvec (void)
{
  slice_bitvec (BTOR_TEST_BITVEC_TESTS, 1);
  slice_bitvec (BTOR_TEST_BITVEC_TESTS, 7);
  slice_bitvec (BTOR_TEST_BITVEC_TESTS, 31);
  slice_bitvec (BTOR_TEST_BITVEC_TESTS, 33);
  slice_bitvec (BTOR_TEST_BITVEC_TESTS, 64);
}

static void
ext_bitvec (BtorBitVector *(*ext_func) (BtorMemMgr *,
                                        BtorBitVector *,
                                        uint32_t),
            uint32_t num_tests,
            uint32_t bit_width)
{
  uint32_t i, j, len;
  char *sbv, *sres;
  BtorBitVector *bv, *res;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    len = btor_pick_rand_rng (g_rng, 1, bit_width - 1);
    bv  = random_bv (bit_width - len);

    res = ext_func (g_mm, bv, len);
    assert (bv->width + len == res->width);
    sres = btor_bv_to_char_bv (g_mm, res);
    sbv  = btor_bv_to_char_bv (g_mm, bv);

    assert (!strncmp (sbv, sres + len, bit_width - len));

    for (j = 0; j < len; j++)
      assert (sres[j] == (ext_func == btor_uext_bv ? '0' : sbv[0]));

    btor_freestr (g_mm, sbv);
    btor_freestr (g_mm, sres);
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv);
  }
}

static void
test_uext_bitvec (void)
{
  ext_bitvec (btor_uext_bv, BTOR_TEST_BITVEC_TESTS, 2);
  ext_bitvec (btor_uext_bv, BTOR_TEST_BITVEC_TESTS, 7);
  ext_bitvec (btor_uext_bv, BTOR_TEST_BITVEC_TESTS, 31);
  ext_bitvec (btor_uext_bv, BTOR_TEST_BITVEC_TESTS, 33);
  ext_bitvec (btor_uext_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
test_sext_bitvec (void)
{
  ext_bitvec (btor_sext_bv, BTOR_TEST_BITVEC_TESTS, 2);
  ext_bitvec (btor_sext_bv, BTOR_TEST_BITVEC_TESTS, 7);
  ext_bitvec (btor_sext_bv, BTOR_TEST_BITVEC_TESTS, 31);
  ext_bitvec (btor_sext_bv, BTOR_TEST_BITVEC_TESTS, 33);
  ext_bitvec (btor_sext_bv, BTOR_TEST_BITVEC_TESTS, 64);
}

static void
flipped_bit_bitvec (uint32_t num_tests, uint32_t bit_width)
{
  uint32_t i, j, pos;
  BtorBitVector *bv, *res;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    pos = btor_pick_rand_rng (g_rng, 0, bit_width - 1);
    bv  = random_bv (bit_width);
    res = btor_flipped_bit_bv (g_mm, bv, pos);
    assert (btor_get_bit_bv (bv, pos) == !btor_get_bit_bv (res, pos));
    for (j = 0; j < bit_width; j++)
    {
      if (j == pos) continue;
      assert (btor_get_bit_bv (bv, j) == btor_get_bit_bv (res, j));
    }
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv);
  }
}

static void
test_flipped_bit_bitvec (void)
{
  flipped_bit_bitvec (BTOR_TEST_BITVEC_TESTS, 1);
  flipped_bit_bitvec (BTOR_TEST_BITVEC_TESTS, 7);
  flipped_bit_bitvec (BTOR_TEST_BITVEC_TESTS, 31);
  flipped_bit_bitvec (BTOR_TEST_BITVEC_TESTS, 33);
  flipped_bit_bitvec (BTOR_TEST_BITVEC_TESTS, 64);
}

static void
flipped_bit_range_bitvec (uint32_t num_tests, uint32_t bit_width)
{
  uint32_t i, j, up, lo;
  BtorBitVector *bv, *res;

  printf (" %u", bit_width);
  fflush (stdout);
  for (i = 0; i < num_tests; i++)
  {
    lo = btor_pick_rand_rng (g_rng, 0, bit_width - 1);
    up = lo == bit_width - 1
             ? bit_width - 1
             : btor_pick_rand_rng (g_rng, lo + 1, bit_width - 1);
    bv  = random_bv (bit_width);
    res = btor_flipped_bit_range_bv (g_mm, bv, up, lo);
    for (j = lo; j <= up; j++)
      assert (btor_get_bit_bv (bv, j) == !btor_get_bit_bv (res, j));
    for (j = 0; j < lo; j++)
      assert (btor_get_bit_bv (bv, j) == btor_get_bit_bv (res, j));
    for (j = up + 1; j < bit_width; j++)
      assert (btor_get_bit_bv (bv, j) == btor_get_bit_bv (res, j));
    btor_free_bv (g_mm, res);
    btor_free_bv (g_mm, bv);
  }
}

static void
test_flipped_bit_range_bitvec (void)
{
  flipped_bit_range_bitvec (BTOR_TEST_BITVEC_TESTS, 1);
  flipped_bit_range_bitvec (BTOR_TEST_BITVEC_TESTS, 7);
  flipped_bit_range_bitvec (BTOR_TEST_BITVEC_TESTS, 31);
  flipped_bit_range_bitvec (BTOR_TEST_BITVEC_TESTS, 33);
  flipped_bit_range_bitvec (BTOR_TEST_BITVEC_TESTS, 64);
}

#define TEST_IS_UMULO_BITVEC(bw, v0, v1, res)          \
  do                                                   \
  {                                                    \
    bv0 = btor_uint64_to_bv (g_mm, v0, bw);            \
    bv1 = btor_uint64_to_bv (g_mm, v1, bw);            \
    assert (btor_is_umulo_bv (g_mm, bv0, bv1) == res); \
    btor_free_bv (g_mm, bv0);                          \
    btor_free_bv (g_mm, bv1);                          \
  } while (0)

static void
is_umulo_bitvec (int bw)
{
  BtorBitVector *bv0, *bv1;

  switch (bw)
  {
    case 1:
      TEST_IS_UMULO_BITVEC (bw, 0, 0, false);
      TEST_IS_UMULO_BITVEC (bw, 0, 1, false);
      TEST_IS_UMULO_BITVEC (bw, 1, 1, false);
      break;
    case 7:
      TEST_IS_UMULO_BITVEC (bw, 3, 6, false);
      TEST_IS_UMULO_BITVEC (bw, 124, 2, true);
      break;
    case 31:
      TEST_IS_UMULO_BITVEC (bw, 15, 78, false);
      TEST_IS_UMULO_BITVEC (bw, 1073742058, 2, true);
      break;
    case 33:
      TEST_IS_UMULO_BITVEC (bw, 15, 78, false);
      TEST_IS_UMULO_BITVEC (bw, 4294967530, 4294967530, true);
      break;
  }
}

static void
test_is_umulo_bitvec (void)
{
  is_umulo_bitvec (1);
  is_umulo_bitvec (7);
  is_umulo_bitvec (31);
  is_umulo_bitvec (33);
}

static void
test_compare_bitvec (void)
{
  int i, j, k;
  BtorBitVector *bv1, *bv2;

  for (i = 0; i < 15; i++)
  {
    bv1 = btor_uint64_to_bv (g_mm, i, 4);
    bv2 = btor_uint64_to_bv (g_mm, i, 4);
    assert (!btor_compare_bv (bv1, bv2));
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
  }

  for (i = 0; i < 15 - 1; i++)
  {
    bv1 = btor_uint64_to_bv (g_mm, i, 4);
    bv2 = btor_uint64_to_bv (g_mm, i + 1, 4);
    assert (btor_compare_bv (bv1, bv2) < 0);
    assert (btor_compare_bv (bv2, bv1) > 0);
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
  }

  for (i = 0, j = 0, k = 0; i < 15; i++)
  {
    k = rand () % 16;
    do
      j = rand () % 16;
    while (j == k);
    bv1 = btor_uint64_to_bv (g_mm, j, 4);
    bv2 = btor_uint64_to_bv (g_mm, k, 4);
    if (j > k)
    {
      assert (btor_compare_bv (bv1, bv2) > 0);
      assert (btor_compare_bv (bv2, bv1) < 0);
    }
    if (j < k)
    {
      assert (btor_compare_bv (bv1, bv2) < 0);
      assert (btor_compare_bv (bv2, bv1) > 0);
    }
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
  }
}

static void
test_is_one_bitvec (void)
{
  int i;
  char *s;
  BtorBitVector *bv1, *bv2, *bv3;

  for (i = 1; i < 32; i++)
  {
    bv1 = btor_one_bv (g_mm, i);
    bv2 = btor_uint64_to_bv (g_mm, 1, i);
    BTOR_CNEWN (g_mm, s, i + 1);
    memset (s, '0', i - 1);
    s[i - 1] = '1';
    bv3      = btor_char_to_bv (g_mm, s);
    assert (btor_is_one_bv (bv1));
    assert (btor_is_one_bv (bv2));
    assert (btor_is_one_bv (bv3));
    assert (!btor_compare_bv (bv1, bv2));
    assert (!btor_compare_bv (bv1, bv3));
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
    btor_free_bv (g_mm, bv3);
    BTOR_DELETEN (g_mm, s, i + 1);
  }
}

static void
test_is_ones_bitvec (void)
{
  int i;
  char *s;
  BtorBitVector *bv1, *bv2, *bv3;

  for (i = 1; i < 32; i++)
  {
    bv1 = btor_ones_bv (g_mm, i);
    bv2 = btor_uint64_to_bv (g_mm, UINT_MAX, i);
    BTOR_CNEWN (g_mm, s, i + 1);
    memset (s, '1', i);
    bv3 = btor_char_to_bv (g_mm, s);
    assert (btor_is_ones_bv (bv1));
    assert (btor_is_ones_bv (bv2));
    assert (btor_is_ones_bv (bv3));
    assert (!btor_compare_bv (bv1, bv2));
    assert (!btor_compare_bv (bv1, bv3));
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
    btor_free_bv (g_mm, bv3);
    BTOR_DELETEN (g_mm, s, i + 1);
  }
}

static void
test_is_zero_bitvec (void)
{
  int i;
  char *s;
  BtorBitVector *bv1, *bv2, *bv3;

  for (i = 1; i < 32; i++)
  {
    bv1 = btor_new_bv (g_mm, i);
    bv2 = btor_uint64_to_bv (g_mm, 0, i);
    BTOR_CNEWN (g_mm, s, i + 1);
    memset (s, '0', i);
    bv3 = btor_char_to_bv (g_mm, s);
    assert (btor_is_zero_bv (bv1));
    assert (btor_is_zero_bv (bv2));
    assert (btor_is_zero_bv (bv3));
    assert (!btor_compare_bv (bv1, bv2));
    assert (!btor_compare_bv (bv1, bv3));
    btor_free_bv (g_mm, bv1);
    btor_free_bv (g_mm, bv2);
    btor_free_bv (g_mm, bv3);
    BTOR_DELETEN (g_mm, s, i + 1);
  }
}

static void
test_is_special_const_bitvec (void)
{
  int i;
  BtorBitVector *bv;

  bv = btor_char_to_bv (g_mm, "0");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ZERO);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONE_ONES);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ZERO);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "01");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONE);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_NONE);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONES);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ZERO);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONE);
  btor_free_bv (g_mm, bv);

  for (i = 2; i < 7; i++)
  {
    bv = btor_uint64_to_bv (g_mm, i, 3);
    assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_NONE);
    btor_free_bv (g_mm, bv);
  }

  bv = btor_char_to_bv (g_mm, "111");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONES);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ZERO);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONE);
  btor_free_bv (g_mm, bv);

  for (i = 2; i < 15; i++)
  {
    bv = btor_uint64_to_bv (g_mm, i, 4);
    assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_NONE);
    btor_free_bv (g_mm, bv);
  }

  bv = btor_char_to_bv (g_mm, "1111");
  assert (btor_is_special_const_bv (bv) == BTOR_SPECIAL_CONST_BV_ONES);
  btor_free_bv (g_mm, bv);
}

static void
test_power_of_two_bitvec (void)
{
  BtorBitVector *bv;

  bv = btor_char_to_bv (
      g_mm, "0000000000000000000000000000000000000000000000000000000000000000");
  assert (btor_power_of_two_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000");
  assert (btor_power_of_two_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001");
  assert (btor_power_of_two_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010");
  assert (btor_power_of_two_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00100");
  assert (btor_power_of_two_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001000");
  assert (btor_power_of_two_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010000");
  assert (btor_power_of_two_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000");
  assert (btor_power_of_two_bv (bv) == 5);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000");
  assert (btor_power_of_two_bv (bv) == 6);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00010000000");
  assert (btor_power_of_two_bv (bv) == 7);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000000");
  assert (btor_power_of_two_bv (bv) == 8);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000000");
  assert (btor_power_of_two_bv (bv) == 9);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000010000000000");
  assert (btor_power_of_two_bv (bv) == 10);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000000");
  assert (btor_power_of_two_bv (bv) == 28);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "100000000000000000000000000000");
  assert (btor_power_of_two_bv (bv) == 29);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1000000000000000000000000000000");
  assert (btor_power_of_two_bv (bv) == 30);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "01000000000000000000000000000000");
  assert (btor_power_of_two_bv (bv) == 30);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "1110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "11110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "1111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "111111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "1111111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "11111111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "111111111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "1111111111110");
  assert (btor_power_of_two_bv (bv) == -1);

  btor_free_bv (g_mm, bv);
  bv = btor_char_to_bv (g_mm, "1111111111111110");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "011");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0011");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00101");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "101101");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010001");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100111");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1001000001");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11010000001");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000011");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000111");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000010000001111");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000010");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "100000000000000000000001000000");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1000000000000100000000000000000");
  assert (btor_power_of_two_bv (bv) == -1);
  btor_free_bv (g_mm, bv);
}

static void
test_small_positive_int_bitvec (void)
{
  BtorBitVector *bv;

  bv = btor_char_to_bv (
      g_mm, "0000000000000000000000000000000000000000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000");
  assert (btor_small_positive_int_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001");
  assert (btor_small_positive_int_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010");
  assert (btor_small_positive_int_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00100");
  assert (btor_small_positive_int_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "001000");
  assert (btor_small_positive_int_bv (bv) == 8);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010000");
  assert (btor_small_positive_int_bv (bv) == 16);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000");
  assert (btor_small_positive_int_bv (bv) == 32);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000");
  assert (btor_small_positive_int_bv (bv) == 64);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00010000000");
  assert (btor_small_positive_int_bv (bv) == 128);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000000");
  assert (btor_small_positive_int_bv (bv) == 256);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000000");
  assert (btor_small_positive_int_bv (bv) == 512);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000010000000000");
  assert (btor_small_positive_int_bv (bv) == 1024);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) == (1 << 28));
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "100000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) == (1 << 29));
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1000000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) == (1 << 30));
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "01000000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) == (1 << 30));
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "110");
  assert (btor_small_positive_int_bv (bv) == 6);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1110");
  assert (btor_small_positive_int_bv (bv) == 14);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11110");
  assert (btor_small_positive_int_bv (bv) == 30);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111110");
  assert (btor_small_positive_int_bv (bv) == 62);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1111110");
  assert (btor_small_positive_int_bv (bv) == 126);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111111110");
  assert (btor_small_positive_int_bv (bv) == 510);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1111111110");
  assert (btor_small_positive_int_bv (bv) == 1022);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11111111110");
  assert (btor_small_positive_int_bv (bv) == 2046);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111111111110");
  assert (btor_small_positive_int_bv (bv) == 4094);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1111111111110");
  assert (btor_small_positive_int_bv (bv) == 8190);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1111111111111110");
  assert (btor_small_positive_int_bv (bv) == 65534);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "011");
  assert (btor_small_positive_int_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "111");
  assert (btor_small_positive_int_bv (bv) == 7);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0011");
  assert (btor_small_positive_int_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00101");
  assert (btor_small_positive_int_bv (bv) == 5);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "101101");
  assert (btor_small_positive_int_bv (bv) == 45);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "00100001");
  assert (btor_small_positive_int_bv (bv) == 33);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100111");
  assert (btor_small_positive_int_bv (bv) == 39);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1001000001");
  assert (btor_small_positive_int_bv (bv) == 577);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "11010000001");
  assert (btor_small_positive_int_bv (bv) == 1665);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "000100000011");
  assert (btor_small_positive_int_bv (bv) == 259);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0001000000111");
  assert (btor_small_positive_int_bv (bv) == 519);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0000010000001111");
  assert (btor_small_positive_int_bv (bv) == 1039);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000010");
  assert (btor_small_positive_int_bv (bv) == 268435458);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "100000000000000000000001000000");
  assert (btor_small_positive_int_bv (bv) == 536870976);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "1000000000000100000000000000000");
  assert (btor_small_positive_int_bv (bv) == 1073872896);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) < 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "10000100000000000000000011100000");
  assert (btor_small_positive_int_bv (bv) < 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010000000000000000000000000000000");
  assert (btor_small_positive_int_bv (bv) < 0);
  btor_free_bv (g_mm, bv);

  bv = btor_char_to_bv (g_mm, "0010000000000100000000000011110000");
  assert (btor_small_positive_int_bv (bv) < 0);
  btor_free_bv (g_mm, bv);
}

static void
test_get_num_trailing_zeros_bitvec (void)
{
  BtorBitVector *bv;

  // 1000
  bv = btor_uint64_to_bv (g_mm, 8, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  // 0100
  bv = btor_uint64_to_bv (g_mm, 4, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  // 0010
  bv = btor_uint64_to_bv (g_mm, 2, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0001
  bv = btor_uint64_to_bv (g_mm, 1, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 0000
  bv = btor_uint64_to_bv (g_mm, 0, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  // 1111
  bv = btor_uint64_to_bv (g_mm, 15, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 0110
  bv = btor_uint64_to_bv (g_mm, 6, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0111
  bv = btor_uint64_to_bv (g_mm, 7, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 1010
  bv = btor_uint64_to_bv (g_mm, 10, 4);
  assert (btor_get_num_trailing_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0
  bv = btor_uint64_to_bv (g_mm, 0, 1);
  assert (btor_get_num_trailing_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 1
  bv = btor_uint64_to_bv (g_mm, 1, 1);
  assert (btor_get_num_trailing_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);
}

static void
test_get_num_leading_zeros_bitvec (void)
{
  BtorBitVector *bv;

  // 1000
  bv = btor_uint64_to_bv (g_mm, 8, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 0100
  bv = btor_uint64_to_bv (g_mm, 4, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0010
  bv = btor_uint64_to_bv (g_mm, 2, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  // 0001
  bv = btor_uint64_to_bv (g_mm, 1, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  // 0000
  bv = btor_uint64_to_bv (g_mm, 0, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  // 1111
  bv = btor_uint64_to_bv (g_mm, 15, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 0110
  bv = btor_uint64_to_bv (g_mm, 6, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0111
  bv = btor_uint64_to_bv (g_mm, 7, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 1010
  bv = btor_uint64_to_bv (g_mm, 10, 4);
  assert (btor_get_num_leading_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 0
  bv = btor_uint64_to_bv (g_mm, 0, 1);
  assert (btor_get_num_leading_zeros_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 1
  bv = btor_uint64_to_bv (g_mm, 1, 1);
  assert (btor_get_num_leading_zeros_bv (bv) == 0);
  btor_free_bv (g_mm, bv);
}

static void
test_get_num_leading_ones_bitvec (void)
{
  BtorBitVector *bv;

  // 1000
  bv = btor_uint64_to_bv (g_mm, 8, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 1100
  bv = btor_uint64_to_bv (g_mm, 12, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  // 1110
  bv = btor_uint64_to_bv (g_mm, 14, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 3);
  btor_free_bv (g_mm, bv);

  // 1111
  bv = btor_uint64_to_bv (g_mm, 15, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 4);
  btor_free_bv (g_mm, bv);

  // 0000
  bv = btor_uint64_to_bv (g_mm, 0, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 1011
  bv = btor_uint64_to_bv (g_mm, 11, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 1101
  bv = btor_uint64_to_bv (g_mm, 13, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 2);
  btor_free_bv (g_mm, bv);

  // 1001
  bv = btor_uint64_to_bv (g_mm, 9, 4);
  assert (btor_get_num_leading_ones_bv (bv) == 1);
  btor_free_bv (g_mm, bv);

  // 0
  bv = btor_uint64_to_bv (g_mm, 0, 1);
  assert (btor_get_num_leading_ones_bv (bv) == 0);
  btor_free_bv (g_mm, bv);

  // 1
  bv = btor_uint64_to_bv (g_mm, 1, 1);
  assert (btor_get_num_leading_ones_bv (bv) == 1);
  btor_free_bv (g_mm, bv);
}

void
run_bitvec_tests (int argc, char **argv)
{
  srand (42);

  BTOR_RUN_TEST (new_bitvec);
  BTOR_RUN_TEST (new_random_range_bitvec);

  BTOR_RUN_TEST (uint64_to_bitvec);
  BTOR_RUN_TEST (uint64_to_bv_to_uint64_bitvec);
  BTOR_RUN_TEST (char_to_bitvec);
  BTOR_RUN_TEST (bv_to_char_bitvec);
  BTOR_RUN_TEST_CHECK_LOG (bv_to_hex_char_bitvec);
  BTOR_RUN_TEST_CHECK_LOG (bv_to_dec_char_bitvec);

  BTOR_RUN_TEST (one_bitvec);
  BTOR_RUN_TEST (ones_bitvec);

  BTOR_RUN_TEST (not_bitvec);
  BTOR_RUN_TEST (neg_bitvec);
  BTOR_RUN_TEST (inc_bitvec);
  BTOR_RUN_TEST (dec_bitvec);

  BTOR_RUN_TEST (add_bitvec);
  BTOR_RUN_TEST (sub_bitvec);
  BTOR_RUN_TEST (and_bitvec);
  BTOR_RUN_TEST (xor_bitvec);
  BTOR_RUN_TEST (eq_bitvec);
  BTOR_RUN_TEST (ult_bitvec);
  BTOR_RUN_TEST (sll_bitvec);
  BTOR_RUN_TEST (srl_bitvec);
  BTOR_RUN_TEST (mul_bitvec);
  BTOR_RUN_TEST (udiv_bitvec);
  BTOR_RUN_TEST (urem_bitvec);
  BTOR_RUN_TEST (concat_bitvec);
  BTOR_RUN_TEST (slice_bitvec);
  BTOR_RUN_TEST (uext_bitvec);
  BTOR_RUN_TEST (sext_bitvec);

  BTOR_RUN_TEST (flipped_bit_bitvec);
  BTOR_RUN_TEST (flipped_bit_range_bitvec);

  BTOR_RUN_TEST (is_umulo_bitvec);

  BTOR_RUN_TEST (compare_bitvec);

  BTOR_RUN_TEST (is_one_bitvec);
  BTOR_RUN_TEST (is_ones_bitvec);
  BTOR_RUN_TEST (is_zero_bitvec);
  BTOR_RUN_TEST (is_special_const_bitvec);
  BTOR_RUN_TEST (small_positive_int_bitvec);
  BTOR_RUN_TEST (power_of_two_bitvec);
  BTOR_RUN_TEST (get_num_trailing_zeros_bitvec);
  BTOR_RUN_TEST (get_num_leading_zeros_bitvec);
  BTOR_RUN_TEST (get_num_leading_ones_bitvec);
}

void
finish_bitvec_tests (void)
{
  btor_delete_btor (g_btor);
}
