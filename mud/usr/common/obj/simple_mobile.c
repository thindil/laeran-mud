#include <phantasmal/lpc_names.h>

inherit MOBILE;

void upgraded(varargs int clone);

static void create(varargs int clone)
{
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
}

string get_type(void)
{
  return "simple";
}
