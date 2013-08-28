#include "evaxmlrpc.h"
#include "../common/evadate.h"
#include "../common/evabase64.h"
#include <string.h>

static void
append_value (EvaBuffer *buffer, const EvaXmlrpcValue *value)
{
  switch (value->type)
    {
    case EVA_XMLRPC_INT32:
      eva_buffer_printf (buffer, "    <value><int>%d</int></value>\n",
			 value->data.v_int32);
      break;
    case EVA_XMLRPC_BOOLEAN:
      eva_buffer_printf (buffer, "    <value><boolean>%d</boolean></value>\n",
			 value->data.v_boolean ? 1 : 0);
      break;
    case EVA_XMLRPC_DOUBLE:
      eva_buffer_printf (buffer, "    <value><double>%.21g</double></value>\n",
			 value->data.v_double);
      break;
    case EVA_XMLRPC_STRING:
      {
	char *encoded = g_markup_escape_text (value->data.v_string, -1);
	eva_buffer_printf (buffer, "    <value><string>%s</string></value>\n",
			   encoded); 
	g_free (encoded);
      }
    break;
    case EVA_XMLRPC_DATE:
      {
	char date_buf[EVA_DATE_MAX_LENGTH];
	eva_date_print_timet (value->data.v_date,
			      date_buf, EVA_DATE_MAX_LENGTH,
			      EVA_DATE_FORMAT_ISO8601);
	eva_buffer_printf (buffer, "    <value><dateTime.iso8601>%s</dateTime.iso8601></value>\n",
			   date_buf); 
      }
      break;
    case EVA_XMLRPC_BINARY_DATA:
      {
	GByteArray *data =value->data.v_binary_data;
	char *encoded = eva_base64_encode_alloc ((char*)(data->data), data->len);
	eva_buffer_append_string (buffer, "  <value><base64>\n");
	eva_buffer_append_foreign (buffer, encoded, strlen (encoded),
				   g_free, encoded);
	eva_buffer_append_string (buffer, "  </base64></value>\n");
      }
      break;
    case EVA_XMLRPC_STRUCT:
      {
	EvaXmlrpcStruct *st = value->data.v_struct;
	guint i;
	eva_buffer_append_string (buffer, "  <value><struct>\n");
	for (i = 0; i < st->n_members; i++)
	  {
	    eva_buffer_printf (buffer, "    <member>\n"
			               "      <name>%s</name>\n", st->members[i].name);
	    append_value (buffer, &st->members[i].value);
	    eva_buffer_append_string (buffer, "    </member>\n");
	  }
	eva_buffer_append_string (buffer, "  </struct></value>\n");
	
      }
      break;
    case EVA_XMLRPC_ARRAY:
      {
	EvaXmlrpcArray *ar = value->data.v_array;
	guint i;
	eva_buffer_append_string (buffer, "  <value><array><data>\n");
	for (i = 0; i < ar->len; i++)
	  {
	    append_value (buffer, ar->values + i);
	  }
	eva_buffer_append_string (buffer, "  </data></array></value>\n");
	
      }
      break;
    default:
      g_assert_not_reached ();
    }
}

/**
 * eva_xmlrpc_response_to_buffer:
 * @response: the XMLRPC response to serialize.
 * @buffer: the buffer to append to.
 *
 * Write the XML corresponding to this response to the buffer.
 */
void eva_xmlrpc_response_to_buffer (EvaXmlrpcResponse *response,
				    EvaBuffer         *buffer)
{
  eva_buffer_append_string (buffer, "<methodResponse>\n");
  if (response->has_fault)
    {
      eva_buffer_append_string (buffer, " <fault>\n");
      append_value (buffer, &response->fault);
      eva_buffer_append_string (buffer, " </fault>\n");
    }
  else
    {
      guint i;
      eva_buffer_append_string (buffer, " <params>\n");
      for (i = 0; i < response->params->len; i++)
	{
	  eva_buffer_append_string (buffer, " <param>\n");
	  append_value (buffer, response->params->values + i);
	  eva_buffer_append_string (buffer, " </param>\n");
	}
      eva_buffer_append_string (buffer, " </params>\n");
    }
  eva_buffer_append_string (buffer, "</methodResponse>\n");
}

/* eva_xmlrpc_request_to_buffer:
 * @request: the XMLRPC request to serialize.
 * @buffer: the buffer to append to.
 *
 * Write the XML corresponding to this request to the buffer.
 */
void eva_xmlrpc_request_to_buffer  (EvaXmlrpcRequest  *request,
				    EvaBuffer         *buffer)
{
  guint i;
  eva_buffer_append_string (buffer, "<methodCall>\n");
  eva_buffer_printf (buffer, "  <methodName>%s</methodName>\n", request->method_name);
  eva_buffer_append_string (buffer, " <params>\n");
  for (i = 0; i < request->params->len; i++)
    {
      eva_buffer_append_string (buffer, " <param>\n");
      append_value (buffer, request->params->values + i);
      eva_buffer_append_string (buffer, " </param>\n");
    }
  eva_buffer_append_string (buffer, " </params>\n");
  eva_buffer_append_string (buffer, "</methodCall>\n");
}
