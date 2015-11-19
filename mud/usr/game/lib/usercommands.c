/* $Header: /cvsroot/phantasmal/mudlib/usr/game/obj/user.c,v 1.17 2005/08/02 21:20:01 angelbob Exp $ */

#include <kernel/kernel.h>
#include <kernel/user.h>
#include <kernel/rsrc.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/channel.h>
#include <phantasmal/map.h>
#include <phantasmal/search_locations.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

#include <gameconfig.h>


static void create(varargs int clone)
{
}

static void upgraded(varargs int clone)
{
}

static void destructed(varargs int clone)
{
}

/************** User-level commands *************************/

/*
 * NAME:	lalign()
 * DESCRIPTION:	return a string as a left-aligned string
 */
private string lalign(string str, int width)
{
    int i, diff;

    for (i = 0; i < strlen(str); i++) 
        if ((int)str[i] == 196 || (int)str[i] == 197) 
            width++;
    if (width > strlen(str)) {
        diff = width - strlen(str);
        for (i = 0; i <= diff; i++)
            str += " ";
    }
    if (strlen(str) <= width)
        width = strlen(str) - 1;
    return str[0..width];
}

static void cmd_ooc(object user, string cmd, string str) {
  if (!str || str == "") {
    user->message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  CHANNELD->chat_to_channel(CHANNEL_OOC, str);
}

static void cmd_say(object user, string cmd, string str) {
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  user->get_mobile()->say(str);
}

static void cmd_emote(object user, string cmd, string str) {
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  user->get_mobile()->emote(str);
}

static void cmd_tell(object self, string cmd, string str) {
    object user;
    string username;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s %s", username, str) != 2 ||
            !(user=self->find_user(username))) 
        self->message("Użycie: " + cmd + " <imię> <tekst>\n");
    else 
        user->message(self->get_Name() + " mówi Tobie: " + str + "\n");
}

/* Whisper to someone */
static void cmd_whisper(object self, string cmd, string str)
{
    object *user;
    string username;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s %s", username, str) != 2) {
        self->message("Użycie: " + cmd + " <imię> <tekst>\n");
        return;
    }
    user = self->find_first_objects(username, LOC_CURRENT_ROOM);
    if (!user)
        self->message("Nie ma kogoś o imieniu " + username + " w okolicy.\n");
    else
        self->get_mobile()->whisper(user[0], str);
}

/* Show info about character */
static void cmd_whoami(object user, string cmd, string str)
{
    string charinfo, skilltext, tmptext;
    mapping statsinfo, skillsinfo;
    int i, j;
    string *stattext, *statnames, *skillname, *skillsnames;

    charinfo = "Nazywasz się " + user->get_Name() + ". ";
    if (user->get_gender() == 1)
        charinfo += "Jesteś mężczyzną. ";
    else
        charinfo += "Jesteś kobietą. ";
    charinfo += "Masz " + user->get_body()->get_length() + " cm wzrostu. "
        + "Ważysz " + user->get_body()->get_weight() + " kg.\n"
        + "Jesteś " + STRINGD->to_lower(user->get_health()) + " i " + STRINGD->to_lower(user->get_condition()) + ".\n"
        + "Posiadasz " + user->get_body()->get_price() + " miedziaków.\n"
        + "Niesiesz " + user->get_body()->get_current_weight() + " kg ekwipunku na " + user->get_body()->get_weight_capacity() + " kg możliwych.\n\n"
        + "===== CECHY =====";
    statsinfo = ([ "siła": ({ "Słaby", "Przeciętny", "Dobrze zbudowany", "Siłacz" }),
            "zręczność": ({ "Niezdarny", "Przeciętny", "Zwinny", "Akrobata" }),
            "inteligencja": ({ "Niezbyt rozgarnięty", "Przeciętny", "Inteligentny", "Mistrz Umysłu" }),
            "kondycja": ({ "Chorowity", "Przeciętny", "Wysportowany", "Niezniszczalny" }) ]);
    stattext = ({ "Siła: ", "Zręczność: ", "Inteligencja: ", "Kondycja: " });
    statnames = ({ "siła", "zręczność", "inteligencja", "kondycja" });
    for (i = 0; i < sizeof(statnames); i++) {
        if (!(i % 2))
            charinfo += "\n";
        else
            charinfo += "      ";
        tmptext = stattext[i];
        if (user->get_stat_val(statnames[i]) >= 10 && user->get_stat_val(statnames[i]) < 20)
            tmptext += statsinfo[statnames[i]][0];
        else if (user->get_stat_val(statnames[i]) >= 20 && user->get_stat_val(statnames[i]) < 30)
            tmptext += statsinfo[statnames[i]][1];
        else if (user->get_stat_val(statnames[i]) >= 30 && user->get_stat_val(statnames[i]) < 40)
            tmptext += statsinfo[statnames[i]][2];
        else
            tmptext += statsinfo[statnames[i]][3];
        charinfo += lalign(tmptext, 40);
    }
    charinfo += "\n\n===== UMIEJĘTNOŚCI =====";
    skillsinfo = ([ ]);
    skillsnames = user->get_skills_names();
    for (i = 0; i < sizeof(skillsnames); i++) {
        skillname = explode(skillsnames[i], "/");
        if (user->get_skill_val(skillsnames[i]) >= 1 && user->get_skill_val(skillsnames[i]) < 20)
            skilltext = "nowicjusz";
        else if (user->get_skill_val(skillsnames[i]) >= 20 && user->get_skill_val(skillsnames[i]) < 40)
            skilltext = "amator";
        else if (user->get_skill_val(skillsnames[i]) >= 40 && user->get_skill_val(skillsnames[i]) < 60)
            skilltext = "adept";
        else if (user->get_skill_val(skillsnames[i]) >= 60 && user->get_skill_val(skillsnames[i]) < 80)
            skilltext = "zaawansowany";
        else
            skilltext = "mistrz";
        if (!skillsinfo[skillname[0]])
            skillsinfo[skillname[0]] = ({ });
        skillsinfo[skillname[0]] += ({ skillname[1], skilltext });
    }
    skillsnames = map_indices(skillsinfo);
    for (i = 0; i < sizeof(skillsnames); i++) {
        charinfo += "\n                     ==== " + skillsnames[i] + " ====";
        for (j = 0; j < sizeof(skillsinfo[skillsnames[i]]); j += 2) {
            if (!(j % 4))
                charinfo += "\n";
            charinfo += lalign(skillsinfo[skillsnames[i]][j] + ": " + skillsinfo[skillsnames[i]][j + 1], 40);
        }
    }
    charinfo += "\n";
    user->message_scroll(charinfo);
}

