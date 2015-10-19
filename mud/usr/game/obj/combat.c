#include <kernel/kernel.h>

#include <type.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <config.h>

static object fighter1, fighter2;
static int combat_call;
static mapping combat_info1, combat_info2;

static void create(varargs int clone)
{
  if (clone)
    {
      fighter1 = nil;
      fighter2 = nil;
      combat_info1 = ([ ]);
      combat_info2 = ([ ]);
      combat_call = 0;
    }
}

void start_combat(object new_fighter1, object new_fighter2)
{
  object *objs;
  int i,j;
  int *tmp;
  string *locs;
  
  fighter1 = new_fighter1; /* always player body */
  fighter2 = new_fighter2; /* player or mobile body */
  combat_call = call_out("combat_round", 2);
  TAGD->set_tag_value(fighter1, "Combat", 1);
  TAGD->set_tag_value(fighter2, "Combat", 1);

  combat_info1["skill"] = "walka/walka wręcz";
  combat_info1["damage"] = fighter1->get_mobile()->get_user()->get_stat_val("strength") / 10;
  combat_info1["armor"] = ({({0, "głowa"}), ({0, "tułów"}), ({0, "ręce"}), ({0, "dłonie"}), ({0, "nogi"})});
  objs = fighter1->objects_in_container();
  for (i = 0; i < sizeof(objs); i++)
    {
      if (objs[i]->is_weapon() && objs[i]->is_dressed())
	{
	  combat_info1["skill"] = objs[i]->get_skill();
	  combat_info1["damage"] += objs[i]->get_damage();
	}
      if (!objs[i]->is_weapon() && objs[i]->is_dressed())
	{
	  tmp = objs[i]->get_wearlocations();
	  for (j = 0; j < sizeof(tmp); j++)
	    {
	      combat_info1["armor"][tmp[j]][0] = objs[i]->get_armor();
	    }
	}
    }
  combat_info1["hit"] = fighter1->get_mobile()->get_user()->get_stat_val("strength") + fighter1->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
  combat_info1["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("agility") + fighter1->get_mobile()->get_user()->get_skill_val("walka/unik");
  
  if (fighter2->get_mobile()->get_user()) /* is player */
    {
      objs = fighter2->objects_in_container();
      combat_info2["skill"] = "walka/walka wręcz";
      combat_info2["damage"] = fighter2->get_mobile()->get_user()->get_stat_val("strength") / 10;
      combat_info2["armor"] = ({({0, "głowa"}), ({0, "tułów"}), ({0, "ręce"}), ({0, "dłonie"}), ({0, "nogi"})});
      for (i = 0; i < sizeof(objs); i++)
	{
	  if (objs[i]->is_weapon() && objs[i]->is_dressed())
	    {
	      combat_info2["skill"] = objs[i]->get_skill();
	      combat_info2["damage"] += objs[i]->get_damage();
	      break;
	    }
	  if (!objs[i]->is_weapon() && objs[i]->is_dressed())
	    {
	      tmp = objs[i]->get_wearlocations();
	      for (j = 0; j < sizeof(tmp); j++)
		{
		  combat_info2["armor"][tmp[j]][0] = objs[i]->get_armor();
		}
	    }
	}
      combat_info2["hit"] = fighter2->get_mobile()->get_user()->get_stat_val("strength") + fighter2->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
      combat_info2["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("agility") + fighter2->get_mobile()->get_user()->get_skill_val("walka/unik");
    }
  else /* is mobile */
    {
      combat_info2["skill"] = "";
      combat_info2["damage"] = fighter2->get_damage();
      combat_info2["hit"] = fighter2->get_combat_rating();
      combat_info2["evade"] = fighter2->get_combat_rating();
      locs = fighter2->get_body_locations();
      combat_info2["armor"] = allocate(sizeof(locs));
      for (i = 0; i < sizeof(locs); i++)
	{
	  combat_info2["armor"][i] = ({fighter2->get_armor(), locs[i]});
	}
    }
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
