#ifndef __EVA_XMLRPC_H_
#define __EVA_XMLRPC_H_

#include <glib.h>

G_BEGIN_DECLS
/* --- typedefs --- */
typedef struct _EvaXmlrpcStream EvaXmlrpcStream;
typedef struct _EvaXmlrpcStreamClass EvaXmlrpcStreamClass;
typedef struct _EvaXmlrpcOutgoing EvaXmlrpcOutgoing;
typedef struct _EvaXmlrpcIncoming EvaXmlrpcIncoming;

typedef struct _EvaXmlrpcArray EvaXmlrpcArray;
typedef struct _EvaXmlrpcStruct EvaXmlrpcStruct;
typedef struct _EvaXmlrpcValue EvaXmlrpcValue;
typedef struct _EvaXmlrpcNamedValue EvaXmlrpcNamedValue;
typedef struct _EvaXmlrpcRequest EvaXmlrpcRequest;
typedef struct _EvaXmlrpcResponse EvaXmlrpcResponse;
typedef struct _EvaXmlrpcParser EvaXmlrpcParser;

typedef enum
{
  EVA_XMLRPC_INT32,
  EVA_XMLRPC_BOOLEAN,
  EVA_XMLRPC_DOUBLE,
  EVA_XMLRPC_STRING,
  EVA_XMLRPC_DATE,
  EVA_XMLRPC_BINARY_DATA,
  EVA_XMLRPC_STRUCT,
  EVA_XMLRPC_ARRAY
} EvaXmlrpcType;

struct _EvaXmlrpcArray
{
  unsigned len;
  EvaXmlrpcValue *values;

  /*< private >*/
  unsigned alloced;
};

struct _EvaXmlrpcStruct
{
  unsigned n_members;
  EvaXmlrpcNamedValue *members;

  /*< private >*/
  unsigned alloced;
};

struct _EvaXmlrpcValue
{
  EvaXmlrpcType type;
  union
  {
    int v_int32;
    gboolean v_boolean;
    double v_double;
    char *v_string;
    gulong v_date;
    GByteArray *v_binary_data;
    EvaXmlrpcStruct *v_struct;
    EvaXmlrpcArray *v_array;
  } data;
};

struct _EvaXmlrpcNamedValue
{
  char *name;
  EvaXmlrpcValue value;
};

struct _EvaXmlrpcRequest
{
  unsigned magic;		/* private, must be first */
  char *method_name;
  EvaXmlrpcArray *params;
  EvaXmlrpcStream *xmlrpc_stream;
  /*< private >*/
  unsigned ref_count;
};

struct _EvaXmlrpcResponse
{
  unsigned magic;		/* private, must be first */

  EvaXmlrpcArray *params;
  gboolean has_fault;
  EvaXmlrpcValue fault;

  /*< private >*/
  unsigned ref_count;
};


EvaXmlrpcStruct *eva_xmlrpc_struct_new         (void);
void             eva_xmlrpc_struct_free        (EvaXmlrpcStruct *structure);
void             eva_xmlrpc_struct_add_int32   (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gint32           value);
void             eva_xmlrpc_struct_add_boolean(EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gboolean         value);
void             eva_xmlrpc_struct_add_double  (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gdouble          value);
void             eva_xmlrpc_struct_add_string  (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                const char      *value);
void             eva_xmlrpc_struct_add_date    (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gulong           value);

/* these take ownership of second argument */  
void             eva_xmlrpc_struct_add_data    (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                GByteArray      *data);
void             eva_xmlrpc_struct_add_struct  (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                EvaXmlrpcStruct *substructure);
void             eva_xmlrpc_struct_add_array   (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                EvaXmlrpcArray  *array);

/* lookups */
gboolean         eva_xmlrpc_struct_peek_int32  (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gint32          *out);
gboolean         eva_xmlrpc_struct_peek_boolean(EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                gboolean        *out);
gboolean         eva_xmlrpc_struct_peek_double (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
                                                double          *out);
const char *     eva_xmlrpc_struct_peek_string (EvaXmlrpcStruct *structure,
                                                const char      *member_name);
gboolean         eva_xmlrpc_struct_peek_date   (EvaXmlrpcStruct *structure,
                                                const char      *member_name,
						gulong          *out);
const GByteArray*eva_xmlrpc_struct_peek_data   (EvaXmlrpcStruct *structure,
                                                const char      *member_name);
EvaXmlrpcStruct *eva_xmlrpc_struct_peek_struct (EvaXmlrpcStruct *structure,
                                                const char      *member_name);
EvaXmlrpcArray  *eva_xmlrpc_struct_peek_array  (EvaXmlrpcStruct *structure,
                                                const char      *member_name);


EvaXmlrpcArray  *eva_xmlrpc_array_new          (void);
void             eva_xmlrpc_array_free         (EvaXmlrpcArray  *array);
void             eva_xmlrpc_array_add_int32    (EvaXmlrpcArray  *array,
                                                gint32           value);
void             eva_xmlrpc_array_add_boolean  (EvaXmlrpcArray  *array,
                                                gboolean         value);
void             eva_xmlrpc_array_add_double   (EvaXmlrpcArray  *array,
                                                gdouble          value);
void             eva_xmlrpc_array_add_string   (EvaXmlrpcArray  *array,
                                                const char      *value);