static void cmd_look(object user, string cmd, string str) 
{
    object* tmp, *objs;
    int     ctr, index;
    float   damage, durability;
    string  msg;

    str = STRINGD->trim_whitespace(str);

    if(!user->get_location()) {
        user->message("Jesteś w pustce!\n");
        return;
    }

    if(!str || str == "") {
        user->show_room_to_player(user->get_location());
        return;
    }

    if (cmd[0] == 'p' && str == "w niebo" && 
            (user->get_location()->get_room_type() == 1 || user->get_location()->get_room_type() == 2)) {
        ctr = ZONED->get_zone_for_room(user->get_location());
        switch (ZONED->get_attribute(ctr, "weather")) {
            case "clear":
                msg = "Jest pogodnie. ";
                break;
            case "overcast":
                msg = "Niebo jest zachmurzone. ";
                break;
            case "rain":
                msg = "Pada deszcz. ";
                break;
            default:
                msg = "";
                break;
        }
        msg += WORLDD->get_hour() + WORLDD->get_date();
        user->message(msg + "\n");
        return;
    }

    if (cmd[0] != 'z') 
        sscanf(str, "na %s", str);

    if(sscanf(str, "w %s", str) || sscanf(str, "do srodka %s", str)
            || sscanf(str, "do %s", str) || sscanf(str, "pomiedzy %s", str)) {
        sscanf(str, "%d %s", index, str);
        if (!index) 
            index = 1;
        index--;
        if (index < 0) {
            user->message("W marzeniach.\n");
            return;
        }
        /* Look inside container */
        str = STRINGD->trim_whitespace(str);
        tmp = user->find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
        if(!tmp) {
            user->message("Nie znalazłeś jakiegokolwiek '" + str + "'.\n");
            return;
        }
        if (index >= sizeof(tmp)) {
            user->message("Nie ma tak dużo '" + str + "' w okolicy.\n");
            return;
        }
        if(sizeof(tmp) > 1) {
            user->message("Widzisz więcej niż jeden '" + str +"'. Wybierasz " 
                    + tmp[index]->get_brief()->to_string(user) + ".\n");
            return;
        }

        if(!tmp[index]->is_container()) {
            user->message("To nie jest pojemnik.\n");
            return;
        }

        if(!tmp[index]->is_open()) {
            user->message("Jest zamknięte.\n");
            return;
        }

        if ((!tmp[index]->get_mobile()) || (user->is_admin()))
            objs = tmp[index]->objects_in_container();
        else
            objs = nil;
        if(objs && sizeof(objs)) {
            for(ctr = 0; ctr < sizeof(objs); ctr++) {
                user->message("- " + objs[ctr]->get_brief()->to_string(user) + "\n");
            }
            user->message("-----\n");
        } else {
            user->message("Nie ma nic w " + tmp[index]->get_brief()->to_string(user) + ".\n");
        }
        return;
    }

    sscanf(str, "%d %s", index, str);
    if (!index) 
        index = 1;
    index--;
    if (index < 0) {
        user->message("W marzeniach.\n");
        return;
    }

    tmp = user->find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie widzisz żadnego '" + str + "'.\n");
        return;
    }
    if (index >= sizeof(tmp)) {
        user->message("Nie ma tak dużo '" + str + "' w okolicy.\n");
        return;
    }

    if(sizeof(tmp) > 1) 
        user->message("Więcej niż jeden taki jest tutaj. "
                + "Sprawdzasz " + tmp[index]->get_brief()->to_string(user) + ".\n\n");

    if(cmd[0] == 'z' && tmp[index]->get_examine()) 
        user->send_phrase(tmp[index]->get_examine());
    else
        user->send_phrase(tmp[index]->get_look());

    if (cmd[0] == 'z' && tmp[index]->get_mobile() && tmp[index]->get_mobile()->get_user()) {
        objs = tmp[index]->objects_in_container();
        if (objs && sizeof(objs)) {
            msg = "";
            for (ctr = 0; ctr < sizeof(objs); ctr++) {
                if (objs[ctr]->is_dressed()) {
                    if (msg == "")
                        msg = "\n\nNosi na sobie:\n";
                    msg += "- " + objs[ctr]->get_brief()->to_string(user) + "\n";
                }
            }
            if (msg != "")
                user->message(msg);
        }
    }

    if (cmd[0] == 'z' && tmp[index]->get_quality()) {
        msg = "\n------\n";
        durability = (float)tmp[index]->get_durability();
        damage = (float)tmp[index]->get_cur_durability() / durability;
        if (damage == 1.0)
            msg += "Jest w dobrym stanie. ";
        else if (damage < 1.0 && damage >= 0.8)
            msg += "Jest lekko porysowany. ";
        else if (damage < 0.8 && damage >= 0.6)
            msg += "Jest lekko podniszczony. ";
        else if (damage < 0.6 && damage >= 0.4)
            msg += "Jest uszkodzony. ";
        else if (damage < 0.4 && damage >= 0.2)
            msg += "Jest mocno uszkodzony. ";
        else
            msg += "Jest prawie zniszczony. ";
        if (durability < 100.0 && durability >= 80.0)
            msg += "Widać ślady po naprawie.";
        else if (durability < 80.0 && durability >= 60.0)
            msg += "Był już naprawiany kilka razy.";
        else if (durability < 60.0 && durability >= 40.0)
            msg += "Nosi ślady wielu napraw.";
        else if (durability < 40.0 && durability >= 20.0)
            msg += "Wygląda na zużyty.";
        else if (durability < 20.0)
            msg += "Przeszedł zbyt wiele napraw.";
        user->message(msg + "\n");
    }
    user->message("\n");
}

