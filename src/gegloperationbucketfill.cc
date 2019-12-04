/* GEGL - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef GEGL_PROPERTIES
  property_boolean(antialias, _("Antialias"), FALSE)
  property_double(threshold, _("Threshold"), 0)
  property_boolean(select_transparent, _("Select Transparent"), TRUE)
  property_int(select_criterion, _("Select Criterion"), 0)
  property_boolean(diagonal_neighbors, _("Diagonal neighbors"), FALSE)
  property_double(x, _("X"), 0)
  property_double(y, _("Y"), 0)
#else

extern "C"
{

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#define _(String) gettext(String)
#define N_(String) gettext_noop(String)
#define gettext_noop(String) (String)

#include "gegloperationbucketfill.h"

//#include <cairo.h>
//#include <gdk-pixbuf/gdk-pixbuf.h>

#include <math.h>

#define GEGL_OP_FILTER
#define GEGL_OP_NAME gegloperationbucketfill
#define GEGL_OP_C_SOURCE gegloperationbucketfill.cc

#include "gegl-op.h"

#if 0
#include <gegl.h>
#include <gegl-types.h>
#include <operation/gegl-operation.h>
#include <operation/gegl-operation-filter.h>

#endif
#define MAX_CHANNELS 4
#define EPSILON 1e-6

#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)
#if 0
enum
{
  PROP_0,
  PROP_ANTIALIAS,
  PROP_THRESHOLD,
  PROP_SELECT_TRANSPARENT,
  PROP_SELECT_CRITERION,
  PROP_DIAGONAL_NEIGHBORS,
  PROP_X,
  PROP_Y
};
struct _GeglOperationBucketFill
{
  GeglOperationFilter  parent_instance;
  gboolean antialias;
  gfloat threshold;
  GeglSelectCriterion select_criterion;
  gboolean select_transparent;
  gboolean diagonal_neighbors;
  gint x;
  gint y;
};
#endif

typedef struct
{
  gint   x;
  gint   y;
  gint   level;
} BorderPixel;




/*  local function prototypes  */

//static void gegl_operation_bucket_fill_class_init (GeglOpClass *klass);


static const Babl * choose_format         (GeglBuffer          *buffer,
                                           GeglSelectCriterion  select_criterion,
                                           gint                *n_components,
                                           gboolean            *has_alpha);
static gfloat   pixel_difference          (const gfloat        *col1,
                                           const gfloat        *col2,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GeglSelectCriterion  select_criterion);
static void     push_segment              (GQueue              *segment_queue,
                                           gint                 y,
                                           gint                 old_y,
                                           gint                 start,
                                           gint                 end,
                                           gint                 new_y,
                                           gint                 new_start,
                                           gint                 new_end);
static void     pop_segment               (GQueue              *segment_queue,
                                           gint                *y,
                                           gint                *old_y,
                                           gint                *start,
                                           gint                *end);
static gboolean find_contiguous_segment   (const gfloat        *col,
                                           GeglBuffer          *src_buffer,
                                           GeglSampler         *src_sampler,
                                           const GeglRectangle *src_extent,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *src_format,
                                           const Babl          *mask_format,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GeglSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gint                 initial_x,
                                           gint                 initial_y,
                                           gint                *start,
                                           gint                *end,
                                           gfloat              *row);
static void     find_contiguous_region    (GeglBuffer          *src_buffer,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *format,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GeglSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gboolean             diagonal_neighbors,
                                           gint                 x,
                                           gint                 y,
                                           const gfloat        *col);
/*
static void            line_art_queue_pixel (GQueue              *queue,
                                             gint                 x,
                                             gint                 y,
                                             gint                 level);
*/

/*
static void
gegl_op_init (GeglOp *self)
{
}
*/

