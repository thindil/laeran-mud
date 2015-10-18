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

void from_dtd_unq(mixed* unq)
{
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);
}