static void cmd_inventory(object user, string cmd, string str) 
{
    int    ctr, size, i;
    mixed* objs;
    string *inv, *weared, *wearlocations;
    string msg, money;
    int *wlocs;

    if(str && !STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    money = "\nPosiadasz " + user->get_body()->get_price() + " miedziaków.\n";
    objs = user->get_body()->objects_in_container();
    if(!objs || !sizeof(objs)) {
        user->message("Nic nie nosisz przy sobie.\n" + money);
        return;
    }
    inv = weared = ({ });
    wearlocations = ({"głowa", "tułów", "ręce", "dłonie", "nogi", "prawa dłoń", "lewa dłoń", 
            "plecy", "prawa strona pasa", "lewa strona pasa", "broń strzelecka"});
    for(ctr = 0; ctr < sizeof(objs); ctr++) {
        if (objs[ctr]->is_dressed()) {
            wlocs = objs[ctr]->get_wearlocations();
            msg = "- " + objs[ctr]->get_brief()->to_string(user) + " ( ";
            for (i = 0; i < sizeof(wlocs); i++)
                msg += wearlocations[wlocs[i]] + " ";
            msg += ")";
            weared += ({ msg });
        } else
            inv += ({ lalign("- " + objs[ctr]->get_brief()->to_string(user), 30) });
    }
    if (sizeof(weared) > sizeof(inv))
        size = sizeof(weared);
    else
        size = sizeof(inv);
    msg = lalign("=Inwentarz=", 30) + "=Założone=\n";
    for (ctr = 0; ctr < size; ctr ++) {
        if (ctr < sizeof(inv))
            msg += inv[ctr];
        else
            msg += lalign("            ", 30);
        if (ctr < sizeof(weared))
            msg += weared[ctr] + "\n";
        else
            msg += "\n";
    }
    msg += money + "Niesiesz " + user->get_body()->get_current_weight() + " kg ekwipunku na " 
        + user->get_body()->get_weight_capacity() + " kg możliwych.\n\n";
    user->message_scroll(msg);
}

static void cmd_put(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr, portindex, contindex;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <obiekt1> w <obiekt2>\n");
    return;
  }

  if(sscanf(str, "%s do %s", obj1, obj2) != 2
     && sscanf(str, "%s w %s", obj1, obj2) != 2) {
    user->message("Użycie: " + cmd + " <obiekt1> w <obiekt2>\n");
    return;
  }

  if (sscanf(obj1, "%d %s", portindex, obj1) != 2)
    {
      portindex = 0;
    }
  portindex --;
  if (portindex < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  if (sscanf(obj2, "%d %s", contindex, obj2) != 2)
    {
      contindex = 0;
    }
  contindex --;
  if (contindex < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  portlist = user->find_first_objects(obj1, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj1 + "' w okolicy.\n");
    return;
  }
  if (portindex >= sizeof(portlist))
    {
      user->message("Nie ma aż tyle '" + obj1 + "' w okolicy.\n");
      return;
    }

  contlist = user->find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if (!user->is_admin())
    {
      ctr = 0;
      while (ctr < sizeof(contlist))
	{
	  if (contlist[0]->get_mobile())
	    {
	      contlist = contlist[0..(ctr - 1)] + contlist[(ctr + 1)..];
	    }
	  ctr++;
	}
    }
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj2 + "' w okolicy.\n");
    return;
  }
  if (contindex >= sizeof(contlist))
    {
      user->message("Nie ma aż tyle '" + obj2 + "' w okolicy.\n");
      return;
    }

  if(sizeof(portlist) > 1)
    {
      if (portindex == -1)
	{
	  user->message("Jest więcej niż jeden obiekt pasujący do '" + obj1 + "'.\n");
	  portindex = 0;
	}
      user->message("Wybrałeś " + portlist[portindex]->get_brief()->to_string(user) + ".\n");
  }
  if (portindex == -1)
    {
      portindex = 0;
    }

  if(sizeof(contlist) > 1)
    {
      if (contindex == -1)
	{
	  user->message("Jest więcej niż jeden obiekt pasujący do '" + obj2 + "'.\n");
	  contindex = 0;
	}
      user->message("Wybrałeś " + contlist[contindex]->get_brief()->to_string(user) + ".\n");
  }
  if (contindex == -1)
    {
      contindex = 0;
    }

  port = portlist[portindex];
  cont = contlist[contindex];

  if (port->is_dressed())
    {
      port->set_dressed(0);
      user->message("Zdejmujesz " + port->get_brief()->to_string(user) + ".\n");
    }

  if (!(err = user->get_mobile()->place(port, cont))) {
    user->message("Wkładasz ");
    user->send_phrase(port->get_brief());
    user->message(" do ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }

}

static void cmd_remove(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr, portindex, contindex;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <obiekt1> z <obiekt2>\n");
    return;
  }

  if(sscanf(str, "%s ze srodka %s", obj1, obj2) != 2
     && sscanf(str, "%s z wnetrza %s", obj1, obj2) != 2
     && sscanf(str, "%s ze %s", obj1, obj2) != 2
     && sscanf(str, "%s z %s", obj1, obj2) != 2) {
    user->message("Użycie: " + cmd + " <obiekt1> z <obiekt2>\n");
    return;
  }

  if (sscanf(obj1, "%d %s", portindex, obj1) != 2)
    {
      portindex = 0;
    }
  portindex --;
  if (portindex < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  if (sscanf(obj2, "%d %s", contindex, obj2) != 2)
    {
      contindex = 0;
    }
  contindex --;
  if (contindex < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  contlist = user->find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj2 + "' w okolicy.\n");
    return;
  }
  if (contindex >= sizeof(contlist))
    {
      user->message("Nie ma aż tyle '" + obj2 + "' w okolicy.\n");
      return;
    }

  if(sizeof(contlist) > 1)
    {
      if (contindex == -1)
	{
	  user->message("Jest więcej niż jeden obiekt pasujący do '" + obj2 + "'.\n");
	  contindex = 0;
	}
      user->message("Wybrałeś " + contlist[contindex]->get_brief()->to_string(user) + ".\n");
    }
  if (contindex == -1)
    {
      contindex = 0;
    }
  cont = contlist[contindex];

  portlist = cont->find_contained_objects(user, obj1);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj1 + "' w ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
    return;
  }
  if (portindex >= sizeof(portlist))
    {
      user->message("Nie ma aż tyle '" + obj1 + "' w okolicy.\n");
      return;
    }

  if(sizeof(portlist) > 1)
    {
      if (portindex == -1)
	{
	  user->message("Jest więcej niż jeden obiekt pasujący do '" + obj1 + "'.\n");
	  portindex = 0;
	}
      user->message("Wybrałeś " + portlist[portindex]->get_brief()->to_string(user) + ".\n");
  }
  if (portindex == -1)
    {
      portindex = 0;
    }
  port = portlist[portindex];

  if (!(err = user->get_mobile()->place(port, user->get_body()))) {
    user->message("Wziąłeś ");
    user->send_phrase(port->get_brief());
    user->message(" z ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }
}


static void cmd_users(object user, string cmd, string str) {
    int i, sz;
    object* users;
    string name_idx;

    users = users();
    user->message("Zalogowani: \n");
    str = "";
    for (i = 0, sz = sizeof(users); i < sz; i++) {
        name_idx = "* " + users[i]->query_name();
        if (name_idx) {
            if (users[i]->is_admin())
                name_idx += " (Opiekun)";
            str += lalign(name_idx, 25) + lalign("Bezczynny(a): " + users[i]->get_idle_time() + " sekund", 25) + "\n";
        }
    }
    user->message(str + "\n");
}

/* List available social commands */
static void cmd_socials(object user, string cmd, string str)
{
    int i, sz;
    string* scommands;

    user->message("Dostępne komendy socjalne:\n");
    scommands = SOULD->all_socials();
    str = "";
    for (i = 0, sz = sizeof(scommands); i < sz; i++) {
        if (!(i % 5))
            str += "\n";
        str += lalign(scommands[i], 20);
    }
    user->message_scroll(str + "\n");
}

/* Stop following */
void stop_follow(object user)
{
    object target;

    target = MAPD->get_room_by_num(TAGD->get_tag_value(user->get_body(), "Follow"));
    TAGD->set_tag_value(user->get_body(), "Follow", nil);
    if (target->get_mobile()->get_user())
        target->get_mobile()->get_user()->message(user->get_Name() + " przestał za Tobą podążać.\n");
    user->message("Przestałeś podążać za " + target->get_brief()->to_string(user) + ".\n");
}

static void cmd_movement(object user, string cmd, string str) {
    int    dir, fatigue, exp;
    string reason;
    float capacity;

    if (TAGD->get_tag_value(user->get_body(), "Fatigue"))
        fatigue = TAGD->get_tag_value(user->get_body(), "Fatigue");
    else
        fatigue = 0;

    /* Currently, we ignore modifiers (str) and just move */

    dir = EXITD->direction_by_string(cmd);
    if(dir == -1) {
        user->message("'" + cmd + "' nie wygląda na poprawny kierunek.\n");
        return;
    }

    if (reason = user->get_mobile()->move(dir)) {
        user->message(reason + "\n");
        return;
    }

    capacity = user->get_body()->get_current_weight() / user->get_body()->get_weight_capacity();
    if (capacity <= 0.5) {
        fatigue++;
        exp = 1;
    } else if(capacity > 0.5 && capacity <= 0.75) {
        fatigue += 2;
        exp = 2;
    } else if(capacity > 0.75 && capacity <= 0.95) {
        fatigue += 5;
        exp =5;
    } else {
        fatigue += 10;
        exp = 10;
    }
    if (fatigue > (user->get_stat_val("kondycja") * 10))
        fatigue = user->get_stat_val("kondycja") * 10;
    TAGD->set_tag_value(user->get_body(), "Fatigue", fatigue);
    user->gain_exp("kondycja", exp);
    user->set_condition((user->get_stat_val("kondycja") * 10) - fatigue);

    if (TAGD->get_tag_value(user->get_body(), "Combat")) 
        user->end_combat();
    if (TAGD->get_tag_value(user->get_body(), "Follow"))
        stop_follow(user);

    user->show_room_to_player(user->get_location());
}

/* This one is special, and is called specially... */
static void cmd_social(object user, string cmd, string str) 
{
    object* targets;
    int index;

    if(!SOULD->is_social_verb(cmd)) {
        user->message(cmd + " nie wygląda na poprawną komendę socjalną.\n");
        return;
    }
    
    if (!str || STRINGD->trim_whitespace(str) == "") {
        user->get_mobile()->social(cmd, nil);
        return;
    }

    if (sscanf(str, "%d %s", index, str) != 2)
        index = 0;
    index --;
    if (index < -1) {
        user->message("W marzeniach.\n");
        return;
    }
    targets = user->get_location()->find_contained_objects(user, str);
    if(!targets) {
        user->message("Nie możesz znaleźć żadnego '" + str + "' w okolicy.\n");
        return;
    }
    if (index >= sizeof(targets)) {
        user->message("Nie ma aż tyle '" + str + "' w okolicy.\n");
        return;
    }
    if (sizeof(targets) > 1) {
        if (index == -1) {
            user->message("Więcej niż jedna taka osoba jest w okolicy.\n");
            index = 0;
        }
        user->message("Wybierasz " + targets[index]->get_brief()->to_string(user) + "\n");

    }
    if (index == -1)
        index = 0;
    user->get_mobile()->social(cmd, targets[index]);
}

static void cmd_get(object user, string cmd, string str) {
  object* tmp;
  string err;
  int index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <opis w pomocy>\n");
    return;
  }

  if(sscanf(str, "%*s z wnetrza %*s") == 2
     || sscanf(str, "%*s ze srodka %*s") == 2
     || sscanf(str, "%*s z %*s") == 2
     || sscanf(str, "%*s ze %*s") == 2) {
    cmd_remove(user, cmd, str);
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  tmp = user->find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      user->message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }

  if(sizeof(tmp) > 1)
    {
      if (index == -1)
	{
	  user->message("Więcej niż jedna z tych rzeczy leży w okolicy.\n");
	  index = 0;
	}
    user->message("Wybrałeś "+ tmp[index]->get_brief()->to_string(user) + ".\n");
  }
  if (index == -1)
    {
      index = 0;
    }

  if(tmp[index] == user->get_location()) {
    user->message("Nie możesz tego zabrać. Znajdujesz się wewnątrz tego.\n");
    return;
  }

  if(tmp[index]->get_detail_of()) {
    user->message("Nie możesz tego wziąć. Jest częścią ");
    user->send_phrase(tmp[index]->get_detail_of()->get_brief());
    user->message(".\n");
    return;
  }

  if(!(err = user->get_mobile()->place(tmp[index], user->get_body())))
    {
      user->message("Bierzesz " + tmp[index]->get_brief()->to_string(user) + ".\n");
      TAGD->set_tag_value(tmp[index], "DropTime", nil);
    }
  else
    {
      user->message(err + "\n");
    }
}

