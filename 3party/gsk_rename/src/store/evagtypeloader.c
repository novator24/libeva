#include <string.h>
#include <gmodule.h>
#include "../evaerror.h"
#include "evagtypeloader.h"

typedef struct _TypeTest TypeTest;

struct _TypeTest
{
  gpointer test_data;
  EvaTestTypeFunc test_func;
  TypeTest *next;
  GDestroyNotify destroy;
};

struct _EvaGtypeLoader
{
  int ref_count;
  TypeTest *first_test;
  TypeTest *last_test;

  EvaLoadTypeFunc load_type_func;
  gpointer load_type_data;
  GDestroyNotify load_type_destroy;
};

EvaGtypeLoader *
eva_gtype_loader_new (void)
{
  EvaGtypeLoader *loader;

  loader = g_new0 (EvaGtypeLoader, 1);
  loader->ref_count = 1;
  return loader;
}

static gboolean
test_type_is_a (GType type, gpointer is_a_type)
{
  return g_type_is_a (type, (GType) GPOINTER_TO_UINT (is_a_type));
}

void
eva_gtype_loader_add_type (EvaGtypeLoader *loader, GType type)
{
  eva_gtype_loader_add_test (loader,
			     test_type_is_a,
			     GUINT_TO_POINTER (type),
			     NULL);
}

void
eva_gtype_loader_add_test (EvaGtypeLoader  *loader,
			   EvaTestTypeFunc  type_func,
			   gpointer         test_data,
			   GDestroyNotify   test_destroy)
{
  TypeTest *test = g_new (TypeTest, 1);
  test->test_data = test_data;
  test->test_func = type_func;
  test->destroy = test_destroy;
  test->next = NULL;
  if (loader->last_test != NULL)
    loader->last_test->next = test;
  else
    loader->first_test = test;
  loader->last_test = test;
}

gboolean
eva_gtype_loader_test_type (EvaGtypeLoader *loader, GType type)
{
  TypeTest *test;
  for (test = loader->first_test; test != NULL; test = test->next)
    if ((*test->test_func) (type, test->test_data))
      return TRUE;
  return FALSE;
}

void
eva_gtype_loader_set_loader (EvaGtypeLoader      *loader,
			     EvaLoadTypeFunc  load_type_func,
			     gpointer           load_type_data,
			     GDestroyNotify     load_type_destroy)
{
  if (loader->load_type_destroy != NULL)
    (*loader->load_type_destroy) (loader->load_type_data);
  loader->load_type_func = load_type_func;
  loader->load_type_data = load_type_data;
  loader->load_type_destroy = load_type_destroy;
}

GType
eva_gtype_loader_load_type (EvaGtypeLoader  *loader,
			  const char    *type_name,
			  GError       **error)
{
  if (loader->load_type_func == NULL)
    return g_type_from_name (type_name);
  else
    return (*loader->load_type_func) (type_name, loader->load_type_data, error);
}

void
eva_gtype_loader_ref (EvaGtypeLoader *loader)
{
  g_return_if_fail (loader->ref_count > 0);
  ++loader->ref_count;
}

void
eva_gtype_loader_unref (EvaGtypeLoader *loader)
{
  g_return_if_fail (loader->ref_count > 0);
  if (--loader->ref_count == 0)
    {
      while (loader->first_test != NULL)
	{
	  TypeTest *test = loader->first_test;

	  loader->first_test = test->next;
	  if (loader->first_test == NULL)
	    loader->last_test = NULL;

	  if (test->destroy)
	    (*test->destroy) (test->test_data);

	  g_free (test);
	}
      g_free (loader);
    }
}

static gboolean
return_true (GType type, gpointer data)
{
  (void) type;
  (void) data;
  return TRUE;
}

static EvaGtypeLoader *default_config = NULL;

EvaGtypeLoader *
eva_gtype_loader_default (void)
{
  if (!default_config)
    {
      default_config = eva_gtype_loader_new ();
      eva_gtype_loader_add_test (default_config, return_true, NULL, NULL);
      eva_gtype_loader_set_loader (default_config,
				 eva_load_type_introspective,
				 NULL,
				 NULL);
    }
  eva_gtype_loader_ref (default_config);
  return default_config;
}

GType
eva_load_type_introspective (const char  *type_name,
			     gpointer     unused,
			     GError     **error)
{
  static gboolean self_inited = FALSE;
  static GModule *self_module = NULL;
  guint index = 0;
  GType type;
  GString *func_name;
  gpointer symbol;

  (void) unused;

  type = g_type_from_name (type_name);
  if (type != G_TYPE_INVALID)
    return type;

  /* Transform `GObject' into `g_object_get_type',
   * which should be a function that returns a GType,
   * if we're lucky...
   */
  func_name = g_string_new ("");
  while (type_name[index] != '\0')
    {
      if ('A' <= type_name[index] && type_name[index] <= 'Z')
	{
	  if (index > 0)
	    g_string_append_c (func_name, '_');
	  g_string_append_c (func_name, g_ascii_tolower (type_name[index]));
	}
      else
	g_string_append_c (func_name, type_name[index]);
      ++index;
    }
  g_string_append (func_name, "_get_type");

  if (!self_inited)
    {
      self_inited = TRUE;
      self_module = g_module_open (NULL, G_MODULE_BIND_LAZY);
      if (self_module == NULL)
	{
	  g_set_error (error,
		       EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_UNKNOWN,
		       "g_module_open: %s",
		       g_module_error ());
	  goto DONE;
	}
    }
  if (g_module_symbol (self_module, func_name->str, &symbol))
    {
      GType (*func) () = (GType (*)()) symbol;
      const char *name;
      GTypeClass *klass;

      type = (*func) ();
      name = g_type_name (type);
      if (name == NULL)
	{
	  g_set_error (error,
		       EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_UNKNOWN,
		       "called %s, didn't get a valid GType",
		       func_name->str);
	  type = G_TYPE_INVALID;
	  goto DONE;
	}
      if (strcmp (name, type_name) != 0)
	{
	  g_set_error (error,
		       EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_UNKNOWN,
		       "called %s: got %s instead of %s",
		       func_name->str,
		       name,
		       type_name);
	  type = G_TYPE_INVALID;
	  goto DONE;
	}

      /* Sometimes the registrations in the class_init are vital. */
      klass = g_type_class_ref (type);
      g_type_class_unref (klass);
    }
  else
    {
      g_set_error (error,
		   EVA_G_ERROR_DOMAIN,
		   EVA_ERROR_UNKNOWN,
		   "couldn't find symbol %s: %s",
		   func_name->str,
		   g_module_error ());
    }

DONE:
  g_string_free (func_name, TRUE);
  return type;
}
