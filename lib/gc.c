#include "gc.h"

#include <string.h>
#include <unictype.h>
#include <uniname.h>
#include <unistr.h>

static const uc_block_t *all_blocks;
static size_t all_block_count;

struct GcCharacterIter
{
  ucs4_t uc;

  const uc_block_t *blocks;
  size_t block_index;
  size_t block_count;

  gboolean (* filter) (ucs4_t uc, gpointer data);
  GBoxedFreeFunc free_data;
  GBoxedCopyFunc copy_data;
  gpointer data;
};

static GcCharacterIter *
gc_character_iter_copy (GcCharacterIter *src)
{
  GcCharacterIter *dest = g_slice_dup (GcCharacterIter, src);
  if (src->data && src->copy_data)
    dest->data = src->copy_data (src->data);
  return dest;
}

static void
gc_character_iter_free (GcCharacterIter *iter)
{
  if (iter->data && iter->free_data)
    iter->free_data (iter->data);

  g_slice_free (GcCharacterIter, iter);
}

G_DEFINE_BOXED_TYPE (GcCharacterIter, gc_character_iter,
		     gc_character_iter_copy,
		     gc_character_iter_free);

gboolean
gc_character_iter_next (GcCharacterIter *iter)
{
  ucs4_t uc = iter->uc;

  if (!iter->blocks)
    return FALSE;

  while (TRUE)
    {
      if (uc == iter->blocks[iter->block_index].end)
	{
	  iter->block_index++;
	  uc = UNINAME_INVALID;
	}

      if (uc == UNINAME_INVALID)
	{
	  while (iter->block_index < iter->block_count
		 && iter->blocks[iter->block_index].start
		 == iter->blocks[iter->block_index].end)
	    iter->block_index++;
	  if (iter->block_index == iter->block_count)
	    return FALSE;
	  uc = iter->blocks[iter->block_index].start;
	}
      else
	uc++;

      while (uc < iter->blocks[iter->block_index].end
	     && !iter->filter (uc, iter->data))
	uc++;

      if (uc < iter->blocks[iter->block_index].end)
	{
	  iter->uc = uc;
	  return TRUE;
	}
    }

  return FALSE;
}

gunichar
gc_character_iter_get (GcCharacterIter *iter)
{
  return iter->uc;
}

static GcCharacterIter *
gc_character_iter_new (void)
{
  GcCharacterIter *iter = g_slice_new0 (GcCharacterIter);
  iter->uc = UNINAME_INVALID;
  return iter;
}

static gboolean
filter_category (ucs4_t uc, gpointer data)
{
  uc_general_category_t *category = data;
  return uc_is_print (uc) && uc_is_general_category (uc, *category);
}

static gpointer
copy_category (gpointer data)
{
  return g_slice_dup (uc_general_category_t, data);
}

static void
free_category (gpointer data)
{
  g_slice_free (uc_general_category_t, data);
}

static GcCharacterIter *
gc_character_iter_new_for_general_category (uc_general_category_t category)
{
  GcCharacterIter *iter = gc_character_iter_new ();
  iter->blocks = all_blocks;
  iter->block_count = all_block_count;
  iter->filter = filter_category;
  iter->copy_data = copy_category;
  iter->free_data = free_category;
  iter->data = g_slice_dup (uc_general_category_t, &category);
  return iter;
}

static gboolean
filter_is_print (ucs4_t uc, gpointer data)
{
  return uc_is_print (uc);
}

static GcCharacterIter *
gc_character_iter_new_for_blocks (const uc_block_t *blocks,
                                  size_t            block_count)
{
  GcCharacterIter *iter = gc_character_iter_new ();
  iter->blocks = blocks;
  iter->block_count = block_count;
  iter->filter = filter_is_print;
  return iter;
}

static gboolean
filter_script (ucs4_t uc, gpointer data)
{
  uc_script_t *script = data;
  return uc_is_print (uc) && uc_is_script (uc, script);
}

static GcCharacterIter *
gc_character_iter_new_for_script (const uc_script_t *script)
{
  GcCharacterIter *iter = gc_character_iter_new ();
  iter->blocks = all_blocks;
  iter->block_count = all_block_count;
  iter->filter = filter_script;
  iter->data = (gpointer) script;
  return iter;
}

/**
 * gc_enumerate_character_by_category:
 * @category: a #GcCategory.
 *
 * Returns: a #GcCharacterIter.
 */