static void cmd_drop(object user, string cmd, string str)
{
  object* tmp;
  string err;
  int index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  tmp = user->find_first_objects(str, LOC_INVENTORY, LOC_BODY);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie niesiesz ze sobą '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      user->message("Nie masz aż tyle '" + str + "' ze sobą.\n");
      return;
    }

  if(sizeof(tmp) > 1)
    {
      if (index == -1)
	{
	  user->message("Masz więcej niż jedną taką rzecz.\n");
	  index = 0;
	}
      user->message("Wybierasz " + tmp[index]->get_brief()->to_string(user) + ".\n");
    }
  if (index == -1)
    {
      index = 0;
    }

  if (tmp[index]->is_dressed())
    {
      tmp[index]->set_dressed(0);
      user->message("Zdejmujesz " + tmp[index]->get_brief()->to_string(user) + ".\n");
    }

  if (!(err = user->get_mobile()->place(tmp[index], user->get_location())))
    {
      user->message("Upuszczasz " + tmp[index]->get_brief()->to_string(user) + ".\n");
      TAGD->set_tag_value(tmp[index], "DropTime", time());
    }
  else
    {
      user->message(err + "\n");
    }
}

static void cmd_open(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr, index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }

  tmp = user->find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      user->message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }

  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if(tmp[ctr]->is_openable())
	    break;
	}
      if(ctr >= sizeof(tmp))
	{
	  user->message("Żadne z wybranych nie może być otwarte.\n");
	  return;
	}
      if (index == -1)
	{
	  user->message("Więcej niż jedno takie jest w okolicy.\n");
	}
      else
	{
	  ctr = index;
	}
      user->message("Wybierasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }

  if(!tmp[ctr]->is_openable()) {
    user->message("Nie możesz tego otworzyć!\n");
    return;
  }

  if(!(err = user->get_mobile()->open(tmp[ctr]))) {
    user->message("Otwierasz ");
    user->send_phrase(tmp[0]->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }
}

static void cmd_close(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr, index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      user->message("W marzeniach.\n");
      return;
    }
  
  tmp = user->find_first_objects(str, LOC_CURRENT_ROOM, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      user->message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }
  
  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if(tmp[ctr]->is_openable())
	    break;
	}
      if(ctr >= sizeof(tmp))
	{
	  user->message("Żadne z tych nie może być zamknięte.\n");
	  return;
	}
      if (index == -1)
	{
	  user->message("Więcej niż jedno takie jest w okolicy.\n");
	}
      else
	{
	  ctr = index;
	}
      user->message("Wybierasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  }

  if(!tmp[ctr]->is_openable()) {
    user->message("Nie możesz tego zamknąć!\n");
    return;
  }

  if(!(err = user->get_mobile()->close(tmp[ctr]))) {
    user->message("Zamknąłeś ");
    user->send_phrase(tmp[0]->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }
}

/* Show list of available commands */
static void cmd_commands(object user, string cmd, string str)
{
  string msg;
  int i;
  mixed indices;

  /* Standard commands */
  msg = "Dostępne komendy:";
  indices = user->get_commands() + ({ "wyloguj" });
  for (i = 0; i < sizeof(indices); i++) {
      if (!(i % 5))
          msg += "\n";
      msg += lalign(indices[i], 20);
    }
  msg += "\n\n";
  /* Admin commands */
  if (user->is_admin()) {
      mixed* admin_commands;
      msg += "Komendy administracyjne:";
      admin_commands = SYSTEM_WIZTOOL->get_command_sets(this_object());
      indices = map_indices(admin_commands[0]);
      for (i = 0; i < sizeof(indices); i++) {
          if (!(i % 5))
              msg += "\n";
    	  msg += lalign(indices[i], 20);
	    }
      msg += "\n";
    }
  user->message_scroll(msg);
}

/* Wear weapons, armors, etc */
static void cmd_wear(object user, string cmd, string str)
{
    object* tmp;
    int ctr, i;
    mixed* items;

    if(str)
        str = STRINGD->trim_whitespace(str);
    if(!str || str == "") {
        user->message("Użycie: " + cmd + " <obiekt>\n");
        return;
    }
    tmp = user->find_first_objects(str, LOC_INVENTORY);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
        return;
    }
    ctr = 0;
    if(sizeof(tmp) > 1) {
        for(ctr = 0; ctr < sizeof(tmp); ctr++)
            if (tmp[ctr]->is_wearable() && !tmp[ctr]->is_dressed())
                break;
        if(ctr >= sizeof(tmp)) {
            user->message("Żadna z tych rzeczy nie może być założona.\n");
            return;
        }
        user->message("Więcej niż jedna taka rzecz jest w ekwipunku.\n");
        user->message("Wybierasz ");
        user->send_phrase(tmp[ctr]->get_brief());
        user->message(".\n");
    }
    if(!tmp[ctr]->is_wearable()) {
        user->message("Nie możesz tego założyć!\n");
        return;
    }
    if (tmp[ctr]->is_dressed()) {
        user->message("Ten obiekt jest już założony!\n");
        return;
    }
    items = user->get_body()->objects_in_container();
    if (tmp[ctr]->get_wearlocations()[0] == 5) {
        for (i = 0; i < sizeof(items); i++) {
            if (items[i]->is_wearable() && items[i]->get_wearlocations()[0] == 5 && items[i]->is_dressed()) {
                user->message("Chowasz " + items[i]->get_brief()->to_string(user) + ".\n");
                items[i]->set_dressed(0);
                break;
            }
        }
        user->message("Bierzesz " + tmp[ctr]->get_brief()->to_string(user) + " do ręki.\n");
    }
    else {
        for (i = 0; i < sizeof(items); i++) {
            if (items[i]->is_wearable() && items[i]->get_wearlocations()[0] != 5 && items[i]->is_dressed()) {
                if (sizeof(tmp[ctr]->get_wearlocations() - items[i]->get_wearlocations()) < sizeof(tmp[ctr]->get_wearlocations())) {
                    user->message("Zdejmujesz " + items[i]->get_brief()->to_string(user) + ".\n");
                    items[i]->set_dressed(0);
                }
            }
        }
        user->message("Zakładasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }
    tmp[ctr]->set_dressed(1);
}

/* Take off equipment */
static void cmd_takeoff(object user, string cmd, string str)
{
  object* tmp;
  int ctr, i;
  mixed* items;
  
  if(str)
    {
      str = STRINGD->trim_whitespace(str);
    }
  if(!str || str == "")
    {
      user->message("Użycie: " + cmd + " <obiekt>\n");
      return;
    }
  tmp = user->find_first_objects(str, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp))
    {
      user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
      return;
    }
  ctr = 0;
  if(sizeof(tmp) > 1) {
      for(ctr = 0; ctr < sizeof(tmp); ctr++) {
	  if (tmp[ctr]->is_wearable() && tmp[ctr]->is_dressed())
	      break;
      }
      if(ctr >= sizeof(tmp))
      {
          user->message("Żadna z tych rzeczy nie może być zdjęta.\n");
          return;
      }
  }
  if (!tmp[ctr]->is_dressed())
    {
      user->message("Ten obiekt nie jest założony!\n");
      return;
    }
  items = user->get_body()->objects_in_container();
  if (tmp[ctr]->get_wearlocations()[0] == 5)
      user->message("Chowasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  else
      user->message("Zdejmujesz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  tmp[ctr]->set_dressed(0);
}

/* Start combat */
static void cmd_attack(object user, string cmd, string str)
{
    object *tmp, *inv;
    string target;
    int number, i, shoot;
    string *nouns;

    if(str)
        str = STRINGD->trim_whitespace(str);
    if(!str || str == "") {
        user->message("Użycie: " + cmd + " <obiekt>\n");
        return;
    }
    if (sscanf(str, "%d %s", number, target) != 2) {
        target = str;
        number = 1;
    }
    tmp = user->find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (sizeof(tmp) > 1 && !number) {
        user->message("Jest kilka '" + target + "' w okolicy. Wybierz dokładnie którego chcesz zaatakować.\n");
        return;
    }
    if (number > sizeof(tmp)) {
        user->message("Nie możesz znaleźć tego '" + target + "'.\n");
        return;
    }
    number--;
    if (number < 0) {
        user->message("W marzeniach\n");
        return;
    }
    if (!tmp[number]->get_mobile()) {
        user->message("Możesz atakować tylko istoty. Nie wyżywaj się na sprzęcie.\n");
        return;
    }
    if (!tmp[number]->get_mobile()->get_parentbody()) {
        user->message("Nie możesz zaatakować tej istoty.\n");
        return;
    }
    if (TAGD->get_tag_value(user->get_body(), "Combat") || TAGD->get_tag_value(tmp[number], "Combat")) {
        user->message("Któreś z Was już walczy. Dokończcie najpierw jedną walkę.\n");
        return;
    }
    if (TAGD->get_tag_value(user->get_body(), "Follow"))
        stop_follow(user);
    shoot = -1;
    if (cmd == "strzel") {
        inv = user->get_body()->objects_in_container();
        for (i = 0; i < sizeof(inv); i++) {
            if (inv[i]->is_dressed() && sizeof(inv[i]->get_wearlocations() & ({10}))) {
                shoot = i;
                break;
            }
        }
        if (shoot == -1) {
            user->message("Nie masz założonej jakiejkolwiek broni strzeleckiej.\n");
            return;
        }
        if (!sizeof(inv[shoot]->get_cur_magazine())) {
            user->message("Twoja broń strzelecka nie jest załadowana.\n");
            return;
        }
    }
    nouns = tmp[number]->get_nouns(user->get_locale());
    if (sizeof(nouns) < 5)
        user->message("Atakujesz " + tmp[number]->get_brief()->to_string(user) + "...\n");
    else
        user->message("Atakujesz " + nouns[3] + "...\n");
    user->begin_combat(tmp[number], shoot);
}

/* Report bug, typo or propose idea */
static void cmd_report(object user, string cmd, string str)
{
    string filename, rtype;

    switch (cmd) {
        case "bug":
            filename = "/usr/game/text/bug_reports.txt";
            rtype = "błędu";
            break;
        case "literowka":
            filename = "/usr/game/text/typo_reports.txt";
            rtype = "literówki";
            break;
        case "idea":
            filename = "/usr/game/text/idea_reports.txt";
            rtype = "pomysłu";
            break;
        default:
            user->message("Ups, coś poszło nie tak.\n");
            return;
    }
    if (!str || str == "") {
        user->message("Opis " + rtype + ". Postaraj się podać jak najwięcej szczegółów,\n"
                + "to pomoże nam w pracy.\n");
        user->push_new_state(US_REPORT_BUG, cmd, user);
        user->push_new_state(US_ENTER_DATA);
        return;
    }
    else
        write_file(filename, user->get_Name() + ": " + str + "\n");
    user->message("Dziękujemy za zgłoszenie " + rtype  + ".\n");
}

/* Player settings - description, email and password change */
static void cmd_settings(object user, string cmd, string str)
{
    string *parts;
    string oldmail;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        user->message("Użycie: " + cmd + " [opis|mail|haslo|odmiana]\n");
        return;
    }
    parts = explode(str, " ");
    switch (parts[0]) {
        case "opis":
            user->message("Ustawiasz nowy opis postaci.\n");
            user->push_new_state(US_SET_DESC, user);
            user->push_new_state(US_ENTER_DATA);
            break;
        case "mail":
            oldmail = user->get_email();
            if (oldmail == "")
                oldmail = "brak adresu";
            if (sizeof(parts) < 2) 
                parts = ({ "mail", "" });
            user->set_email(parts[1]);
            if (parts[1] == "")
                parts[1] = "brak adresu";
            user->message("Zmieniono adres mailowy z "+ oldmail + " na " + parts[1] + "\n");
            break;
        case "odmiana":
            user->push_new_state(US_ENTER_CONJ, user);
            break;
        default:
            user->message("Nieznana opcja, spróbuj " + cmd + " [opis|mail|haslo|odmiana]\n");
            break;
    }
}

/* Follow other player/mobile */
static void cmd_follow(object user, string cmd, string str)
{
    object *tmp;
    string target;
    int number;
    object leader;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        user->message("Użycie: " + cmd + " <cel> lub [przestan]\n");
        return;
    }
    
    if (str == "przestan" && TAGD->get_tag_value(user->get_body(), "Follow")) {
        stop_follow(user);
        return;
    }
    if (sscanf(str, "%d %s", number, target) != 2) {
        target = str;
        number = 1;
    }
    tmp = user->find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (sizeof(tmp) > 1 && !number) {
        user->message("Jest kilka '" + target + "' w okolicy. Wybierz dokładnie za kim chcesz podążać.\n");
        return;
    }
    if (number > sizeof(tmp)) {
        user->message("Nie możesz znaleźć tego '" + target + "'.\n");
        return;
    }
    number--;
    if (number < 0) {
        user->message("W marzeniach.\n");
        return;
    }
    if (!tmp[number]->get_mobile()) {
        user->message("Możesz podążać tylko za żywymi istotami. Martwa natura nie porusza się.\n");
        return;
    }
    TAGD->set_tag_value(user->get_body(), "Follow", tmp[number]->get_number());
    user->message("Zaczynasz podążać za " + tmp[number]->get_brief()->to_string(user) + "\n");
    if (tmp[number]->get_mobile()->get_user())
        tmp[number]->get_mobile()->get_user()->message(user->get_Name() + " zaczyna podążać za Tobą.\n");
}

/* Temporary command for packages delivery */
static void cmd_give(object user, string cmd, string str)
{
    string item, target;
    object *tmp, *items;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s %s", item, target) != 2) {
        user->message("Użycie: " + cmd + " paczka <cel>\n");
        return;
    }
    
    items = user->find_first_objects(item, LOC_IMMEDIATE_INVENTORY);
    if (!items || !sizeof(items)) {
        user->message("Nie masz takiego przedmiotu jak '" + item + "'\n");
        return;
    }
    tmp = user->find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (tmp[0]->get_number() != TAGD->get_tag_value(items[0], "Recipient")) {
        user->message("Nie możesz dać tego przedmiotu akurat tej osobie.\n");
        return;
    }
    user->get_body()->set_price(user->get_body()->get_price() + 2);
    items[0]->get_location()->remove_from_container(items[0]);
    destruct_object(items[0]);
    user->delete_cmd("daj");
    user->message("Przekazałeś paczkę odbiorcy i dostałeś 2 miedziaki.\n");
}

