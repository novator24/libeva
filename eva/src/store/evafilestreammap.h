#ifndef __EVA_FILE_STREAM_MAP_H_
#define __EVA_FILE_STREAM_MAP_H_

/*
 *
 * EvaFileStreamMap -- implementation of EvaStreamMap that uses the
 * filesystem.
 *
 * Properties:
 *   directory   string   Directory to store files in.
 */

#include "evastreammap.h"

G_BEGIN_DECLS

typedef GObjectClass             EvaFileStreamMapClass;
typedef struct _EvaFileStreamMap EvaFileStreamMap;

GType eva_file_stream_map_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_FILE_STREAM_MAP (eva_file_stream_map_get_type ())
#define EVA_FILE_STREAM_MAP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_FILE_STREAM_MAP, \
			       EvaFileStreamMap))
#define EVA_FILE_STREAM_MAP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
			    EVA_TYPE_FILE_STREAM_MAP, \
			    EvaFileStreamMapClass))
#define EVA_FILE_STREAM_MAP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
			      EVA_TYPE_FILE_STREAM_MAP, \
			      EvaFileStreamMapClass))
#define EVA_IS_FILE_STREAM_MAP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_FILE_STREAM_MAP))
#define EVA_IS_FILE_STREAM_MAP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_FILE_STREAM_MAP))

struct _EvaFileStreamMap
{
  GObject object;

  char *directory;
};

EvaFileStreamMap * eva_file_stream_map_new (const char *directory);

G_END_DECLS

#endif