GcCharacterIter *
gc_enumerate_character_by_category (GcCategory category)
{
  if (!all_blocks)
    uc_all_blocks (&all_blocks, &all_block_count);

  switch (category)
    {
    case GC_CATEGORY_NONE:
      break;

    case GC_CATEGORY_PUNCTUATION:
      return gc_character_iter_new_for_general_category (UC_CATEGORY_P);

    case GC_CATEGORY_ARROW:
      {
	static uc_block_t arrow_blocks[3];
	static gsize arrow_blocks_size = 0;
	static gsize arrow_blocks_initialized = 0;
	if (g_once_init_enter (&arrow_blocks_initialized))
	  {
	    const uc_block_t *block;

	    /* 2190..21FF; Arrows */
	    block = uc_block (0x2190);
	    if (block)
	      memcpy (&arrow_blocks[arrow_blocks_size++], block,
		      sizeof (uc_block_t));
	    /* 27F0..27FF; Supplemental Arrows-A */
	    block = uc_block (0x27F0);
	    if (block)
	      memcpy (&arrow_blocks[arrow_blocks_size++], block,
		      sizeof (uc_block_t));
	    /* 2900..297F; Supplemental Arrows-B */
	    block = uc_block (0x2900);
	    if (block)
	      memcpy (&arrow_blocks[arrow_blocks_size++], block,
		      sizeof (uc_block_t));
	    g_once_init_leave (&arrow_blocks_initialized, 1);
	  }
	return gc_character_iter_new_for_blocks (arrow_blocks,
						 arrow_blocks_size);
      }

    case GC_CATEGORY_BULLET:
      /* Not implemented.  */
      break;

    case GC_CATEGORY_PICTURE:
      {
	static uc_block_t picture_blocks[2];
	static gsize picture_blocks_size = 0;
	static gsize picture_blocks_initialized = 0;
	if (g_once_init_enter (&picture_blocks_initialized))
	  {
	    const uc_block_t *block;

	    /* 2600..26FF; Miscellaneous Symbols */
	    block = uc_block (0x2600);
	    if (block)
	      memcpy (&picture_blocks[picture_blocks_size++], block,
		      sizeof (uc_block_t));
	    /* 2700..27BF; Dingbats */
	    block = uc_block (0x2700);
	    if (block)
	      memcpy (&picture_blocks[picture_blocks_size++], block,
		      sizeof (uc_block_t));
	    g_once_init_leave (&picture_blocks_initialized, 1);
	  }
	return gc_character_iter_new_for_blocks (picture_blocks,
						 picture_blocks_size);
      }
      break;

    case GC_CATEGORY_CURRENCY:
      return gc_character_iter_new_for_general_category (UC_CATEGORY_Sc);

    case GC_CATEGORY_MATH:
      return gc_character_iter_new_for_general_category (UC_CATEGORY_Sm);

    case GC_CATEGORY_LATIN:
      return gc_character_iter_new_for_script (uc_script ('A'));

    case GC_CATEGORY_EMOTICON:
      {
	static uc_block_t emoticon_blocks[1];
	static gsize emoticon_blocks_size = 0;
	static gsize emoticon_blocks_initialized = 0;
	if (g_once_init_enter (&emoticon_blocks_initialized))
	  {
	    const uc_block_t *block;

	    /* 1F600..1F64F; Emoticons */
	    block = uc_block (0x1F600);
	    if (block)
	      memcpy (&emoticon_blocks[emoticon_blocks_size++], block,
		      sizeof (uc_block_t));
	    g_once_init_leave (&emoticon_blocks_initialized, 1);
	  }
	return gc_character_iter_new_for_blocks (emoticon_blocks,
						 emoticon_blocks_size);
      }
    }

  return gc_character_iter_new ();
}

static gboolean
filter_keywords (ucs4_t uc, gpointer data)
{
  const gchar * const * keywords = data;
  gchar buffer[UNINAME_MAX];

  if (!uc_is_print (uc))
    return FALSE;

  /* Match against the character itself.  */
  if (*keywords)
    {
      uint8_t utf8[6];
      size_t length = G_N_ELEMENTS (utf8);

      u32_to_u8 (&uc, 1, utf8, &length);
      if (length == strlen (*keywords) && memcmp (*keywords, utf8, length) == 0)
	return TRUE;
    }

  /* Match against the name.  */
  if (!unicode_character_name (uc, buffer))
    return FALSE;

  while (*keywords)
    if (g_strstr_len (buffer, UNINAME_MAX, *keywords++) == NULL)
      return FALSE;

  return TRUE;
}