/* Show current quest info and old quests */
static void cmd_quests(object user, string cmd, string str)
{
    string msg;
    string *questinfo;
    int i;
    
    if (TAGD->get_tag_value(user->get_body()->get_mobile(), "Quest") == nil)
        msg = "Obecnie nie uczestniczysz w jakiejkolwiek przygodzie.\n\n";
    else {
        questinfo = QUESTD->get_quest_info(TAGD->get_tag_value(user->get_body()->get_mobile(), "Quest"));
        msg = "Przygoda: " + questinfo[0] + "\n\n" + questinfo[1] + "\n\n";
    }

    if (!sizeof(user->get_quests())) {
        msg += "Nie ukończyłeś jeszcze jakiejkolwiek przygody.\n";
        user->message(msg);
        return;
    }

    msg += "Lista ukończonych przygód:\n";
    for (i = 0; i < sizeof(user->get_quests()); i++)
        msg += lalign((string)(i + 1), 5) + user->get_quests()[i] + "\n";

    user->message_scroll(msg);
}

/* Transform items in money or repair them. */
static void cmd_transform(object user, string cmd, string str)
{
    string item, target;
    int number, gain, fatigue;
    object *tmp, *items;

    if (!user->have_skill("alchemia/transformacja")) {
        user->message("Nie możesz transformować rzeczy ponieważ nie masz odpowiedniej umiejętności.\n");
        return;
    }

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s w %s", item, target) < 2) {
        user->message("Użycie: " + cmd + " <obiekt> w [zloto|nowy] \n");
        return;
    }
    if (sscanf(item, "%d %s", number, item) < 2) 
        number = 1;
    number--;
    if (number < 0) {
        user->message("W marzeniach.\n");
        return;
    }
    
    if (!target || (target != "zloto" && target != "złoto" && target != "nowy")) {
        user->message("Na razie możesz przemieniać inne rzeczy tylko w złoto bądź naprawiać.\n");
        return;
    }

    items = user->find_first_objects("kreda", LOC_INVENTORY);
    if (!items || !sizeof(items)) {
        user->message("Nie posiadasz nawet kawałka kredy przy sobie!\n");
        return;
    }

    tmp = user->find_first_objects(item, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + item + "'.\n");
        return;
    }
    if (sizeof(tmp) < number) {
        user->message("Nie możesz znaleźć aż tak wiele '" + item + "' w okolicy.\n");
        return;
    }
    if (tmp[number]->get_mobile()) {
        user->message("Nie możesz przemieniać żywych istot.\n");
        return;
    }
    if (tmp[number]->get_detail_of() || tmp[number]->get_brief()->to_string(user) == "miecz treningowy") {
        user->message("Nie możesz przemienić tego obiektu.\n");
        return;
    }
    if (sizeof(tmp[number]->objects_in_container())) {
        user->message("Jakieś rzeczy znajdują się jeszcze w '" + item + "'. Wyjmij je najpierw aby móc transformować\n" 
                + "'" + item + "'.\n");
        return;
    }
    if (TAGD->get_tag_value(user->get_body(), "Fatigue") && (TAGD->get_tag_value(user->get_body(), "Fatigue") + 10) > (user->get_stat_val("kondycja") * 10)) {
        user->message("Jesteś zbyt zmęczony aby transformować rzeczy. Odpocznij chwilę.\n");
        return;
    }
    if (sizeof(tmp) > 1) {
        user->message("Jest więcej niż jeden '" + item + "' w okolicy.\n"
                + "Wybierasz " + tmp[number]->get_brief()->to_string(user) + ".\n");
    }

    if (target == "zloto" || target == "złoto") {
        user->message("Wyciągasz krędę i rysujesz na ziemi odpowiedni wzór alchemiczny.\n"
                + "Po pewnym czasie kończysz i przykładasz ręce w odpowiednim punkcie,\n"
                + "napełniając wzór mocą. Przez chwilę świeci on jasnym światłem a następnie\n"
                + "znika wraz z " + item + " zamiast tego znajdujesz na ziemi monety.\n"
                + "Szybko zbierasz je i chowasz do sakiewki.\n");
        if (tmp[number]->get_combat_rating()) {
            gain = tmp[number]->get_combat_rating();
            if (gain > user->get_skill_val("alchemia/transformacja"))
                gain = user->get_skill_val("alchemia/transformacja");
        } else
            gain = 1;
        user->get_body()->set_price(user->get_body()->get_price() + gain);
        user->get_location()->remove_from_container(tmp[number]);
        destruct_object(tmp[number]);
        user->message("Zdobywasz " + gain + " miedziaków.\n");
    }
    else if (target == "nowy") {
        if (tmp[number]->get_cur_durability() == tmp[number]->get_durability()) {
            user->message(item + " jest w doskonałym stanie, nie potrzebuje naprawy.\n");
            return;
        }
        if (tmp[number]->get_craft_skill() == "") {
            user->message("Nie można naprawiać " + item + ".\n");
            return;
        }
        user->message("Wyciągasz kredę i rysujesz na ziemi odpowiedni wzór alchemiczny.\n"
                + "Po pewnym czasie kończysz i przykładasz ręce w odpowiednim punkcie,\n"
                + "napełniając wzór mocą. Przez chwilę świeci on jasnym światłem a następnie\n"
                + "znika, zostawiając na ziemi naprawiony " + item + "\n");
        if (random(user->get_skill_val("alchemia/transformacja")) < (100 - tmp[number]->get_quality())) 
            tmp[number]->set_durability(tmp[number]->get_durability() - 3);
        else
            tmp[number]->set_durability(tmp[number]->get_durability() - 1);
        tmp[number]->set_cur_durability(tmp[number]->get_durability());
        gain = 5;
    }
    user->gain_exp("alchemia/transformacja", gain);
    user->gain_exp("kondycja", gain);
    user->gain_exp("inteligencja", gain);
    if (TAGD->get_tag_value(user->get_body(), "Fatigue"))
        fatigue = TAGD->get_tag_value(user->get_body(), "Fatigue");
    else
        fatigue = 0;
    if (TAGD->get_tag_value(user->get_body(), "Fatigue"))
        fatigue = TAGD->get_tag_value(user->get_body(), "Fatigue") + 10;
    else
        fatigue = 10;
    TAGD->set_tag_value(user->get_body(), "Fatigue", fatigue);
    user->set_condition((user->get_stat_val("kondycja") * 10) - fatigue);
    number = items[0]->damage_item(user);
    if (number == 1)
        user->message("Wypisujesz nieco kredę.\n");
    else if (number == 2)
        user->message("Zużywasz cały kawałek kredy.\n");
}

