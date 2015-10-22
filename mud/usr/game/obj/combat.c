#include <kernel/kernel.h>

#include <type.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <config.h>

static object fighter1, fighter2;
static int combat_call;
static mapping combat_info1, combat_info2;

void stop_combat(void);

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
  int i,j, fatigue;
  int *tmp;
  string *locs;
  
  fighter1 = new_fighter1; /* always player body */
  fighter2 = new_fighter2; /* player or mobile body */
  combat_call = call_out("combat_round", 2);
  TAGD->set_tag_value(fighter1, "Combat", 1);
  TAGD->set_tag_value(fighter2, "Combat", 1);

  combat_info1["name"] = fighter1->get_brief()->to_string(fighter1->get_mobile()->get_user());
  combat_info1["skill"] = "walka/walka wręcz";
  combat_info1["damage"] = fighter1->get_mobile()->get_user()->get_stat_val("siła") / 10;
  combat_info1["armor"] = ({({0, "głowa"}), ({0, "tułów"}), ({0, "ręce"}), ({0, "dłonie"}), ({0, "nogi"})});
  if (TAGD->get_tag_value(fighter1, "Fatigue"))
    {
      fatigue = TAGD->get_tag_value(fighter1, "Fatigue");
    }
  else
    {
      fatigue = 0;
    }
  combat_info1["stamina"] = (fighter1->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - fatigue;
  combat_info1["stam_cost"] = 1;
  if (TAGD->get_tag_value(fighter1, "Hp"))
    {
      combat_info1["hp"] = TAGD->get_tag_value(fighter1, "Hp");
    }
  else
    {
      combat_info1["hp"] = fighter1->get_hp();
    }
  objs = fighter1->objects_in_container();
  for (i = 0; i < sizeof(objs); i++)
    {
      if (objs[i]->is_weapon() && objs[i]->is_dressed())
	{
	  combat_info1["skill"] = objs[i]->get_skill();
	  combat_info1["damage"] += objs[i]->get_damage();
	  combat_info1["stam_cost"] += (int)objs[i]->get_weight();
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
  combat_info1["hit"] = fighter1->get_mobile()->get_user()->get_stat_val("siła") + fighter1->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
  combat_info1["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("zręczność") + fighter1->get_mobile()->get_user()->get_skill_val("walka/unik");
  combat_info1["exp"] = combat_info1["hit"] + combat_info1["evade"] + combat_info1["hp"];

  if (TAGD->get_tag_value(fighter2, "Hp"))
    {
      combat_info2["hp"] = TAGD->get_tag_value(fighter2, "Hp");
    }
  else
    {
      combat_info2["hp"] = fighter2->get_hp();
    }
  combat_info2["name"] = fighter2->get_brief()->to_string(fighter1->get_mobile()->get_user());
  combat_info2["stamina"] = 100;
  combat_info2["stam_cost"] = 1;
  if (fighter2->get_mobile()->get_user()) /* is player */
    {
      objs = fighter2->objects_in_container();
      combat_info2["skill"] = "walka/walka wręcz";
      combat_info2["damage"] = fighter2->get_mobile()->get_user()->get_stat_val("siła") / 10;
      combat_info2["armor"] = ({({0, "głowa"}), ({0, "tułów"}), ({0, "ręce"}), ({0, "dłonie"}), ({0, "nogi"})});
      if (TAGD->get_tag_value(fighter2, "Fatigue"))
	{
	  fatigue = TAGD->get_tag_value(fighter2, "Fatigue");
	}
      else
	{
	  fatigue = 0;
	}
      combat_info2["stamina"] = (fighter2->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - fatigue;
      for (i = 0; i < sizeof(objs); i++)
	{
	  if (objs[i]->is_weapon() && objs[i]->is_dressed())
	    {
	      combat_info2["skill"] = objs[i]->get_skill();
	      combat_info2["damage"] += objs[i]->get_damage();
	      combat_info2["stam_cost"] += (int)objs[i]->get_weight();
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
      combat_info2["hit"] = fighter2->get_mobile()->get_user()->get_stat_val("siła") + fighter2->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
      combat_info2["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("zręczność") + fighter2->get_mobile()->get_user()->get_skill_val("walka/unik");
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
  combat_info2["exp"] = combat_info2["hit"] + combat_info2["evade"] + combat_info2["hp"];
}

void combat_round(void)
{
  int hit, evade, loc, dmg;
  string message, message2;
  if (combat_info1["stamina"] > 0)
    {
      hit = combat_info1["hit"] + random(50);
    }
  else
    {
      hit = (combat_info1["hit"] / 2) + (random(50) / 2);
    }
  if (combat_info2["stamina"] > 0)
    {
      evade = combat_info2["evade"] + random(50);
    }
  else
    {
      evade = (combat_info2["evade"] / 2) + (random(50) / 2);
    }
  combat_info1["stamina"] -= combat_info1["stam_cost"];
  message = "Atakujesz " + combat_info2["name"];
  message2 = combat_info1["name"] + " atakuje Ciebie";
  if (hit > evade)
    {
      loc = random(sizeof(combat_info2["armor"]));
      message += " i  trafiasz go w " + combat_info2["armor"][loc][1] + ".";
      message2 +=  " i trafia cię w " + combat_info2["armor"][loc][1] + ".";
      dmg = combat_info1["damage"] - combat_info2["armor"][loc][0];
      if (dmg < 0)
	{
	  dmg = 0;
	}
      combat_info2["hp"] -= dmg;
      if (dmg = 0)
	{
	  message += " Jednak atak odbija się od jego ciała.";
	  message2 += " Na szczęście atak odbija się od Twojej zbroi.";
	}
      combat_info1["hits"] = 1;
    }
  else
    {
      message += " ale ten unika twojego ciosu.";
      message2 += " ale udaje Ci się uniknąć ciosu.";
      combat_info2["dodge"] = 1;
      combat_info2["stamina"]--;
    }
  fighter1->get_mobile()->get_user()->message(message + "\n");
  if (fighter2->get_mobile()->get_user())
    {
      fighter2->get_mobile()->get_user()->message(message2 + "\n");
    }
  if (combat_info2["stamina"] > 0)
    {
      hit = combat_info2["hit"] + random(50);
    }
  else
    {
      hit = (combat_info2["hit"] / 2) + (random(50) / 2);
    }
  if (combat_info1["stamina"] > 0)
    {
      evade = combat_info1["evade"] + random(50);
    }
  else
    {
      evade = (combat_info1["evade"] / 2) + (random(50) / 2);
    }
  combat_info2["stamina"] -= combat_info2["stam_cost"];
  message2 = "Atakujesz " + combat_info1["name"];
  message = combat_info2["name"] + " atakuje Ciebie";
  if (hit > evade)
    {
      loc = random(sizeof(combat_info1["armor"]));
      message2 += " i  trafiasz go w " + combat_info1["armor"][loc][1] + ".";
      message +=  " i trafia cię w " + combat_info1["armor"][loc][1] + ".";
      dmg = combat_info2["damage"] - combat_info1["armor"][loc][0];
      if (dmg < 0)
	{
	  dmg = 0;
	}
      combat_info1["hp"] -= dmg;
      if (dmg = 0)
	{
	  message2 += " Jednak atak odbija się od jego ciała.";
	  message += " Na szczęście atak odbija się od Twojej zbroi.";
	}
      combat_info2["hits"] = 1;
    }
  else
    {
      message2 += " ale ten unika twojego ciosu.";
      message += " ale udaje Ci się uniknąć ciosu.";
      combat_info1["dodge"] = 1;
      combat_info1["stamina"]--;
    }
  fighter1->get_mobile()->get_user()->message(message + "\n");
  fighter1->get_mobile()->get_user()->print_prompt();
  if (fighter2->get_mobile()->get_user())
    {
      fighter2->get_mobile()->get_user()->message(message2 + "\n");
      fighter2->get_mobile()->get_user()->print_prompt();
    }
  if (combat_info1["hp"] < 1)
    {
      if (fighter2->get_mobile()->get_user())
	{
	  fighter2->get_mobile()->get_user()->message(fighter1->get_brief()->to_string(fighter1->get_mobile()->get_user()) + " ginie.\n");
	  fighter2->get_mobile()->get_user()->gain_exp(combat_info2["skill"], combat_info1["exp"]);
	  fighter2->get_mobile()->get_user()->gain_exp("siła", combat_info1["exp"]);
	  fighter2->get_mobile()->get_user()->gain_exp("kondycja", combat_info1["exp"]);
	  if (combat_info2["dodge"])
	    {
	      fighter2->get_mobile()->get_user()->gain_exp("walka/uniki", combat_info1["exp"]);
	      fighter2->get_mobile()->get_user()->gain_exp("zręczność", combat_info1["exp"]);
	    }
	}
      fighter1->get_mobile()->get_user()->death();
      stop_combat();
    }
  else if (combat_info2["hp"] < 1)
    {
      fighter1->get_mobile()->get_user()->message(fighter2->get_brief()->to_string(fighter1->get_mobile()->get_user()) + " ginie.\n");
      if (fighter2->get_mobile()->get_user())
	{
	  fighter2->get_mobile()->get_user()->death();
	}
      else
	{
	  fighter2->get_mobile()->death(fighter1->get_mobile()->get_user());
	}
      fighter1->get_mobile()->get_user()->gain_exp(combat_info1["skill"], combat_info2["exp"]);
      fighter1->get_mobile()->get_user()->gain_exp("siła", combat_info2["exp"]);
      fighter1->get_mobile()->get_user()->gain_exp("kondycja", combat_info2["exp"]);
      if (combat_info1["dodge"])
	{
	  fighter1->get_mobile()->get_user()->gain_exp("walka/uniki", combat_info2["exp"]);
	  fighter1->get_mobile()->get_user()->gain_exp("zręczność", combat_info2["exp"]);
	}
      stop_combat();
    }
  else
    {
      combat_call = call_out("combat_round", 2);
    }
}

void stop_combat()
{
  int fatigue;
  
  remove_call_out(combat_call);
  TAGD->set_tag_value(fighter1, "Combat", nil);
  TAGD->set_tag_value(fighter2, "Combat", nil);
  if (combat_info1["hp"] > 0)
    {
      if (combat_info1["hp"] < fighter1->get_hp())
	{
	  TAGD->set_tag_value(fighter1, "Hp", combat_info1["hp"]);
	  fighter1->get_mobile()->get_user()->set_health(combat_info1["hp"]);
	}
      fatigue = (fighter1->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - combat_info1["stamina"];
      if (fatigue < 0)
	{
	  fatigue = 0;
	}
      TAGD->set_tag_value(fighter1, "Fatigue", fatigue);
      fighter1->get_mobile()->get_user()->set_condition(fatigue);
    }
  if (combat_info2["hp"] > 0)
    {
      if (combat_info2["hp"] < fighter2->get_hp())
	{
	  TAGD->set_tag_value(fighter2, "Hp", combat_info2["hp"]);
	}
      if (fighter2->get_mobile()->get_user())
	{
	  if (combat_info2["hp"] < fighter2->get_hp())
	    {
	      fighter2->get_mobile()->get_user()->set_health(combat_info2["hp"]);
	    }
	  fatigue = (fighter2->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - combat_info2["stamina"];
	  if (fatigue < 0)
	    {
	      fatigue = 0;
	    }
	  TAGD->set_tag_value(fighter2, "Fatigue", fatigue);
	  fighter2->get_mobile()->get_user()->set_condition(fatigue);
	}
    }
}
