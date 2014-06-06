/* gost28147.c - GOST 28147-89 implementation for Libgcrypt
 * Copyright (C) 2012 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* GOST 28147-89 defines several modes of encryption:
 * - ECB which should be used only for key transfer
 * - CFB mode
 * - OFB-like mode with additional transformation on keystream
 *   RFC 5830 names this 'counter encryption' mode
 *   Original GOST text uses the term 'gammirovanie'
 * - MAC mode
 *
 * This implementation handles ECB and CFB modes via usual libgcrypt handling.
 * OFB-like and MAC modes are unsupported.
 */

#include <config.h>
#include "types.h"
#include "g10lib.h"
#include "cipher.h"

#include "gost.h"
#include "gost-sb.h"

static gcry_err_code_t
gost_setkey (void *c, const byte *key, unsigned keylen)
{
  int i;
  GOST28147_context *ctx = c;

  if (keylen != 256 / 8)
    return GPG_ERR_INV_KEYLEN;

  if (!ctx->sbox)
    ctx->sbox = sbox_test_3411;

  for (i = 0; i < 8; i++)
    {
      ctx->key[i] = (key[4 * i + 3] << 24) |
                    (key[4 * i + 2] << 16) |
                    (key[4 * i + 1] <<  8) |
                    (key[4 * i + 0] <<  0);
    }
  return GPG_ERR_NO_ERROR;
}

static u32
gost_val (GOST28147_context *ctx, u32 cm1, int subkey)
{
  cm1 += ctx->key[subkey];
  cm1 = ctx->sbox[0*256 + ((cm1 >>  0) & 0xff)] |
        ctx->sbox[1*256 + ((cm1 >>  8) & 0xff)] |
        ctx->sbox[2*256 + ((cm1 >> 16) & 0xff)] |
        ctx->sbox[3*256 + ((cm1 >> 24) & 0xff)];
  return cm1;
}

static unsigned int
gost_encrypt_block (void *c, byte *outbuf, const byte *inbuf)
{
  GOST28147_context *ctx = c;
  u32 n1, n2;

  n1 =  (inbuf[0] << 0) |
        (inbuf[1] << 8) |
        (inbuf[2] << 16) |
        (inbuf[3] << 24);
  n2 =  (inbuf[4] << 0) |
        (inbuf[5] << 8) |
        (inbuf[6] << 16) |
        (inbuf[7] << 24);

  n2 ^= gost_val (ctx, n1, 0); n1 ^= gost_val (ctx, n2, 1);
  n2 ^= gost_val (ctx, n1, 2); n1 ^= gost_val (ctx, n2, 3);
  n2 ^= gost_val (ctx, n1, 4); n1 ^= gost_val (ctx, n2, 5);
  n2 ^= gost_val (ctx, n1, 6); n1 ^= gost_val (ctx, n2, 7);

  n2 ^= gost_val (ctx, n1, 0); n1 ^= gost_val (ctx, n2, 1);
  n2 ^= gost_val (ctx, n1, 2); n1 ^= gost_val (ctx, n2, 3);
  n2 ^= gost_val (ctx, n1, 4); n1 ^= gost_val (ctx, n2, 5);
  n2 ^= gost_val (ctx, n1, 6); n1 ^= gost_val (ctx, n2, 7);

  n2 ^= gost_val (ctx, n1, 0); n1 ^= gost_val (ctx, n2, 1);
  n2 ^= gost_val (ctx, n1, 2); n1 ^= gost_val (ctx, n2, 3);
  n2 ^= gost_val (ctx, n1, 4); n1 ^= gost_val (ctx, n2, 5);
  n2 ^= gost_val (ctx, n1, 6); n1 ^= gost_val (ctx, n2, 7);

  n2 ^= gost_val (ctx, n1, 7); n1 ^= gost_val (ctx, n2, 6);
  n2 ^= gost_val (ctx, n1, 5); n1 ^= gost_val (ctx, n2, 4);
  n2 ^= gost_val (ctx, n1, 3); n1 ^= gost_val (ctx, n2, 2);
  n2 ^= gost_val (ctx, n1, 1); n1 ^= gost_val (ctx, n2, 0);

  outbuf[0 + 0] = (n2 >> (0 * 8)) & 0xff;
  outbuf[1 + 0] = (n2 >> (1 * 8)) & 0xff;
  outbuf[2 + 0] = (n2 >> (2 * 8)) & 0xff;
  outbuf[3 + 0] = (n2 >> (3 * 8)) & 0xff;
  outbuf[0 + 4] = (n1 >> (0 * 8)) & 0xff;
  outbuf[1 + 4] = (n1 >> (1 * 8)) & 0xff;
  outbuf[2 + 4] = (n1 >> (2 * 8)) & 0xff;
  outbuf[3 + 4] = (n1 >> (3 * 8)) & 0xff;

  return /* burn_stack */ 4*sizeof(void*) /* func call */ +
                          3*sizeof(void*) /* stack */ +
                          4*sizeof(void*) /* gost_val call */;
}