static void
prepare (GeglOperation *operation)
{
    g_warning("BucketFill::prepare");

  const Babl *format = gegl_operation_get_source_format(operation, "input"); //gegl_operation_get_source_space (operation, "input");
  const Babl *space = babl_format_get_space (format);
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("Y float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("Y float", space));
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                                              const gchar         *input_pad,
                                              const GeglRectangle *roi)
{
    g_warning("BucketFill::get_required_for_output");
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
get_cached_region (GeglOperation       *self,
                                        const GeglRectangle *roi)
{
    g_warning("BucketFill::get_cached_region");
  return *gegl_operation_source_get_bounding_box (self, "input");
}
#if 0
static void
gegl_operation_bucket_fill_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GeglProperties *self = GEGL_PROPERTIES (object);
  switch (property_id)
    {
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, self->antialias);
      break;

    case PROP_THRESHOLD:
      g_value_set_float (value, self->threshold);
      break;

    case PROP_SELECT_TRANSPARENT:
      g_value_set_boolean (value, self->select_transparent);
      break;

    case PROP_SELECT_CRITERION:
      g_value_set_int (value, self->select_criterion);
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      g_value_set_boolean (value, self->diagonal_neighbors);
      break;

    case PROP_X:
      g_value_set_int (value, self->x);
      break;

    case PROP_Y:
      g_value_set_int (value, self->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gegl_operation_bucket_fill_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GeglOperationBucketFill *self = GEGL_OPERATION_BUCKET_FILL (object);

  switch (property_id)
    {
    case PROP_ANTIALIAS:
      self->antialias = g_value_get_boolean (value);
      g_object_notify (object, "antialias");
      break;

    case PROP_THRESHOLD:
      self->threshold = g_value_get_float (value);
      g_object_notify (object, "threshold");
      break;

    case PROP_SELECT_TRANSPARENT:
      self->select_transparent = g_value_get_boolean (value);
      g_object_notify (object, "select-transparent");
      break;

    case PROP_SELECT_CRITERION:
      self->select_criterion = (GeglSelectCriterion)g_value_get_int (value);
      g_object_notify (object, "select-criterion");
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      self->diagonal_neighbors = g_value_get_boolean (value);
      g_object_notify (object, "diagonal-neighbors");
      break;

    case PROP_X:
      self->x = g_value_get_int (value);
      g_object_notify (object, "x");
      break;

    case PROP_Y:
      self->y = g_value_get_int (value);
      g_object_notify (object, "y");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
} 
#endif
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *src_buffer, // input
         GeglBuffer          *mask_buffer, // output
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *self = GEGL_PROPERTIES (operation);
  gboolean antialias = self->antialias;
  gfloat threshold = self->threshold;
  GeglSelectCriterion select_criterion = self->select_criterion;
  gboolean select_transparent = self->select_transparent;
  gboolean diagonal_neighbors = self->diagonal_neighbors;
  gint x = int(self->x);
  gint y = int(self->y);

  g_warning("BucketFill::process");

  const Babl    *format;
  GeglRectangle  extent;
  gint           n_components = 0;
  gboolean       has_alpha;
  gfloat         start_col[MAX_CHANNELS];

  g_warning("BucketFill::choose_format");
  format = choose_format (src_buffer, select_criterion,
                          &n_components, &has_alpha);
  g_warning("BucketFill::gegl_buffer_sample");
  gegl_buffer_sample (src_buffer, x, y, NULL, start_col, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparent regions if the start pixel isn't
           *  fully transparent
           */
          if (start_col[n_components - 1] > 0)
            select_transparent = FALSE;
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  g_warning("BucketFill::gegl_buffer_get_extent");
  extent = *gegl_buffer_get_extent (src_buffer);

  if (x >= extent.x && x < (extent.x + extent.width) &&
      y >= extent.y && y < (extent.y + extent.height))
    {
//      GEGL_TIMER_START();

      g_warning("BucketFill::find_contiguous_region");
      find_contiguous_region (src_buffer, mask_buffer,
                              format, n_components, has_alpha,
                              select_transparent, select_criterion,
                              antialias, threshold, diagonal_neighbors,
                              x, y, start_col);

//      GEGL_TIMER_END("foo");
    }

  return TRUE;
}
#if 0
GeglBuffer *
gegl_pickable_contiguous_region_by_color (GeglPickable        *pickable,
                                          gboolean             antialias,
                                          gfloat               threshold,
                                          gboolean             select_transparent,
                                          GeglSelectCriterion  select_criterion,
                                          const GeglRGB       *color)
{
  /*  Scan over the pickable's active layer, finding pixels within the
   *  specified threshold from the given R, G, & B values.  If
   *  antialiasing is on, use the same antialiasing scheme as in
   *  fuzzy_select.  Modify the pickable's mask to reflect the
   *  additional selection
   */
  GeglBuffer *src_buffer;
  GeglBuffer *mask_buffer;
  const Babl *format;
  gint        n_components;
  gboolean    has_alpha;
  gfloat      start_col[MAX_CHANNELS];

  g_return_val_if_fail (GEGL_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  /* increase the threshold by EPSILON, to allow for conversion errors,
   * especially when threshold == 0 (see issue #1554.)  we need to do this
   * here, but not in the other functions, since the input color gets converted
   * to the format in which we perform the comparison through a different path
   * than the pickable's pixels, which can introduce error.
   */
  threshold += EPSILON;

  gegl_pickable_flush (pickable);

  src_buffer = gegl_pickable_get_buffer (pickable);

  format = choose_format (src_buffer, select_criterion,
                          &n_components, &has_alpha);

  gegl_rgba_get_pixel (color, format, start_col);

  if (has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparency if "color" isn't fully transparent
           */
          if (start_col[n_components - 1] > 0.0)
            select_transparent = FALSE;
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  mask_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                 babl_format ("Y float"));

  gegl_parallel_distribute_area (
    gegl_buffer_get_extent (src_buffer), PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (src_buffer,
                                       area, 0, format,
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, mask_buffer,
                                area, 0, babl_format ("Y float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *src   = (const gfloat *) iter->items[0].data;
          gfloat       *dest  = (      gfloat *) iter->items[1].data;
          gint          count = iter->length;

          while (count--)
            {
              /*  Find how closely the colors match  */
              *dest = pixel_difference (start_col, src,
                                        antialias,
                                        threshold,
                                        n_components,
                                        has_alpha,
                                        select_transparent,
                                        select_criterion);

              src  += n_components;
              dest += 1;
            }
        }
    });

  return mask_buffer;
}
#endif

#if 0
GeglBuffer *
gegl_pickable_contiguous_region_by_line_art (GeglPickable *pickable,
                                             GeglLineArt  *line_art,
                                             gint          x,
                                             gint          y)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *mask_buffer;
  const Babl    *format  = babl_format ("Y float");
  gfloat        *distmap = NULL;
  GeglRectangle  extent;
  gboolean       free_line_art = FALSE;
  gboolean       filled        = FALSE;
  guchar         start_col;

  g_return_val_if_fail (GEGL_IS_PICKABLE (pickable) || GEGL_IS_LINE_ART (line_art), NULL);

  if (! line_art)
    {
      /* It is much better experience to pre-compute the line art,
       * but it may not be always possible (for instance when
       * selecting/filling through a PDB call).
       */
      line_art = gegl_line_art_new ();
      gegl_line_art_set_input (line_art, pickable);
      free_line_art = TRUE;
    }

  src_buffer = gegl_line_art_get (line_art, &distmap);
  g_return_val_if_fail (src_buffer && distmap, NULL);

  gegl_buffer_sample (src_buffer, x, y, NULL, &start_col, NULL,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  extent = *gegl_buffer_get_extent (src_buffer);

  mask_buffer = gegl_buffer_new (&extent, format);

  if (start_col)
    {
      if (start_col == 1)
        {
          /* As a special exception, if you fill over a line art pixel, only
           * fill the pixel and exit
           */
          gfloat col = 1.0;
          GeglRectangle rect = {x, y, 1, 1};
          gegl_buffer_set (mask_buffer, &rect,
                           0, format, &col, GEGL_AUTO_ROWSTRIDE);
        }
      else /* start_col == 2 */
        {
          /* If you fill over a closure pixel, let's fill on all sides
           * of the start point. Otherwise we get a very weird result
           * with only a single pixel filled in the middle of an empty
           * region (since closure pixels are invisible by nature).
           */
          gfloat col = 0.0;

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y - 1, &col);

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y >= extent.y && y < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y, &col);

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y + 1, &col);

          if (x >= extent.x && x < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x, y - 1, &col);

          if (x >= extent.x && x < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x, y + 1, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y - 1, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y >= extent.y && y < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y + 1, &col);

          filled = TRUE;
        }
    }
  else if (x >= extent.x && x < (extent.x + extent.width) &&
           y >= extent.y && y < (extent.y + extent.height))
    {
      gfloat col = 0.0;

      find_contiguous_region (src_buffer, mask_buffer,
                              format, 1, FALSE,
                              FALSE, GEGL_SELECT_CRITERION_COMPOSITE,
                              FALSE, 0.0, FALSE,
                              x, y, &col);
      filled = TRUE;
    }

  if (filled)
    {
      GQueue *queue  = g_queue_new ();
      gfloat *mask;
      gint    width  = gegl_buffer_get_width (src_buffer);
      gint    height = gegl_buffer_get_height (src_buffer);
      gint    line_art_max_grow;
      gint    nx, ny;

      GEGL_TIMER_START();
      /* The last step of the line art algorithm is to make sure that
       * selections does not leave "holes" between its borders and the
       * line arts, while not stepping over as well.
       * I used to run the "gegl:watershed-transform" operation to flood
       * the stroke pixels, but for such simple need, this simple code
       * is so much faster while producing better results.
       */
      mask = g_new (gfloat, width * height);
      gegl_buffer_get (mask_buffer, NULL, 1.0, NULL,
                       mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            if (distmap[x + y * width] == 1.0)
              {
                if (x > 0)
                  {
                    nx = x - 1;
                    if (y > 0)
                      {
                        ny = y - 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                    ny = y;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                    if (y < height - 1)
                      {
                        ny = y + 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                  }
                if (x < width - 1)
                  {
                    nx = x + 1;
                    if (y > 0)
                      {
                        ny = y - 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                    ny = y;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                    if (y < height - 1)
                      {
                        ny = y + 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                  }
                nx = x;
                if (y > 0)
                  {
                    ny = y - 1;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                  }
                if (y < height - 1)
                  {
                    ny = y + 1;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                  }
              }
          }

      g_object_get (line_art,
                    "max-grow", &line_art_max_grow,
                    NULL);
      while (! g_queue_is_empty (queue))
        {
          BorderPixel *c = (BorderPixel *) g_queue_pop_head (queue);

          if (mask[c->x + c->y * width] != 1.0)
            {
              mask[c->x + c->y * width] = 1.0;
              if (c->level >= line_art_max_grow)
                /* Do not overflood under line arts. */
                continue;
              if (c->x > 0)
                {
                  nx = c->x - 1;
                  if (c->y > 0)
                    {
                      ny = c->y - 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                  ny = c->y;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                  if (c->y < height - 1)
                    {
                      ny = c->y + 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                }
              if (c->x < width - 1)
                {
                  nx = c->x + 1;
                  if (c->y > 0)
                    {
                      ny = c->y - 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                  ny = c->y;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                  if (c->y < height - 1)
                    {
                      ny = c->y + 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                }
              nx = c->x;
              if (c->y > 0)
                {
                  ny = c->y - 1;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                }
              if (c->y < height - 1)
                {
                  ny = c->y + 1;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                }
            }
          g_free (c);
        }
      g_queue_free (queue);
      gegl_buffer_set (mask_buffer, gegl_buffer_get_extent (mask_buffer),
                       0, NULL, mask, GEGL_AUTO_ROWSTRIDE);
      g_free (mask);

      GEGL_TIMER_END("watershed line art");
    }
  if (free_line_art)
    g_clear_object (&line_art);

  return mask_buffer;
}
#endif

/*  private functions  */

typedef enum {
  GEGL_INVALID = -1,
	GEGL_RGB = 0,
	GEGL_GRAY,
	GEGL_INDEXED
} GeglImageBaseType;

static GeglImageBaseType
gegl_babl_format_get_base_type (const Babl *format)
{
  const gchar *name;

  g_return_val_if_fail (format != NULL, GEGL_INVALID);

  name = babl_get_name (babl_format_get_model (format));

  if (! strcmp (name, "Y")   ||
      ! strcmp (name, "Y'")  ||
      ! strcmp (name, "Y~")  ||
      ! strcmp (name, "YA")  ||
      ! strcmp (name, "Y'A") ||
      ! strcmp (name, "Y~A"))
    {
      return GEGL_GRAY;
    }
  else if (! strcmp (name, "RGB")        ||
           ! strcmp (name, "R'G'B'")     ||
           ! strcmp (name, "R~G~B~")     ||
           ! strcmp (name, "RGBA")       ||
           ! strcmp (name, "R'G'B'A")    ||
           ! strcmp (name, "R~G~B~A")    ||
           ! strcmp (name, "RaGaBaA")    ||
           ! strcmp (name, "R'aG'aB'aA") ||
           ! strcmp (name, "R~aG~aB~aA"))
    {
      return GEGL_RGB;
    }
  else if (babl_format_is_palette (format))
    {
      return GEGL_INDEXED;
    }

  g_return_val_if_reached (GEGL_INVALID);
}

static const Babl *
gegl_babl_format (GeglImageBaseType  base_type,
                  gboolean           with_alpha,
                  const Babl        *space)
{
  switch (base_type)
    {
    case GEGL_RGB:
      if (with_alpha)
        return babl_format_with_space ("R'G'B'A float", space);
      else
        return babl_format_with_space ("R'G'B' float", space);
      break;

    case GEGL_GRAY:
      if (with_alpha)
        return babl_format_with_space ("Y'A float", space);
      else
        return babl_format_with_space ("Y' float", space);
     break;

    case GEGL_INDEXED:
    case GEGL_INVALID:
      /* need to use the image's api for this */
      break;
    }

  g_return_val_if_reached (NULL);
}

static const Babl *
choose_format (GeglBuffer          *buffer,
               GeglSelectCriterion  select_criterion,
               gint                *n_components,
               gboolean            *has_alpha)
{
  const Babl *format = gegl_buffer_get_format (buffer);

  *has_alpha = babl_format_has_alpha (format);

  switch (select_criterion)
    {
    case GEGL_SELECT_CRITERION_COMPOSITE:
      if (babl_format_is_palette (format))
        format = babl_format ("R'G'B'A float");
      else
        format = gegl_babl_format (gegl_babl_format_get_base_type (format),
                                   *has_alpha,
                                   NULL);
      break;

    case GEGL_SELECT_CRITERION_RGB_RED:
    case GEGL_SELECT_CRITERION_RGB_GREEN:
    case GEGL_SELECT_CRITERION_RGB_BLUE:
    case GEGL_SELECT_CRITERION_ALPHA:
      format = babl_format ("R'G'B'A float");
      break;

    case GEGL_SELECT_CRITERION_HSV_HUE:
    case GEGL_SELECT_CRITERION_HSV_SATURATION:
    case GEGL_SELECT_CRITERION_HSV_VALUE:
      format = babl_format ("HSVA float");
      break;

    case GEGL_SELECT_CRITERION_LCH_LIGHTNESS:
      format = babl_format ("CIE L alpha float");
      break;

    case GEGL_SELECT_CRITERION_LCH_CHROMA:
    case GEGL_SELECT_CRITERION_LCH_HUE:
      format = babl_format ("CIE LCH(ab) alpha float");
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }

  *n_components = babl_format_get_n_components (format);

  return format;
}

static gfloat
pixel_difference (const gfloat        *col1,
                  const gfloat        *col2,
                  gboolean             antialias,
                  gfloat               threshold,
                  gint                 n_components,
                  gboolean             has_alpha,
                  gboolean             select_transparent,
                  GeglSelectCriterion  select_criterion)
{
  gfloat max = 0.0;

  /*  if there is an alpha channel, never select transparent regions  */
  if (! select_transparent && has_alpha && col2[n_components - 1] == 0.0)
    return 0.0;

  if (select_transparent && has_alpha)
    {
      max = fabs (col1[n_components - 1] - col2[n_components - 1]);
    }
  else
    {
      gfloat diff;
      gint   b;

      if (has_alpha)
        n_components--;

      switch (select_criterion)
        {
        case GEGL_SELECT_CRITERION_COMPOSITE:
          for (b = 0; b < n_components; b++)
            {
              diff = fabs (col1[b] - col2[b]);
              if (diff > max)
                max = diff;
            }
          break;

        case GEGL_SELECT_CRITERION_RGB_RED:
          max = fabs (col1[0] - col2[0]);
          break;

        case GEGL_SELECT_CRITERION_RGB_GREEN:
          max = fabs (col1[1] - col2[1]);
          break;

        case GEGL_SELECT_CRITERION_RGB_BLUE:
          max = fabs (col1[2] - col2[2]);
          break;

        case GEGL_SELECT_CRITERION_ALPHA:
          max = fabs (col1[3] - col2[3]);
          break;

        case GEGL_SELECT_CRITERION_HSV_HUE:
          if (col1[1] > EPSILON)
            {
              if (col2[1] > EPSILON)
                {
                  max = fabs (col1[0] - col2[0]);
                  max = MIN (max, 1.0 - max);
                }
              else
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
            }
          else
            {
              if (col2[1] > EPSILON)
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
              else
                {
                  max = 0.0;
                }
            }
          break;

        case GEGL_SELECT_CRITERION_HSV_SATURATION:
          max = fabs (col1[1] - col2[1]);
          break;

        case GEGL_SELECT_CRITERION_HSV_VALUE:
          max = fabs (col1[2] - col2[2]);
          break;

        case GEGL_SELECT_CRITERION_LCH_LIGHTNESS:
          max = fabs (col1[0] - col2[0]) / 100.0;
          break;

        case GEGL_SELECT_CRITERION_LCH_CHROMA:
          max = fabs (col1[1] - col2[1]) / 100.0;
          break;

        case GEGL_SELECT_CRITERION_LCH_HUE:
          if (col1[1] > 100.0 * EPSILON)
            {
              if (col2[1] > 100.0 * EPSILON)
                {
                  max = fabs (col1[2] - col2[2]) / 360.0;
                  max = MIN (max, 1.0 - max);
                }
              else
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
            }
          else
            {
              if (col2[1] > 100.0 * EPSILON)
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
              else
                {
                  max = 0.0;
                }
            }
          break;
        }
    }

  if (antialias && threshold > 0.0)
    {
      gfloat aa = 1.5 - (max / threshold);

      if (aa <= 0.0)
        return 0.0;
      else if (aa < 0.5)
        return aa * 2.0;
      else
        return 1.0;
    }
  else
    {
      if (max > threshold)
        return 0.0;
      else
        return 1.0;
    }
}

static void
push_segment (GQueue *segment_queue,
              gint    y,
              gint    old_y,
              gint    start,
              gint    end,
              gint    new_y,
              gint    new_start,
              gint    new_end)
{
  /* To avoid excessive memory allocation (y, old_y, start, end) tuples are
   * stored in interleaved format:
   *
   * [y1] [old_y1] [start1] [end1] [y2] [old_y2] [start2] [end2]
   */

  if (new_y != old_y)
    {
      /* If the new segment's y-coordinate is different than the old (source)
       * segment's y-coordinate, push the entire segment.
       */
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
    }
  else
    {
      /* Otherwise, only push the set-difference between the new segment and
       * the source segment (since we've already scanned the source segment.)
       * Note that the `+ 1` and `- 1` terms of the end/start coordinates below
       * are only necessary when `diagonal_neighbors` is on (and otherwise make
       * the segments slightly larger than necessary), but, meh...
       */
      if (new_start < start)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (start + 1));
        }

      if (new_end > end)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (end - 1));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
        }
    }
}

static void
pop_segment (GQueue *segment_queue,
             gint   *y,
             gint   *old_y,
             gint   *start,
             gint   *end)
{
  *y     = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *old_y = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *start = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *end   = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
}

/* #define FETCH_ROW 1 */

static gboolean
find_contiguous_segment (const gfloat        *col,
                         GeglBuffer          *src_buffer,
                         GeglSampler         *src_sampler,
                         const GeglRectangle *src_extent,
                         GeglBuffer          *mask_buffer,
                         const Babl          *src_format,
                         const Babl          *mask_format,
                         gint                 n_components,
                         gboolean             has_alpha,
                         gboolean             select_transparent,
                         GeglSelectCriterion  select_criterion,
                         gboolean             antialias,
                         gfloat               threshold,
                         gint                 initial_x,
                         gint                 initial_y,
                         gint                *start,
                         gint                *end,
                         gfloat              *row)
{
  gfloat *s;
  gfloat  mask_row_buf[src_extent->width];
  gfloat *mask_row = mask_row_buf - src_extent->x;
  gfloat  diff;

#ifdef FETCH_ROW
  GeglRectangle rect = {0, initial_y, width, 1};
  gegl_buffer_get (src_buffer, &rect, 1.0,
                   src_format,
                   row, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  s = row + initial_x * n_components;
#else
  s = (gfloat *) g_alloca (n_components * sizeof (gfloat));

  gegl_sampler_get (src_sampler,
                    initial_x, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

  diff = pixel_difference (col, s, antialias, threshold,
                           n_components, has_alpha, select_transparent,
                           select_criterion);

  /* check the starting pixel */
  if (! diff)
    return FALSE;

  mask_row[initial_x] = diff;

  *start = initial_x - 1;
#ifdef FETCH_ROW
  s = row + *start * n_components;
#endif

  while (*start >= src_extent->x)
    {
#ifndef FETCH_ROW
      gegl_sampler_get (src_sampler,
                        *start, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);
      if (diff == 0.0)
        break;

      mask_row[*start] = diff;

      (*start)--;
#ifdef FETCH_ROW
      s -= n_components;
#endif
    }

  *end = initial_x + 1;
#ifdef FETCH_ROW
  s = row + *end * n_components;
#endif

  while (*end < src_extent->x + src_extent->width)
    {
#ifndef FETCH_ROW
      gegl_sampler_get (src_sampler,
                        *end, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);
      if (diff == 0.0)
        break;

      mask_row[*end] = diff;

      (*end)++;
#ifdef FETCH_ROW
      s += n_components;
#endif
    }

  GeglRectangle rect = {*start + 1, initial_y, *end - *start - 1, 1};
  gegl_buffer_set (mask_buffer, &rect,
                   0, mask_format, &mask_row[*start + 1],
                   GEGL_AUTO_ROWSTRIDE);

  return TRUE;
}

static void
find_contiguous_region (GeglBuffer          *src_buffer,
                        GeglBuffer          *mask_buffer,
                        const Babl          *format,
                        gint                 n_components,
                        gboolean             has_alpha,
                        gboolean             select_transparent,
                        GeglSelectCriterion  select_criterion,
                        gboolean             antialias,
                        gfloat               threshold,
                        gboolean             diagonal_neighbors,
                        gint                 x,
                        gint                 y,
                        const gfloat        *col)
{
  const Babl          *mask_format = babl_format ("Y float");
  GeglSampler         *src_sampler;
  const GeglRectangle *src_extent;
  gint                 old_y;
  gint                 start, end;
  gint                 new_start, new_end;
  GQueue              *segment_queue;
  gfloat              *row = NULL;

  src_extent = gegl_buffer_get_extent (src_buffer);

#ifdef FETCH_ROW
  row = g_new (gfloat, src_extent->width * n_components);
#endif

  src_sampler = gegl_buffer_sampler_new (src_buffer,
                                         format, GEGL_SAMPLER_NEAREST);

  segment_queue = g_queue_new ();

  push_segment (segment_queue,
                y, /* dummy values: */ -1, 0, 0,
                y, x - 1, x + 1);

  do
    {
      pop_segment (segment_queue,
                   &y, &old_y, &start, &end);

      for (x = start + 1; x < end; x++)
        {
          gfloat val;

          GeglRectangle rect = {x, y, 1, 1};
          gegl_buffer_get (mask_buffer, &rect, 1.0,
                           mask_format, &val, GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);

          if (val != 0.0)
            {
              /* If the current pixel is selected, then we've already visited
               * the next pixel.  (Note that we assume that the maximal image
               * width is sufficiently low that `x` won't overflow.)
               */
              x++;
              continue;
            }

          if (! find_contiguous_segment (col,
                                         src_buffer, src_sampler, src_extent,
                                         mask_buffer,
                                         format, mask_format,
                                         n_components,
                                         has_alpha,
                                         select_transparent, select_criterion,
                                         antialias, threshold, x, y,
                                         &new_start, &new_end,
                                         row))
            continue;

          /* We can skip directly to `new_end + 1` on the next iteration, since
           * we've just selected all pixels in the range `[x, new_end)`, and
           * the pixel at `new_end` is above threshold.  (Note that we assume
           * that the maximal image width is sufficiently low that `x` won't
           * overflow.)
           */
          x = new_end;

          if (diagonal_neighbors)
            {
              if (new_start >= src_extent->x)
                new_start--;

              if (new_end < src_extent->x + src_extent->width)
                new_end++;
            }

          if (y + 1 < src_extent->y + src_extent->height)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y + 1, new_start, new_end);
            }

          if (y - 1 >= src_extent->y)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y - 1, new_start, new_end);
            }

        }
    }
  while (! g_queue_is_empty (segment_queue));

  g_queue_free (segment_queue);

  g_object_unref (src_sampler);

#ifdef FETCH_ROW
  g_free (row);
#endif
}
#if 0
static void
line_art_queue_pixel (GQueue *queue,
                      gint    x,
                      gint    y,
                      gint    level)
{
  BorderPixel *p = g_new (BorderPixel, 1);

  p->x      = x;
  p->y      = y;
  p->level  = level;

  g_queue_push_head (queue, p);
}
#endif

/*  public functions  */
#if 0
G_DEFINE_TYPE (GeglOperationBucketFill, gegl_operation_bucket_fill,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gegl_operation_bucket_fill_parent_class
#endif

/* GEGL graph for the test case. */
static const gchar* reference_xml = "<?xml version='1.0' encoding='UTF-8'?>"
"<gegl>"
"<node operation='gegl:bucket-fill'>"
"  <params>"
"    <param name='antialias'>false</param>"
"    <param name='threshold'>0.5</param>"
"    <param name='select-transparent'>false</param>"
"    <param name='select-'>false</param>"
"    <param name='antialias'>false</param>"
"  </params>"
"</node>"
"<node operation='gegl:load'>"
"  <params>"
"    <param name='path'>flood-input.png</param>"
"  </params>"
"</node>"
"</gegl>";


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
//  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  g_warning("Registering GeglOperationBucketFill\n");

  /* The input and output buffers must be different, since we generally need to
   * be able to access the input-image values after having written to the
   * output buffer.
   */
  operation_class->want_in_place = FALSE;
  /* We don't want `GeglOperationFilter` to split the image across multiple
   * threads, since this operation depends on, and affects, the image as a
   * whole.
   */
  operation_class->threaded      = FALSE;
  /* Note that both of these options are the default; we set them here for
   * explicitness.
   */

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gegl:bucket-fill",
                                 "categories",  "fill",
                                 "description", "GEGL Bucket-fill operation",
                                 "reference", "",
                                 "reference-image", "flood-output.png",
                                 "reference-composition", reference_xml,
                                 NULL);
#if 0
  object_class->set_property   = gegl_operation_bucket_fill_set_property;
  object_class->get_property   = gegl_operation_bucket_fill_get_property;

  g_object_class_install_property (object_class, PROP_ANTIALIAS,\
                                   g_param_spec_boolean ("antialias", _("Antialias"), _("Antialias"),\
                                   FALSE,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));

  g_object_class_install_property (object_class, PROP_THRESHOLD,\
                                   g_param_spec_float ("threshold", _("Threshold"), _("Threshold"),\
                                   0, 1, 0,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));
  
  g_object_class_install_property (object_class, PROP_SELECT_TRANSPARENT,\
                                   g_param_spec_boolean ("select-transparent", _("Select Transparent"), _("Select Transparent"),\
                                   TRUE,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));

  g_object_class_install_property (object_class, PROP_SELECT_CRITERION,\
                                   g_param_spec_int ("select-criterion", _("Select Criterion"), _("Select Criterion"),\
                                   0, 11, 0,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));

  g_object_class_install_property (object_class, PROP_DIAGONAL_NEIGHBORS,\
                                   g_param_spec_boolean ("diagonal-neighbors", _("Diagonal Neighbors"), _("Diagonal Neighbors"),\
                                   FALSE,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));

  g_object_class_install_property (object_class, PROP_X,\
                                   g_param_spec_int ("x", _("X"), _("X"),\
                                   0, ~((int)0), 0,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));

  g_object_class_install_property (object_class, PROP_Y,\
                                   g_param_spec_int ("y", _("Y"), _("Y"),\
                                   0, ~((int)0), 0,\
                                   (GParamFlags)(G_PARAM_READWRITE|G_PARAM_CONSTRUCT)));
#endif
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;

  filter_class->process                    = process;
}

} /* extern "C" */
#endif /* #ifndef GEGL_PROPERTIES */