/*
 * Copyright © 2025  Adobe, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Adobe Author(s): Skef Iterum
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Test that GSUB closure completes transitive substitution chains that
 * exceed HB_CLOSURE_MAX_STAGES.
 *
 * AOTS font classdef1_font3.otf has a ContextSubst (format 2) with
 * feature "test" that applies SingleSubst delta=3 to class-0 glyphs.
 * The full chain from g46 (U+002E) is:
 *
 *   g46 -> g49 -> g52 -> ... -> g88 -> g91 -> g94 -> g97
 *
 * This is 18 hops.  With HB_CLOSURE_MAX_STAGES=12, the iterative
 * closure truncates at g88.  The depend-based fallback should
 * complete the chain. */

static void
test_subset_gsub_closure_long_chain (void)
{
  hb_face_t *face = hb_test_open_font_file ("../shape/data/aots/fonts/classdef1_font3.otf");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  g_assert_nonnull (input);

  /* Input: U+002E (g46) */
  hb_set_add (hb_subset_input_unicode_set (input), 0x002E);

  /* Enable only feature "test" */
  hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  hb_set_clear (features);
  hb_set_add (features, HB_TAG ('t','e','s','t'));

  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  g_assert_nonnull (plan);

  /* Extract retained glyph set */
  hb_map_t *glyph_map = hb_subset_plan_old_to_new_glyph_mapping (plan);
  hb_set_t *retained = hb_set_create ();
  {
    int idx = -1;
    hb_codepoint_t key, value;
    while (hb_map_next (glyph_map, &idx, &key, &value))
      hb_set_add (retained, key);
  }

  /* Chain glyphs before the old truncation point */
  g_assert_true (hb_set_has (retained, 46));  /* starting glyph */
  g_assert_true (hb_set_has (retained, 49));
  g_assert_true (hb_set_has (retained, 85));
  g_assert_true (hb_set_has (retained, 88));  /* old last glyph */

  /* Chain glyphs PAST the old truncation point — these require the
   * depend-based fallback to be reached. */
  g_assert_true (hb_set_has (retained, 91));
  g_assert_true (hb_set_has (retained, 94));
  g_assert_true (hb_set_has (retained, 97));

  hb_set_destroy (retained);
  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_gsub_closure_long_chain);

  return hb_test_run ();
}
