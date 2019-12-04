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

#ifndef __GEGL_OPERATION_BUCKET_FILL_H__
#define __GEGL_OPERATION_BUCKET_FILL_H__

#include <gegl.h>

//typedef enum
//{
  const int GEGL_SELECT_CRITERION_COMPOSITE = 0;//,      /*< desc="Composite"      >*/
  const int GEGL_SELECT_CRITERION_RGB_RED = 1;//,        /*< desc="Red"            >*/
  const int GEGL_SELECT_CRITERION_RGB_GREEN = 2; //,      /*< desc="Green"          >*/
  const int GEGL_SELECT_CRITERION_RGB_BLUE = 3; //,       /*< desc="Blue"           >*/
  const int GEGL_SELECT_CRITERION_HSV_HUE = 4; //,        /*< desc="HSV Hue"        >*/
  const int GEGL_SELECT_CRITERION_HSV_SATURATION=5;//, /*< desc="HSV Saturation" >*/
  const int GEGL_SELECT_CRITERION_HSV_VALUE=6;//,      /*< desc="HSV Value"      >*/
  const int GEGL_SELECT_CRITERION_LCH_LIGHTNESS = 7; //,  /*< desc="LCh Lightness"  >*/
  const int GEGL_SELECT_CRITERION_LCH_CHROMA = 8; //,     /*< desc="LCh Chroma"     >*/
  const int GEGL_SELECT_CRITERION_LCH_HUE = 9; //,        /*< desc="LCh Hue"        >*/
  const int GEGL_SELECT_CRITERION_ALPHA = 10; //,          /*< desc="Alpha"          >*/
//} GeglSelectCriterion;
typedef int GeglSelectCriterion;

/*

GeglBuffer * gegl_pickable_contiguous_region_by_seed                (GeglPickable        *pickable,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     GeglSelectCriterion  select_criterion,
                                                                     gboolean             diagonal_neighbors,
                                                                     gint                 x,
                                                                     gint                 y);
GeglBuffer * gegl_pickable_contiguous_region_by_color               (GeglPickable        *pickable,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     GeglSelectCriterion  select_criterion,
                                                                     const GeglRGB       *color);

GeglBuffer * gegl_pickable_contiguous_region_by_line_art            (GeglPickable        *pickable,
                                                                     GeglLineArt         *line_art,
                                                                     gint                 x,
                                                                     gint                 y);

*/
#if 0
#define GEGL_TYPE_OPERATION_BUCKET_FILL            (gegl_operation_bucket_fill_get_type ())
#define GEGL_OPERATION_BUCKET_FILL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_BUCKET_FILL, GeglOperationBucketFill))
#define GEGL_OPERATION_BUCKET_FILL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_BUCKET_FILL, GeglOperationBucketFillClass))
#define GEGL_IS_OPERATION_FLOOD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_BUCKET_FILL))
#define GEGL_IS_OPERATION_FLOOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_BUCKET_FILL))
#define GEGL_OPERATION_BUCKET_FILL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_BUCKET_FILL, GeglOperationBucketFillClass))


typedef struct _GeglOperationBucketFill      GeglOperationBucketFill;
typedef struct _GeglOperationBucketFillClass GeglOperationBucketFillClass;

struct _GeglOperationBucketFillClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gegl_operation_bucket_fill_get_type (void) G_GNUC_CONST;
#endif

#endif  /*  __GEGL_OPERATION_BUCKET_FILL_H__ */