void             eva_xmlrpc_array_add_date     (EvaXmlrpcArray  *array,
                                                gulong           value);

/* these take ownership of second argument */  
void             eva_xmlrpc_array_add_data     (EvaXmlrpcArray  *array,
                                                GByteArray      *data);
void             eva_xmlrpc_array_add_struct   (EvaXmlrpcArray  *array,
                                                EvaXmlrpcStruct *substructure);


EvaXmlrpcArray  *eva_xmlrpc_array_new          (void);
void             eva_xmlrpc_array_free         (EvaXmlrpcArray  *array);
void             eva_xmlrpc_array_add_int32    (EvaXmlrpcArray  *array,
                                                gint32           value);
void             eva_xmlrpc_array_add_boolean  (EvaXmlrpcArray  *array,
                                                gboolean         value);
void             eva_xmlrpc_array_add_double   (EvaXmlrpcArray  *array,
                                                gdouble          value);
void             eva_xmlrpc_array_add_string   (EvaXmlrpcArray  *array,
                                                const char      *value);
void             eva_xmlrpc_array_add_date     (EvaXmlrpcArray  *array,
                                                gulong           value);

/* these take ownership of second argument */  
void             eva_xmlrpc_array_add_data     (EvaXmlrpcArray  *array,
                                                GByteArray      *data);
void             eva_xmlrpc_array_add_struct   (EvaXmlrpcArray  *array,
                                                EvaXmlrpcStruct *substructure);
void             eva_xmlrpc_array_add_array    (EvaXmlrpcArray  *array,
                                                EvaXmlrpcArray  *subarray);

EvaXmlrpcRequest*eva_xmlrpc_request_new        (EvaXmlrpcStream *xmlrpc_stream);
EvaXmlrpcRequest*eva_xmlrpc_request_ref        (EvaXmlrpcRequest*request);
void             eva_xmlrpc_request_unref      (EvaXmlrpcRequest*request);
void             eva_xmlrpc_request_set_name   (EvaXmlrpcRequest*request,
                                                const char      *name);
void             eva_xmlrpc_request_add_int32  (EvaXmlrpcRequest *request,
                                                gint32           value);
void             eva_xmlrpc_request_add_boolean(EvaXmlrpcRequest *request,
                                                gboolean         value);
void             eva_xmlrpc_request_add_double (EvaXmlrpcRequest *request,
                                                gdouble          value);
void             eva_xmlrpc_request_add_string (EvaXmlrpcRequest *request,
                                                const char      *value);
void             eva_xmlrpc_request_add_date   (EvaXmlrpcRequest *request,
                                                gulong           value);

/* these take ownership of second argument */
void             eva_xmlrpc_request_add_data (EvaXmlrpcRequest *request,
                                              GByteArray      *data);
void             eva_xmlrpc_request_add_struct(EvaXmlrpcRequest *request,
                                              EvaXmlrpcStruct *substructure);
void             eva_xmlrpc_request_add_array(EvaXmlrpcRequest *request,
                                              EvaXmlrpcArray  *array);

EvaXmlrpcResponse  *eva_xmlrpc_response_new      (void);
EvaXmlrpcResponse  *eva_xmlrpc_response_ref      (EvaXmlrpcResponse   *response);
void             eva_xmlrpc_response_unref    (EvaXmlrpcResponse   *response);
void             eva_xmlrpc_response_add_int32(EvaXmlrpcResponse *response,
                                              gint32           value);
void             eva_xmlrpc_response_add_double(EvaXmlrpcResponse *response,
                                              gdouble          value);
void             eva_xmlrpc_response_add_boolean(EvaXmlrpcResponse *response,
                                                gboolean         value);
void             eva_xmlrpc_response_add_string(EvaXmlrpcResponse *response,
                                              const char      *value);
void             eva_xmlrpc_response_add_date (EvaXmlrpcResponse *response,
                                              gulong           value);

/* these take ownership of second argument */
void             eva_xmlrpc_response_add_data (EvaXmlrpcResponse *response,
                                              GByteArray      *data);
void             eva_xmlrpc_response_add_struct(EvaXmlrpcResponse *response,
                                              EvaXmlrpcStruct *substructure);
void             eva_xmlrpc_response_add_array(EvaXmlrpcResponse *response,
                                              EvaXmlrpcArray  *array);


void             eva_xmlrpc_response_fault   (EvaXmlrpcResponse *response,
                                              EvaXmlrpcStruct   *structure);

/* a parser */
EvaXmlrpcParser  *eva_xmlrpc_parser_new (EvaXmlrpcStream* stream);
gboolean          eva_xmlrpc_parser_feed (EvaXmlrpcParser *parser,
				          const char              *text,
				          gssize                   len,
				          GError                 **error);
EvaXmlrpcRequest *eva_xmlrpc_parser_get_request (EvaXmlrpcParser *parser);
EvaXmlrpcResponse *eva_xmlrpc_parser_get_response (EvaXmlrpcParser *parser);
void              eva_xmlrpc_parser_free (EvaXmlrpcParser *parser);


/* printing */
#include "../evabuffer.h"
void eva_xmlrpc_response_to_buffer (EvaXmlrpcResponse *response,
				    EvaBuffer         *buffer);
void eva_xmlrpc_request_to_buffer  (EvaXmlrpcRequest  *request,
				    EvaBuffer         *buffer);

G_END_DECLS

#endif