unsigned int _gcry_gost_enc_one (GOST28147_context *c, const byte *key,
    byte *out, byte *in, int cryptopro)
{
  if (cryptopro)
    c->sbox = sbox_CryptoPro_3411;
  else
    c->sbox = sbox_test_3411;
  gost_setkey (c, key, 32);
  return gost_encrypt_block (c, out, in) + 5 * sizeof(void *);
}

static unsigned int
gost_decrypt_block (void *c, byte *outbuf, const byte *inbuf)
{
  GOST28147_context *ctx = c;
  u32 n1, n2;

  n1 =  (inbuf[0] << 0) |
        (inbuf[1] << 8) |
        (inbuf[2] << 16) |
        (inbuf[3] << 24);
  n2 =  (inbuf[4] << 0) |
        (inbuf[5] << 8) |
        (inbuf[6] << 16) |
        (inbuf[7] << 24);

  n2 ^= gost_val (ctx, n1, 0); n1 ^= gost_val (ctx, n2, 1);
  n2 ^= gost_val (ctx, n1, 2); n1 ^= gost_val (ctx, n2, 3);
  n2 ^= gost_val (ctx, n1, 4); n1 ^= gost_val (ctx, n2, 5);
  n2 ^= gost_val (ctx, n1, 6); n1 ^= gost_val (ctx, n2, 7);

  n2 ^= gost_val (ctx, n1, 7); n1 ^= gost_val (ctx, n2, 6);
  n2 ^= gost_val (ctx, n1, 5); n1 ^= gost_val (ctx, n2, 4);
  n2 ^= gost_val (ctx, n1, 3); n1 ^= gost_val (ctx, n2, 2);
  n2 ^= gost_val (ctx, n1, 1); n1 ^= gost_val (ctx, n2, 0);

  n2 ^= gost_val (ctx, n1, 7); n1 ^= gost_val (ctx, n2, 6);
  n2 ^= gost_val (ctx, n1, 5); n1 ^= gost_val (ctx, n2, 4);
  n2 ^= gost_val (ctx, n1, 3); n1 ^= gost_val (ctx, n2, 2);
  n2 ^= gost_val (ctx, n1, 1); n1 ^= gost_val (ctx, n2, 0);

  n2 ^= gost_val (ctx, n1, 7); n1 ^= gost_val (ctx, n2, 6);
  n2 ^= gost_val (ctx, n1, 5); n1 ^= gost_val (ctx, n2, 4);
  n2 ^= gost_val (ctx, n1, 3); n1 ^= gost_val (ctx, n2, 2);
  n2 ^= gost_val (ctx, n1, 1); n1 ^= gost_val (ctx, n2, 0);

  outbuf[0 + 0] = (n2 >> (0 * 8)) & 0xff;
  outbuf[1 + 0] = (n2 >> (1 * 8)) & 0xff;
  outbuf[2 + 0] = (n2 >> (2 * 8)) & 0xff;
  outbuf[3 + 0] = (n2 >> (3 * 8)) & 0xff;
  outbuf[0 + 4] = (n1 >> (0 * 8)) & 0xff;
  outbuf[1 + 4] = (n1 >> (1 * 8)) & 0xff;
  outbuf[2 + 4] = (n1 >> (2 * 8)) & 0xff;
  outbuf[3 + 4] = (n1 >> (3 * 8)) & 0xff;

  return /* burn_stack */ 4*sizeof(void*) /* func call */ +
                          3*sizeof(void*) /* stack */ +
                          4*sizeof(void*) /* gost_val call */;
}

static gpg_err_code_t
gost_set_sbox (GOST28147_context *ctx, const char *oid)
{
  int i;

  for (i = 0; gost_oid_map[i].oid; i++)
    {
      if (!strcmp(gost_oid_map[i].oid, oid))
        {
          ctx->sbox = gost_oid_map[i].sbox;
          return 0;
        }
    }
  return GPG_ERR_VALUE_NOT_FOUND;
}

static gpg_err_code_t
gost_set_extra_info (void *c, int what, const void *buffer, size_t buflen)
{
  GOST28147_context *ctx = c;
  gpg_err_code_t ec = 0;

  (void)buffer;
  (void)buflen;

  switch (what)
    {
    case GCRYCTL_SET_SBOX:
      ec = gost_set_sbox (ctx, buffer);
      break;

    default:
      ec = GPG_ERR_INV_OP;
      break;
    }
  return ec;
}

static gcry_cipher_oid_spec_t oids_gost28147[] =
  {
    /* { "1.2.643.2.2.31.0", GCRY_CIPHER_MODE_CNTGOST }, */
    { "1.2.643.2.2.31.1", GCRY_CIPHER_MODE_CFB },
    { "1.2.643.2.2.31.2", GCRY_CIPHER_MODE_CFB },
    { "1.2.643.2.2.31.3", GCRY_CIPHER_MODE_CFB },
    { "1.2.643.2.2.31.4", GCRY_CIPHER_MODE_CFB },
    { NULL }
  };

gcry_cipher_spec_t _gcry_cipher_spec_gost28147 =
  {
    GCRY_CIPHER_GOST28147, {0, 0},
    "GOST28147", NULL, oids_gost28147, 8, 256,
    sizeof (GOST28147_context),
    gost_setkey,
    gost_encrypt_block,
    gost_decrypt_block,
    NULL, NULL, NULL, gost_set_extra_info,
  };