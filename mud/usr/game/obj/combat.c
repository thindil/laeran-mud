#include <kernel/kernel.h>

#include <type.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <config.h>

static object fighter1, fighter2;
static int fighter1_call, fighter2_call;
static mapping combat_info1, combat_info2;

void stop_combat(void);
void combat_info(string message, string message2);
void fighter1_shoot(void);

static void create(varargs int clone)
{
  if (clone)
    {
      fighter1 = nil;
      fighter2 = nil;
      combat_info1 = ([ ]);
      combat_info2 = ([ ]);
      fighter1_call = 0;
      fighter2_call = 0;
    }
}

void start_combat(object new_fighter1, object new_fighter2, int shoot)
{
    object *objs;
    int i,j, fatigue;
    int *tmp;
    string *locs, *nouns;

    fighter1 = new_fighter1; /* always player body */
    fighter2 = new_fighter2; /* player or mobile body */
    TAGD->set_tag_value(fighter1, "Combat", 1);
    TAGD->set_tag_value(fighter2, "Combat", 1);

    combat_info1["name"] = fighter1->get_brief()->to_string(fighter1->get_mobile()->get_user());
    nouns = fighter1->get_nouns(fighter1->get_mobile()->get_user()->get_locale());
    if (sizeof(nouns) > 3)
        combat_info1["conj_name"] = nouns[3];
    else
        combat_info1["conj_name"] = combat_info1["name"];
    combat_info1["skill"] = "walka/walka wręcz";
    combat_info1["damage_type"] = "crush";
    combat_info1["damage"] = fighter1->get_mobile()->get_user()->get_stat_val("siła") / 10;
    combat_info1["armor"] = ({({0, "głowa", 0}), ({0, "tułów", 0}), ({0, "ręce", 0}), ({0, "dłonie", 0}), ({0, "nogi", 0})});
    combat_info1["damage_res"] = ([ ]);
    combat_info1["delay"] = 2.0 - ((float)fighter1->get_mobile()->get_user()->get_stat_val("siła") / 100.0); 
    if (TAGD->get_tag_value(fighter1, "Fatigue"))
        fatigue = TAGD->get_tag_value(fighter1, "Fatigue");
    else
        fatigue = 0;
    combat_info1["stamina"] = (fighter1->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - fatigue;
    combat_info1["stam_cost"] = 1;
    if (TAGD->get_tag_value(fighter1, "Hp"))
        combat_info1["hp"] = TAGD->get_tag_value(fighter1, "Hp");
    else
        combat_info1["hp"] = fighter1->get_hp();
    objs = fighter1->objects_in_container();
    for (i = 0; i < sizeof(objs); i++) {
        if (objs[i]->is_dressed()) {
            tmp = objs[i]->get_wearlocations();
            if (tmp[0] == 5) {
                combat_info1["skill"] = objs[i]->get_skill();
                combat_info1["damage"] += objs[i]->get_damage();
                combat_info1["stam_cost"] += (int)objs[i]->get_weight();
                combat_info1["damage_type"] = objs[i]->get_damage_type();
                combat_info1["weapon_num"] = objs[i]->get_number();
            }
            else if (sizeof(tmp & ({10}))) {
                combat_info1["shoot_skill"] = objs[i]->get_skill();
                locs = explode(objs[i]->get_cur_magazine()[0], ":");
                combat_info1["shoot_damage_type"] = locs[0];
                combat_info1["shoot_damage"] = (int)locs[1];
                combat_info1["shoot_weapon_num"] = objs[i]->get_number();
            }
            else {
                for (j = 0; j < sizeof(tmp); j++) 
                    combat_info1["armor"][tmp[j]][0] = objs[i]->get_armor();
            }
        }
    }
    combat_info1["hit"] = fighter1->get_mobile()->get_user()->get_stat_val("siła") + fighter1->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
    if (shoot > -1)
        combat_info1["shoot_hit"] = fighter1->get_mobile()->get_user()->get_stat_val("zręczność") + fighter1->get_mobile()->get_user()->get_skill_val(combat_info1["shoot_skill"]);
    combat_info1["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("zręczność") + fighter1->get_mobile()->get_user()->get_skill_val("walka/unik");
    combat_info1["exp"] = combat_info1["hit"] + combat_info1["evade"] + combat_info1["hp"];
    combat_info1["delay"] -= ((float)fighter1->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]) / 100.0);
    combat_info1["delay"] += (float)(combat_info1["stam_cost"] - 1);
    if (combat_info1["delay"] > 4.0)
        combat_info1["delay"] = 4.0;
    combat_info1["hits"] = 0;
    combat_info1["shoots"] = 0;

    if (TAGD->get_tag_value(fighter2, "Hp"))
        combat_info2["hp"] = TAGD->get_tag_value(fighter2, "Hp");
    else
        combat_info2["hp"] = fighter2->get_hp();
    combat_info2["name"] = fighter2->get_brief()->to_string(fighter1->get_mobile()->get_user());
    nouns = fighter2->get_nouns(fighter1->get_mobile()->get_user()->get_locale());
    if (sizeof(nouns) > 3)
        combat_info2["conj_name"] = nouns[3];
    else
        combat_info2["conj_name"] = combat_info2["name"];

    combat_info2["stamina"] = 100;
    combat_info2["stam_cost"] = 1;
    if (fighter2->get_mobile()->get_user()) /* is player */
    {
        objs = fighter2->objects_in_container();
        combat_info2["skill"] = "walka/walka wręcz";
        combat_info2["damage"] = fighter2->get_mobile()->get_user()->get_stat_val("siła") / 10;
        combat_info2["damage_type"] = "crush";
        combat_info2["armor"] = ({({0, "głowa", 0}), ({0, "tułów", 0}), ({0, "ręce", 0}), ({0, "dłonie", 0}), ({0, "nogi", 0})});
        combat_info2["damage_res"] = ([ ]);
        if (TAGD->get_tag_value(fighter2, "Fatigue"))
            fatigue = TAGD->get_tag_value(fighter2, "Fatigue");
        else
            fatigue = 0;
        combat_info2["stamina"] = (fighter2->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - fatigue;
        combat_info2["delay"] = 2.0 - ((float)fighter2->get_mobile()->get_user()->get_stat_val("siła") / 100.0); 
        for (i = 0; i < sizeof(objs); i++) {
            if (objs[i]->is_dressed()) {
                tmp = objs[i]->get_wearlocations();
                if (tmp[0] == 5) {
                    combat_info2["skill"] = objs[i]->get_skill();
                    combat_info2["damage"] += objs[i]->get_damage();
                    combat_info2["stam_cost"] += (int)objs[i]->get_weight();
                    combat_info2["damage_type"] = objs[i]->get_damage_type();
                    combat_info2["weapon_num"] = objs[i]->get_number();
                } else {
                    for (j = 0; j < sizeof(tmp); j++) 
                        combat_info2["armor"][tmp[j]][0] = objs[i]->get_armor();
                }
            }
        }
        combat_info2["hit"] = fighter2->get_mobile()->get_user()->get_stat_val("siła") + fighter2->get_mobile()->get_user()->get_skill_val(combat_info1["skill"]);
        combat_info2["evade"] = fighter1->get_mobile()->get_user()->get_stat_val("zręczność") + fighter2->get_mobile()->get_user()->get_skill_val("walka/unik");
        combat_info2["delay"] -= ((float)fighter2->get_mobile()->get_user()->get_skill_val(combat_info2["skill"]) / 100.0);
        combat_info2["delay"] += (float)(combat_info2["stam_cost"] - 1);
        if (combat_info2["delay"] > 4.0)
            combat_info2["delay"] = 4.0;
    }
    else /* is mobile */
    {
        combat_info2["skill"] = "";
        combat_info2["damage"] = fighter2->get_damage();
        combat_info2["damage_type"] = fighter2->get_damage_type();
        combat_info2["hit"] = fighter2->get_combat_rating();
        combat_info2["evade"] = fighter2->get_combat_rating();
        locs = fighter2->get_body_locations();
        combat_info2["armor"] = allocate(sizeof(locs));
        combat_info2["delay"] = 3.0 - ((float)(fighter2->get_combat_rating()) / 50.0);
        if (combat_info2["delay"] < 2.0)
            combat_info2["delay"] = 2.0;
        for (i = 0; i < sizeof(locs); i++)
            combat_info2["armor"][i] = ({fighter2->get_armor(), locs[i], 0});
        combat_info2["damage_res"] = fighter2->get_damage_res();
    }
    combat_info2["exp"] = combat_info2["hit"] + combat_info2["evade"] + combat_info2["hp"];
    combat_info2["hits"] = 0;
    fighter1_call = call_out("fighter1_attack", combat_info1["delay"]);
    fighter2_call = call_out("fighter2_attack", combat_info2["delay"]);
    if (shoot > -1)
        fighter1_shoot();
}

void fighter1_shoot(void)
{
    int hit, evade, loc, dmg;
    string message, message2;
    object weapon;

    weapon = MAPD->get_room_by_num(combat_info1["shoot_weapon_num"]);
    if (sizeof(weapon->get_cur_magazine()) == 1)
        weapon->set_cur_magazine(({ }));
    else
        weapon->set_cur_magazine(weapon->get_cur_magazine()[1..]);
    hit = combat_info1["shoot_hit"] + random(50);
    evade = combat_info2["evade"] + random(50);
    combat_info1["stamina"] -= 10;
    if (combat_info1["stamina"] < 0)
        combat_info1["stamina"] = 0;
    message = "Strzelasz do " + combat_info2["conj_name"];
    message2 = combat_info1["name"] + " strzela do Ciebie";
    if (hit > evade) {
        loc = random(sizeof(combat_info2["armor"]));
        message += " i  trafiasz go w " + combat_info2["armor"][loc][1] + ".";
        message2 +=  " i trafia cię w " + combat_info2["armor"][loc][1] + ".";
        dmg = combat_info1["shoot_damage"] - combat_info2["armor"][loc][0];
        if (dmg < 0)
            dmg = 0;
        if (dmg > 0) {
            if (combat_info1["shoot_damage_type"] == "cut") 
                dmg = (int)((float)dmg * 1.5);
            else if (combat_info1["shoot_damage_type"] == "impaled")
                dmg = dmg * 2;
            if (combat_info2["damage_res"][combat_info1["shoot_damage_type"]])
                dmg -= (int)((float)dmg * ((float)combat_info2["damage_res"][combat_info1["shoot_damage_type"]] / 100.0));
        }
        combat_info2["hp"] -= dmg;
        if (dmg == 0) {
            message += " Jednak pocisk odbija się od jego ciała.";
            message2 += " Na szczęście pocisk odbija się od Twojej zbroi.";
        }
        combat_info1["shoots"]++;
        combat_info2["armor"][loc][2]++;
    } else {
        message += " ale ten unika twojego strzału.";
        message2 += " ale udaje Ci się uniknąć strzału.";
        combat_info2["dodge"] = 1;
        combat_info2["stamina"]--;
        if (combat_info2["stamina"] < 0)
            combat_info2["stamina"] = 0;
    }
    combat_info(message, message2);
}

void fighter1_attack(void)
{
    int hit, evade, loc, dmg;
    string message, message2;

    if (combat_info1["stamina"] > 0)
        hit = combat_info1["hit"] + random(50);
    else
        hit = (combat_info1["hit"] / 2) + (random(50) / 2);
    if (combat_info2["stamina"] > 0)
        evade = combat_info2["evade"] + random(50);
    else
        evade = (combat_info2["evade"] / 2) + (random(50) / 2);
    combat_info1["stamina"] -= combat_info1["stam_cost"];
    if (combat_info1["stamina"] < 0)
        combat_info1["stamina"] = 0;
    message = "Atakujesz " + combat_info2["conj_name"];
    message2 = combat_info1["name"] + " atakuje Ciebie";
    if (hit > evade) {
        loc = random(sizeof(combat_info2["armor"]));
        message += " i  trafiasz go w " + combat_info2["armor"][loc][1] + ".";
        message2 +=  " i trafia cię w " + combat_info2["armor"][loc][1] + ".";
        dmg = combat_info1["damage"] - combat_info2["armor"][loc][0];
        if (dmg < 0)
            dmg = 0;
        if (dmg > 0) {
            if (combat_info1["damage_type"] == "cut") 
                dmg = (int)((float)dmg * 1.5);
            else if (combat_info1["damage_type"] == "impaled")
                dmg = dmg * 2;
            if (combat_info2["damage_res"][combat_info1["damage_type"]])
                dmg -= (int)((float)dmg * ((float)combat_info2["damage_res"][combat_info1["damage_type"]] / 100.0));
        }
        combat_info2["hp"] -= dmg;
        if (dmg == 0) {
            message += " Jednak atak odbija się od jego ciała.";
            message2 += " Na szczęście atak odbija się od Twojej zbroi.";
        }
        combat_info1["hits"]++;
        combat_info2["armor"][loc][2]++;
    } else {
        message += " ale ten unika twojego ciosu.";
        message2 += " ale udaje Ci się uniknąć ciosu.";
        combat_info2["dodge"] = 1;
        combat_info2["stamina"]--;
        if (combat_info2["stamina"] < 0)
            combat_info2["stamina"] = 0;
    }
    combat_info(message, message2);
    if (combat_info2["hp"] > 0)
        fighter1_call = call_out("fighter1_attack", combat_info1["delay"]);
}

void fighter2_attack(void)
{
    int hit, evade, loc, dmg;
    string message, message2;

    if (combat_info2["stamina"] > 0)
        hit = combat_info2["hit"] + random(50);
    else
        hit = (combat_info2["hit"] / 2) + (random(50) / 2);
    if (combat_info1["stamina"] > 0)
        evade = combat_info1["evade"] + random(50);
    else
        evade = (combat_info1["evade"] / 2) + (random(50) / 2);
    combat_info2["stamina"] -= combat_info2["stam_cost"];
    if (combat_info2["stamina"] < 0)
        combat_info2["stamina"] = 0;
    message2 = "Atakujesz " + combat_info1["conj_name"];
    message = combat_info2["name"] + " atakuje Ciebie";
    if (hit > evade)
    {
        loc = random(sizeof(combat_info1["armor"]));
        message2 += " i  trafiasz go w " + combat_info1["armor"][loc][1] + ".";
        message +=  " i trafia cię w " + combat_info1["armor"][loc][1] + ".";
        dmg = combat_info2["damage"] - combat_info1["armor"][loc][0];
        if (dmg < 0)
            dmg = 0;
        if (dmg > 0) {
            if (combat_info2["damage_type"] == "cut")
                dmg = (int)((float)dmg * 1.5);
            else if (combat_info2["damage_type"] == "impaled")
                dmg = dmg * 2;
            if (combat_info1["damage_res"][combat_info2["damage_type"]])
                dmg -= (int)((float)dmg * ((float)combat_info1["damage_res"][combat_info2["damage_type"]] / 100.0));
        }
        combat_info1["hp"] -= dmg;
        if (dmg == 0)
        {
            message2 += " Jednak atak odbija się od jego ciała.";
            message += " Na szczęście atak odbija się od Twojej zbroi.";
        }
        combat_info2["hits"]++;
        combat_info1["armor"][loc][2]++;
    } else {
        message2 += " ale ten unika twojego ciosu.";
        message += " ale udaje Ci się uniknąć ciosu.";
        combat_info1["dodge"] = 1;
        combat_info1["stamina"]--;
        if (combat_info1["stamina"] < 0)
            combat_info1["stamina"] = 0;
    }
    combat_info(message, message2);
    if (combat_info1["hp"] > 0)
        fighter2_call = call_out("fighter2_attack", combat_info2["delay"]);
}

void combat_info(string message, string message2)
{
    fighter1->get_mobile()->get_user()->message(message + "\n");
    fighter1->get_mobile()->get_user()->set_condition(combat_info1["stamina"]);
    fighter1->get_mobile()->get_user()->set_health(combat_info1["hp"]);
    fighter1->get_mobile()->get_user()->print_prompt();
    fighter1->get_mobile()->get_user()->message("\n");
    if (fighter2->get_mobile()->get_user()) {
        fighter2->get_mobile()->get_user()->message(message2 + "\n");
        fighter2->get_mobile()->get_user()->set_condition(combat_info2["stamina"]);
        fighter2->get_mobile()->get_user()->set_health(combat_info2["hp"]);
        fighter2->get_mobile()->get_user()->print_prompt();
        fighter2->get_mobile()->get_user()->message("\n");
    }
    if (combat_info1["hp"] < 1) {
        if (fighter2->get_mobile()->get_user()) {
            fighter2->get_mobile()->get_user()->message(combat_info1["name"] + " ginie.\n");
            fighter2->get_mobile()->get_user()->gain_exp(combat_info2["skill"], combat_info1["exp"]);
            fighter2->get_mobile()->get_user()->gain_exp("siła", combat_info1["exp"]);
            fighter2->get_mobile()->get_user()->gain_exp("kondycja", combat_info1["exp"]);
            if (combat_info2["dodge"]) {
                fighter2->get_mobile()->get_user()->gain_exp("walka/uniki", combat_info1["exp"]);
                fighter2->get_mobile()->get_user()->gain_exp("zręczność", combat_info1["exp"]);
            }
        }
        stop_combat();
        fighter1->get_mobile()->get_user()->death();
    } else if (combat_info2["hp"] < 1) {
        fighter1->get_mobile()->get_user()->message(combat_info2["name"] + " ginie.\n");
        stop_combat();
        if (fighter2->get_mobile()->get_user())
            fighter2->get_mobile()->get_user()->death();
        else
            fighter2->get_mobile()->death(fighter1->get_mobile()->get_user());
        if (combat_info1["hits"]) {
            fighter1->get_mobile()->get_user()->gain_exp(combat_info1["skill"], combat_info2["exp"]);
            fighter1->get_mobile()->get_user()->gain_exp("siła", combat_info2["exp"]);
        }
        if (combat_info1["shoots"]) {
            fighter1->get_mobile()->get_user()->gain_exp(combat_info1["shoot_skill"], combat_info2["exp"]);
            fighter1->get_mobile()->get_user()->gain_exp("zręczność", combat_info2["exp"]);
        }
        fighter1->get_mobile()->get_user()->gain_exp("kondycja", combat_info2["exp"]);
        if (combat_info1["dodge"]) {
            fighter1->get_mobile()->get_user()->gain_exp("walka/uniki", combat_info2["exp"]);
            fighter1->get_mobile()->get_user()->gain_exp("zręczność", combat_info2["exp"]);
        }  
    }
}

void stop_combat()
{
    int fatigue, i, j, k, dmg, ctr;
    object *objs;
    int *tmp;
    string *item_dmg;

    remove_call_out(fighter1_call);
    remove_call_out(fighter2_call);
    TAGD->set_tag_value(fighter1, "Combat", nil);
    TAGD->set_tag_value(fighter2, "Combat", nil);
    objs = fighter1->objects_in_container();
    item_dmg = ({ });
    ctr = 0;
    for (i = 0; i < sizeof(objs); i++) {
        if (i == combat_info1["weapon_num"] && combat_info1["hits"]) {
            item_dmg += ({ "" });
            for (j = 0; j < combat_info1["hits"]; j++) {
                dmg = objs[i]->damage_item(fighter1->get_mobile()->get_user());
                if (dmg == 1)
                    item_dmg[ctr] = "Twoja broń ulega uszkodzeniu.\n";
                else if(dmg == 2) {
                    item_dmg[ctr] = "Twoja broń ulega zniszczeniu.\n";
                    break;
                }
            }
            ctr ++;
        } else if (i == combat_info1["shoot_weapon_num"] && combat_info1["shoots"]) {
            item_dmg += ({ "" });
            dmg = objs[i]->damage_item(fighter1->get_mobile()->get_user());
            if (dmg == 1)
                item_dmg[ctr] = "Twoja broń strzelecka ulega uszkodzeniu.\n";
            else if(dmg == 2) 
                item_dmg[ctr] = "Twoja broń strzelecka ulega zniszczeniu.\n";
            ctr ++;
        } else if (objs[i]->is_dressed()) {
            tmp = objs[i]->get_wearlocations();
            item_dmg += ({ "" });
            for (j = 0; j < sizeof(tmp); j++)
                for (k = 0; k < combat_info1["armor"][j][2]; k++) {
                    dmg = objs[i]->damage_item(fighter1->get_mobile()->get_user());
                    if (dmg == 1)
                        item_dmg[ctr] = objs[i]->get_brief()->to_string(fighter1->get_mobile()->get_user())
                            + " ulega uszkodzeniu.\n";
                    else if (dmg == 2) {
                        item_dmg[ctr] = objs[i]->get_brief()->to_string(fighter1->get_mobile()->get_user())
                            + " ulega zniszczeniu.\n";
                        break;
                    }
                }
            ctr++;
        }
    }
    for (i = 0; i < sizeof(item_dmg); i++) {
        if (item_dmg[i] != "")
            fighter1->get_mobile()->get_user()->message(item_dmg[i]);
    }
    if (combat_info1["hp"] > 0) {
        if (combat_info1["hp"] < fighter1->get_hp()) {
            TAGD->set_tag_value(fighter1, "Hp", combat_info1["hp"]);
            fighter1->get_mobile()->get_user()->set_health(combat_info1["hp"]);
        }
        fatigue = (fighter1->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - combat_info1["stamina"];
        if (fatigue < 0)
            fatigue = 0;
        TAGD->set_tag_value(fighter1, "Fatigue", fatigue);
        fighter1->get_mobile()->get_user()->set_condition(combat_info1["stamina"]);
    }
    if (fighter2->get_mobile()->get_user()) {
        objs = fighter2->objects_in_container();
        item_dmg = ({ });
        ctr = 0;
        for (i = 0; i < sizeof(objs); i++) {
            if (objs[i]->is_dressed()) {
                tmp = objs[i]->get_wearlocations();
                item_dmg += ({ "" });
                if (tmp[0] == 5 && combat_info2["hits"])
                    for (j = 0; j < combat_info2["hits"]; j++) {
                        dmg = objs[i]->damage_item(fighter2->get_mobile()->get_user());
                        if (dmg == 1)
                            item_dmg[ctr] = "Twoja broń ulega uszkodzeniu.\n";
                        else if(dmg == 2) {
                            item_dmg[ctr] = "Twoja broń ulega zniszczeniu.\n";
                            break;
                        }
                    }
                else {
                    for (j = 0; j < sizeof(tmp); j++)
                        for (k = 0; k < combat_info2["armor"][j][2]; k++) {
                            dmg = objs[i]->damage_item(fighter2->get_mobile()->get_user());
                            if (dmg == 1)
                                item_dmg[ctr] = objs[i]->get_brief()->to_string(fighter2->get_mobile()->get_user())
                                    + " ulega uszkodzeniu.\n";
                            else if (dmg == 2) {
                                item_dmg[ctr] = objs[i]->get_brief()->to_string(fighter2->get_mobile()->get_user())
                                    + " ulega zniszczeniu.\n";
                                break;
                            }
                        }
                }
                ctr ++;
            }
        }
        for (i = 0; i < sizeof(item_dmg); i++) 
            if (item_dmg[i] != "")
                fighter2->get_mobile()->get_user()->message(item_dmg[i]);
    }
    if (combat_info2["hp"] > 0) {
        if (combat_info2["hp"] < fighter2->get_hp())
            TAGD->set_tag_value(fighter2, "Hp", combat_info2["hp"]);
        if (fighter2->get_mobile()->get_user()) {
            if (combat_info2["hp"] < fighter2->get_hp())
                fighter2->get_mobile()->get_user()->set_health(combat_info1["hp"]);
            fatigue = (fighter2->get_mobile()->get_user()->get_stat_val("kondycja") * 10) - combat_info2["stamina"];
            if (fatigue < 0)
                fatigue = 0;
            TAGD->set_tag_value(fighter2, "Fatigue", fatigue);
            fighter2->get_mobile()->get_user()->set_condition(combat_info2["stamina"]);
        }
    }
}