/* Set aliases for commands. */
static void cmd_aliases(object user, string cmd, string str)
{
    string msg;
    string *indices, *parts;
    int i;

    msg = "";
    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        if (!map_sizeof(user->get_aliases())) 
            msg += "Nie masz jeszcze zdefiniowanych jakichkolwiek aliasów.\n";
        else {
            indices = map_indices(user->get_aliases());
            msg += "Lista aliasów:\n";
            for (i = 0; i < sizeof(indices); i++)
                msg += "- " + indices[i] + ": " + "'" + user->get_aliases()[indices[i]] + "'" + "\n";
        }
        msg += "\nAby dodać nowy alias, użyj komendy alias [nazwa] [komenda], aby usunąć istniejący\n"
            + "użyj komendy alias [nazwa].\n";
    } else {
        parts = explode(str, " ");
        if (sizeof(parts) == 1) {
            if (map_indices(user->get_aliases()) & ({ parts[0] })) {
                user->set_alias(parts[0], nil);
                msg += "Usunąłeś alias '" + parts[0] + "'.\n";
            } else
                msg += "Nie masz ustawionego aliasu '" + parts[0] + "'.\n";
        } else {
            user->set_alias(parts[0], implode(parts[1..], " "));
            msg += "Ustawiłeś alias '" + parts[0] + "' na '" + implode(parts[1..], " ") + "'.\n";
        }
    }

    user->message(msg);
}