GcCharacterIter *
gc_enumerate_character_by_keywords (const gchar * const * keywords)
{
  GcCharacterIter *iter = gc_character_iter_new ();
  gchar **keywords_upper = g_strdupv ((gchar **) keywords), **p;

  for (p = keywords_upper; *p != NULL; p++)
    {
      gchar *upper = g_ascii_strup (*p, strlen (*p));
      g_free (*p);
      *p = upper;
    }

  iter->blocks = all_blocks;
  iter->block_count = all_block_count;
  iter->filter = filter_keywords;
  iter->copy_data = (GBoxedCopyFunc) g_strdupv;
  iter->free_data = (GBoxedFreeFunc) g_strfreev;
  iter->data = keywords_upper;

  return iter;
}

/**
 * gc_character_name:
 * @uc: a UCS-4 character
 *
 * Returns: (nullable): a newly allocated character name of @uc.
 */
gchar *
gc_character_name (gunichar uc)
{
  gchar *buffer = g_new0 (gchar, UNINAME_MAX);
  return unicode_character_name (uc, buffer);
}

G_DEFINE_BOXED_TYPE (GcSearchResult, gc_search_result,
		     g_array_ref, g_array_unref);

gunichar
gc_search_result_get (GcSearchResult *result, gint index)
{
  g_return_val_if_fail (result, G_MAXUINT32);
  g_return_val_if_fail (0 <= index && index < result->len, G_MAXUINT32);

  return g_array_index (result, gunichar, index);
}

struct SearchCharacterData
{
  gchar **keywords;
  gint max_matches;
};

static void
search_character_data_free (struct SearchCharacterData *data)
{
  g_strfreev (data->keywords);
  g_slice_free (struct SearchCharacterData, data);
}

static void
gc_search_character_thread (GTask         *task,
			    gpointer       source_object,
			    gpointer       task_data,
			    GCancellable  *cancellable)
{
  GcCharacterIter *iter;
  GArray *result;
  struct SearchCharacterData *data = task_data;
  const gchar * const * keywords = (const gchar * const *) data->keywords;

  if (!all_blocks)
    uc_all_blocks (&all_blocks, &all_block_count);

  result = g_array_new (FALSE, FALSE, sizeof (gunichar));
  iter = gc_enumerate_character_by_keywords (keywords);
  while (!g_cancellable_is_cancelled (cancellable)
	 && gc_character_iter_next (iter))
    {
      gunichar uc = gc_character_iter_get (iter);
      if (data->max_matches < 0 || result->len < data->max_matches)
	g_array_append_val (result, uc);
    }

  g_task_return_pointer (task, result, (GDestroyNotify) g_array_unref);
}

/**
 * gc_search_character:
 * @keywords: (array zero-terminated=1) (element-type utf8): an array of keywords
 * @max_matches: the maximum number of results.
 * @cancellable: a #GCancellable.
 * @callback: a #GAsyncReadyCallback.
 * @user_data: a user data passed to @callback.
 */
void
gc_search_character (const gchar * const * keywords,
                     gint                  max_matches,
                     GCancellable         *cancellable,
                     GAsyncReadyCallback   callback,
                     gpointer              user_data)
{
  GTask *task;
  struct SearchCharacterData *data;

  task = g_task_new (NULL, cancellable, callback, user_data);

  data = g_slice_new0 (struct SearchCharacterData);
  data->keywords = g_strdupv ((gchar **) keywords);
  data->max_matches = max_matches;
  g_task_set_task_data (task, data,
			(GDestroyNotify) search_character_data_free);
  g_task_run_in_thread (task, gc_search_character_thread);
}

/**
 * gc_search_character_finish:
 * @result: a #GAsyncResult.
 * @error: return location of an error.
 *
 * Returns: (transfer full): an array of characters.
 */
GcSearchResult *
gc_search_character_finish (GAsyncResult *result,
                            GError      **error)
{
  g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * gc_gtk_clipboard_get:
 *
 * Returns: (transfer none): a #GtkClipboard.
 */
GtkClipboard *
gc_gtk_clipboard_get (void)
{
  return gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
}

void
gc_pango_layout_disable_fallback (PangoLayout *layout)
{
  PangoAttrList *attr_list;

  attr_list = pango_attr_list_new ();
  pango_attr_list_insert (attr_list, pango_attr_fallback_new (FALSE));
  pango_layout_set_attributes (layout, attr_list);
}
