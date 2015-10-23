#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

inherit MOBILE;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
}

string get_type(void) {
  return "wander";
}

void random_move(void)
{
  int    num_ex, ctr;
  object exit, dest;
  string reason;

  num_ex = location->num_exits();
  if(num_ex > 0)
    {
      int dir;

      ctr = random(num_ex + 10);
      if (ctr >= (num_ex - 1))
	{
	  return;
	}
      exit = location->get_exit_num(ctr);
      if(!exit)
	{
	  error("Internal error!  Can't get exit!");
	}
      
      dir = exit->get_direction();
      dest = exit->get_destination();
      
      reason = this_object()->move(dir);
      if(reason)
	{
	  error(reason);
	}
    }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