/* Repair damaged items */
static void cmd_repair(object user, string cmd, string str)
{
    string iname;
    object *tmp, *tools;
    int number, rnd, fatigue;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s", iname) < 1) {
        user->message("Użycie: " + cmd + " <przedmiot do naprawy>\n");
        return;
    }
    if (sscanf(iname, "%d %s", number, iname) < 2)
        number = 1;
    number--;
    if (number < 0) {
        user->message("W marzeniach.\n");
        return;
    }

    tools = user->find_first_objects("narzędzia", LOC_INVENTORY);
    if (!tools || !sizeof(tools)) {
        user->message("Nie posiadasz narzędzi przy sobie!\n");
        return;
    }

    tmp = user->find_first_objects(iname, LOC_CURRENT_ROOM, LOC_INVENTORY);
    if (!tmp || !sizeof(tmp)) {
        user->message("Nie możesz znaleźć jakiegokolwiek '" + iname + "' w okolicy!\n");
        return;
    }
    if (number > sizeof(tmp)) {
        user->message("Nie ma aż tak dużo '" + iname + "' w okolicy.\n");
        return;
    }
    iname = tmp[number]->get_brief()->to_string(user);
    if (sizeof(tmp) > 1) 
        user->message("Wybierasz " + iname + ".\n");

    if (tmp[number]->get_cur_durability() == tmp[number]->get_durability()) {
        user->message(iname + " jest w doskonałym stanie, nie potrzebuje naprawy.\n");
        return;
    }
    if (tmp[number]->get_craft_skill() == "") {
        user->message("Nie można naprawiać " + iname + ".\n");
        return;
    }

    if (TAGD->get_tag_value(user->get_body(), "Fatigue") && (TAGD->get_tag_value(user->get_body(), "Fatigue") + 10) > (user->get_stat_val("kondycja") * 10)) {
        user->message("Jesteś zbyt zmęczony aby naprawiać rzeczy. Odpocznij chwilę.\n");
        return;
    }

    if (user->have_skill(tmp[number]->get_craft_skill()))
        rnd = random(user->get_skill_val(tmp[number]->get_craft_skill()));
    else
        rnd = 1;
    if (rnd < (100 - tmp[number]->get_quality())) 
        tmp[number]->set_durability(tmp[number]->get_durability() - 3);
    else
        tmp[number]->set_durability(tmp[number]->get_durability() - 1);
    tmp[number]->set_cur_durability(tmp[number]->get_durability());
    user->gain_exp(tmp[number]->get_craft_skill(), 5);
    user->gain_exp("kondycja", 5);
    user->gain_exp("siła", 5);
    user->gain_exp("zręczność", 5);
    if (TAGD->get_tag_value(user->get_body(), "Fatigue"))
        fatigue = TAGD->get_tag_value(user->get_body(), "Fatigue") + 10;
    else
        fatigue = 10;
    TAGD->set_tag_value(user->get_body(), "Fatigue", fatigue);
    user->set_condition((user->get_stat_val("kondycja") * 10) - fatigue);
    user->message("Pracowałeś przez jakiś czas i naprawiłeś " + iname + ".\n");
    number = tools[0]->damage_item(user);
    if (number == 1)
        user->message("Twoje narzędzia ulegają uszkodzeniu.\n");
    else if (number == 2)
        user->message("Twoje narzędzia ulegają zniszczeniu.\n");
}

