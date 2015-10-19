#include <kernel/kernel.h>

#include <type.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <config.h>

static object fighter1;
static object fighter2;
static int combat_call;

static void create(varargs int clone)
{
  if (clone)
    {
      fighter1 = nil;
      fighter2 = nil;
      combat_call = 0;
    }
}

void start_combat(object new_fighter1, object new_fighter2)
{
  fighter1 = new_fighter1;
  fighter2 = new_fighter2;
  combat_call = call_out("combat_round", 2);
  TAGD->set_tag_value(fighter1, "Combat", 1);
  TAGD->set_tag_value(fighter2, "Combat", 1);
}

void combat_round(void)
{ 
  fighter1->get_mobile()->get_user()->message("test\n");
  combat_call = call_out("combat_round", 2);
}

void stop_combat()
{
  remove_call_out(combat_call);
  TAGD->set_tag_value(fighter1, "Combat", nil);
  TAGD->set_tag_value(fighter2, "Combat", nil);
}