/* Reload shooting weapon */
static void cmd_reload(object user, string cmd, string str)
{
    object *tmp, *inv, *parents;
    int i, diff, ammo, j;
    object weapon;
    string *magazine;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        user->message("Użycie: " + cmd + " <nazwa amunicji>\n");
        return;
    }

    if (TAGD->get_tag_value(user->get_body(), "Combat")) {
        user->message("Nie możesz przeładowywać w trakcie walki.\n");
        return;
    }

    tmp = user->find_first_objects(str, LOC_INVENTORY);
    if (!tmp || !sizeof(tmp)) {
        user->message("Nie masz amunicji '" + str + "' w ekwipunku.\n");
        return;
    }

    inv = user->get_body()->objects_in_container();
    for (i = 0; i < sizeof(inv); i++) {
        if (inv[i]->is_dressed() && sizeof(inv[i]->get_wearlocations() & ({10}))) {
            weapon = inv[i];
            break;
        }
    }
    if (!weapon) {
        user->message("Nie masz założonej żadnej broni strzeleckiej.\n");
        return;
    }
    ammo = 0;
    parents = tmp[0]->get_archetypes();
    for (i = 0; i < sizeof(parents); i++) {
        if (parents[i]->get_number() == weapon->get_ammo()) {
            ammo = 1;
            break;
        }
    }
    if (!ammo) {
        user->message("Amunicja '" + str + "' nie pasuje do założonej broni strzeleckiej.\n");
        return;
    }
    magazine = weapon->get_cur_magazine();
    diff = weapon->get_magazine() - sizeof(magazine);
    if (!diff) {
        user->message(weapon->get_brief()->to_string(user) + " jest już załadowany.\n");
        return;
    }

    for (i = 0; i < sizeof(inv); i++) {
        parents = inv[i]->get_archetypes();
        for (j = 0; j < sizeof(parents); j++) {
            if (parents[j]->get_number() == weapon->get_ammo()) {
                magazine += ({ inv[i]->get_damage_type() + ":" + inv[i]->get_damage() });
                user->get_body()->remove_from_container(inv[i]);
                destruct_object(inv[i]);
                diff --;
                break;
            }
        }
        if (!diff)
            break;
    }
    weapon->set_cur_magazine(magazine);
    user->message("Załadowałeś " + weapon->get_brief()->to_string(user) + ".\n");
}
