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

inherit PHANTASMAL_USER;

/* Duplicated in PHANTASMAL_USER */
#define STATE_NORMAL            0
#define STATE_LOGIN             1
#define STATE_OLDPASSWD         2
#define STATE_NEWPASSWD1        3
#define STATE_NEWPASSWD2        4

static mapping commands_map;

/* Prototypes */
       void upgraded(varargs int clone);
static void cmd_social(object user, string cmd, string str);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

/* This is the mobile we'll use for the user. */
#define USER_MOBILE "/usr/common/obj/user_mobile"

int meat_locker_rn;
int current_room;

/*
 * NAME:	create()
 * DESCRIPTION:	initialize user object
 */
static void create(int clone)
{
  ::create(clone);

  meat_locker_rn = 1;
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    ::upgraded(clone);

    commands_map = ([
		     "mow"       : "cmd_say",
		     "emo"       : "cmd_emote",

		     "pn"         : "cmd_movement",
		     "pd"         : "cmd_movement",
		     "w"          : "cmd_movement",
		     "z"          : "cmd_movement",
		     "pdw"        : "cmd_movement",
		     "pdz"        : "cmd_movement",
		     "pnw"        : "cmd_movement",
		     "pnz"        : "cmd_movement",
		     "g"          : "cmd_movement",
		     "d"          : "cmd_movement",

		     "polnoc"           : "cmd_movement",
		     "poludnie"         : "cmd_movement",
		     "wschod"           : "cmd_movement",
		     "zachod"           : "cmd_movement",
		     "polnocnyzachod"   : "cmd_movement",
		     "poludniowyzachod" : "cmd_movement",
		     "polnocnywschod"   : "cmd_movement",
		     "poludniowywschod" : "cmd_movement",
		     "gora"             : "cmd_movement",
		     "dol"              : "cmd_movement",
		     "dosrodka"         : "cmd_movement",
		     "nazewnatrz"       : "cmd_movement",

		     "pomoc"     : "cmd_help",
		     "kto"       : "cmd_users",
		     "omnie"     : "cmd_whoami",
		     "bug"       : "cmd_bug",
		     "literowka" : "cmd_typo",
		     "idea"      : "cmd_idea",
		     "powiedz"   : "cmd_tell",
		     "szepnij"   : "cmd_whisper",
		     "pp"        : "cmd_ooc",
		     "socjalne"  : "cmd_socials",
		     "komendy"   : "cmd_commands",

		     "kanal"     : "cmd_channels",
		     "kanaly"    : "cmd_channels",

		     "p"         : "cmd_look",
		     "patrz"     : "cmd_look",
		     "zb"        : "cmd_look",
		     "zba"       : "cmd_look",
		     "zbadaj"    : "cmd_look",

		     "wez"       : "cmd_get",
		     "bierz"     : "cmd_get",
		     "upusc"     : "cmd_drop",
		     "i"         : "cmd_inventory",
		     "inw"       : "cmd_inventory",
		     "inwentarz" : "cmd_inventory",
		     "wsadz"     : "cmd_put",
		     "umiesc"    : "cmd_put",
		     "wyjmij"    : "cmd_remove",
		     "otworz"    : "cmd_open",
		     "zamknij"   : "cmd_close",
		     "zaloz"     : "cmd_wear",
		     "zdejmij"   : "cmd_takeoff"

    ]);

  }
}

/****************/


/*
 * Returns true if the filename isn't allowed
 */
int filename_is_forbidden(string filename) {
  if(previous_program() != PHANTASMAL_USER)
    error("Wrong program calling filename_is_forbidden!");

  return 0;
}

/*
 * Returns true if the long name isn't allowed
 */
int name_is_forbidden(string name) {
  if(previous_program() != PHANTASMAL_USER)
    error("Wrong program calling name_is_forbidden!");

  return 0;
}


/*
 * NAME:	player_login()
 * DESCRIPTION:	Create the player body, set the account info and so on...
 */
void player_login(int first_time)
{
  int    start_room_num, start_zone, rnd;
  object start_room, other_user;

  if(previous_program() != PHANTASMAL_USER)
    error("Wrong program calling player_login!");

  body = nil;

  /* Set up location, body, etc */
  start_room_num = 100;
  start_room = MAPD->get_room_by_num(start_room_num);

  /* If start room can't be found, set the start room to the void */
  if (start_room == nil) {
    LOGD->write_syslog("Can't find the start room!  Starting in the void...");
    start_room_num = 0;
    start_room = MAPD->get_room_by_num(start_room_num);
    start_zone = 0;
    if(start_room == nil) {
      /* Panic!  No void! */
      error("Internal Error: no Void!");
    }
  } else {
    start_zone = ZONED->get_zone_for_room(start_room);
    if(start_zone < 0) {
      /* What's with this start room? */
      error("Internal Error:  no zone, not even zero, for start room!");
    }
  }

  if(body_num > 0) {
    body = MAPD->get_room_by_num(body_num);
  }

  if(body && body->get_mobile()
     && body->get_mobile()->get_user()) {
    other_user = body->get_mobile()->get_user();
  }
  if(other_user && other_user->get_name() != name) {
    LOGD->write_syslog("User is already set for this mobile!",
		       LOG_ERROR);
    message("DUP: Body and mobile files are misconfigured!  Internal error!\n");

    other_user->message(
	       "Somebody has logged in with your name and account!\n");
    other_user->message("Closing your connection now...\n");
    destruct_object(other_user);
  }

  if(!body) {
    location = start_room;
    current_room = start_room_num;

    body = clone_object(SIMPLE_ROOM);
    if(!body)
      error("Can't clone player's body!");

    body->set_container(1);
    body->set_open(1);
    body->set_openable(0);

    /* Players weigh about 80 kilograms */
    rnd = random(11);
    if (random(11) < 5)
      {
	rnd = rnd * (-1);
      }
    if (gender == 1)
      {
	body->set_weight(70.0 + (float)rnd);
      }
    else
      {
	body->set_weight(55.0 + (float)rnd);
      }
    rnd = random(11);
    if (random(11) < 5)
      {
	rnd = rnd * (-1);
      }
    if (gender == 1)
      {
	rnd = 170 + rnd;
      }
    else
      {
	rnd = 160 + rnd;
      }
    body->set_length((float)rnd);
    rnd = rnd / 10;
    body->set_volume((2.5 * (float)rnd));

    /* Players are able to lift 50 kilograms */
    body->set_weight_capacity(50.0);
    /* Players are able to carry up to 20 liters of stuff --
       that's roughly a large hiking backpack. */
    body->set_volume_capacity(20.0);
    /* Players are able to take 3m long items */
    body->set_length_capacity(300.0);

    MAPD->add_room_to_zone(body, -1, start_zone);
    if(!MAPD->get_room_by_num(body->get_number())) {
      LOGD->write_syslog("Error making new body!", LOG_ERR);
    }
    body_num = body->get_number();

    /* Set descriptions and add noun for new name */
    body->set_brief(NEW_PHRASE(Name));
    body->set_look(NEW_PHRASE(Name + " stoi tutaj."));
    body->set_examine(nil);
    body->add_noun(NEW_PHRASE(STRINGD->to_lower(Name)));

    /* Can't just clone mobile here, it causes problems later */
    mobile = MOBILED->clone_mobile_by_type("user");
    if(!mobile)
      error("Can't clone mobile of type 'user'!");
    MOBILED->add_mobile_number(mobile, -1);
    mobile->assign_body(body);
    mobile->set_user(this_object());

    mobile->teleport(location, 1);

    /* We just set a body number, so we need to save the player data
       file again... */
    save_user_to_file();
  } else {
    location = MAPD->get_room_by_num(current_room);
    mobile = body->get_mobile();
    if(!mobile) {
      mobile = clone_object(USER_MOBILE);
      MOBILED->add_mobile_number(mobile, -1);
      mobile->assign_body(body);
    }

    mobile->set_user(this_object());
    mobile->teleport(location, 1);

    /* Move body to start room */
    if(location->get_number() <= meat_locker_rn) {
      mobile->teleport(start_room, 1);
    }
  }

  /* Show room to player */
  message("\n");
  show_room_to_player(location);
}


/*
 * NAME:	player_logout()
 * DESCRIPTION:	Deal with player body, update account info and so on...
 */
static void player_logout(void)
{
  if(previous_program() != PHANTASMAL_USER)
    error("Wrong program calling player_logout!");

  /* Teleport body to meat locker */
  if(body)
    {
      object meat_locker;
      object mobile;

      if(meat_locker_rn >= 0)
	{
	  meat_locker = MAPD->get_room_by_num(meat_locker_rn);
	  if(meat_locker)
	    {
	      if (location)
		{
		  current_room = location->get_number();
		  mobile = body->get_mobile();
		  mobile->teleport(meat_locker, 1);
		}
	    }
	  else
	    {
	      LOGD->write_syslog("Can't find room #" + meat_locker_rn
				 + " as meat locker!", LOG_ERR);
	    }
	}
    }
  save_user_to_file();
  CHANNELD->unsubscribe_user_from_all(this_object());
}


int process_command(string str)
{
  string cmd;
  int    ctr, size;
  int    force_command;
  mixed* command;

  cmd = str;
  if (strlen(str) != 0 && str[0] == '!') {
    cmd = cmd[1 ..];
    force_command = 1;
  }

  /* Do this unless we're in the editor and didn't start the command
     with an exclamation mark */
  if (!wiztool || !query_editor(wiztool) || force_command) {
    /* check standard commands */
    cmd = STRINGD->trim_whitespace(cmd);
    if(cmd == "") {
      str = nil;
    }

    if (strlen(cmd) != 0) {
      switch (cmd[0]) {
      case '\'':
	if (strlen(cmd) > 1) {
	  str = cmd[1..];
	} else {
	  str = "";
	}
	cmd = "mow";
	break;

      case ':':
	if (strlen(cmd) > 1) {
	  str = cmd[1..];
	} else {
	  str = "";
	}
	cmd = "emo";
	break;

      default:
	/* If single word, leave cmd the same.  If multiword, put
	   first word in cmd. */
	if(sscanf(cmd, "%s %s", cmd, str) != 2) {
	  str = "";
	}
	break;
      }
    }

    if(cmd == "wyloguj")
      {
	message("Do zobaczenia!");
	return MODE_DISCONNECT;
      }

    if(SOULD->is_social_verb(cmd)) {
      cmd_social(this_object(), cmd, str);
      str = nil;
    }

    if(commands_map[cmd]) {
      string err;

      err = (call_other(this_object(),                /* Call on self */
			commands_map[cmd],            /* The function */
			this_object(),                /* This user */
			cmd,                          /* The command */
			str == "" ? nil : str)        /* str or nil */
	     );
      if(err) {
	LOGD->write_syslog("Error on command '" + cmd + "/"
			   + (str ? str : "(nil)") + "'.  Err text: "
			   + err);

	message("Twoja komenda się wykraczyła w kodzie.\n");
	message("Krzycz na Thindila.\n");

	/* Return normal status, print a prompt and continue. */
	return -1;
      }
      str = nil;
    }
  }

  if (str) {
    if (wiztool) {
      string err;

      err = catch(wiztool->command(cmd, str));
      if(err) {
	LOGD->write_syslog("Error on command '" + cmd + "/"
			   + (str ? str : "(nil)") + "'.  Err text: "
			   + err);

	message("Twoja komenda się wykraczyła w kodzie.\n");
	message("Krzycz na Thindila.\n");

	/* Return normal status, print a prompt and continue. */
	return -1;
      }
    } else {
      message("Nie ma takiej komendy: " + cmd + " " + str + "\n");
    }
  }

  /* All is well, just print a prompt and wait for next command */
  return -1;
}


/************** User-level commands *************************/

static void cmd_ooc(object user, string cmd, string str) {
  if (!str || str == "") {
    message("Uzycie: " + cmd + " <tekst>\n");
    return;
  }

  CHANNELD->chat_to_channel(CHANNEL_OOC, str);
}

static void cmd_say(object user, string cmd, string str) {
  if(!str || str == "") {
    message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->say(str);
}

static void cmd_emote(object user, string cmd, string str) {
  if(!str || str == "") {
    message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->emote(str);
}

static void cmd_tell(object self, string cmd, string str) {
  object user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2 ||
      !(user=::find_user(username))) {
    message("Użycie: " + cmd + " <imię> <tekst>\n");
  } else {
    user->message(Name + " mówi Tobie: " + str + "\n");
  }
}

/* Whisper to someone */
static void cmd_whisper(object self, string cmd, string str)
{
  object *user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2)
    {
      message("Użycie: " + cmd + " <imię> <tekst>\n");
      return;
    }
  user = find_first_objects(username, LOC_CURRENT_ROOM);
  if (!user)
    {
      message("Nie ma kogoś o imieniu " + username + " w okolicy.\n");
    }
  else
    {
      mobile->whisper(user[0], str);
    }
}

/* Report bug */
static void cmd_bug(object user, string cmd, string str)
{
  if (!str || str == "")
    {
      message("Użycie: " + cmd + " <tekst>\n");
      return;
    }
  write_file("/usr/game/text/bug_reports.txt", Name + ": " + str + "\n\n");
  message("Dziękujemy za zgłoszenie błędu.\n");
}

/* Report typo */
static void cmd_typo(object user, string cmd, string str)
{
  if (!str || str == "")
    {
      message("Użycie: " + cmd + " <tekst>\n");
      return;
    }
  write_file("/usr/game/text/typo_reports.txt", Name + ": " + str + "\n\n");
  message("Dziękujemy za zgłoszenie literówki.\n");
}

/* Propose idea */
static void cmd_idea(object user, string cmd, string str)
{
  if (!str || str == "")
    {
      message("Użycie: " + cmd + " <tekst>\n");
      return;
    }
  write_file("/usr/game/text/idea_reports.txt", Name + ": " + str + "\n\n");
  message("Dziękujemy za propozycję.\n");
}

static void cmd_whoami(object user, string cmd, string str) {
  message("Jesteś '" + Name + "'.\n");
}

static void cmd_look(object user, string cmd, string str) {
  object* tmp, *objs;
  int     ctr;

  str = STRINGD->trim_whitespace(str);

  if(!location) {
    user->message("Jesteś w pustce!\n");
    return;
  }

  if(!str || str == "") {
    show_room_to_player(location);
    return;
  }

  if (cmd[0] != 'z') {
    /* trim an initial "at" off the front of the command if the verb
       was "look" and not "examine". */
    sscanf(str, "na %s", str);
  }

  if(sscanf(str, "w %s", str) || sscanf(str, "do srodka %s", str)
     || sscanf(str, "do %s", str) || sscanf(str, "pomiedzy %s", str)) {
    /* Look inside container */
    str = STRINGD->trim_whitespace(str);
    tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
    if(!tmp) {
      user->message("Nie znalazłeś jakiegokolwiek '" + str + "'.\n");
      return;
    }
    if(sizeof(tmp) > 1) {
      user->message("Widzisz więcej niż jeden '" + str +"'. Musisz wybrać który.\n");
    }

    if(!tmp[0]->is_container()) {
      user->message("To nie jest pojemnik.\n");
      return;
    }

    if(!tmp[0]->is_open()) {
      user->message("Jest zamknięte.\n");
      return;
    }

    objs = tmp[0]->objects_in_container();
    if(objs && sizeof(objs)) {
      for(ctr = 0; ctr < sizeof(objs); ctr++) {
        user->message("- ");
        user->send_phrase(objs[ctr]->get_brief());
        user->message("\n");
      }
    user->message("-----\n");
    } else {
      user->message("Nie ma nic w ");
      user->send_phrase(tmp[0]->get_brief());
      user->message(".\n");
    }
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie widzisz żadnego '" + str + "'.\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    user->message("Więcej niż jeden taki jest tutaj. "
		  + "Sprawdziłeś pierwszy.\n\n");
  }

  if(cmd[0] == 'z' && tmp[0]->get_examine()) {
    user->send_phrase(tmp[0]->get_examine());
  } else {
    user->send_phrase(tmp[0]->get_look());
  }
  user->message("\n");
}

static void cmd_inventory(object user, string cmd, string str) {
  int    ctr;
  mixed* objs;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  objs = body->objects_in_container();
  if(!objs || !sizeof(objs)) {
    user->message("Nic nie posiadasz przy sobie.\n");
    return;
  }
  for(ctr = 0; ctr < sizeof(objs); ctr++) {
    user->message("- ");
    user->send_phrase(objs[ctr]->get_brief());
    if (objs[ctr]->is_dressed())
      {
	user->message(" (założony)");
      }
    user->message("\n");
  }
}

static void cmd_put(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr;
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

  portlist = find_first_objects(obj1, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj1 + "' w okolicy.\n");
    return;
  }

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj2 + "' w okolicy.\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("Jest więcej niż jeden obiekt pasujący do '" + obj1 + "'.  "
		  + "Wybrałeś " + portlist[0]->get_brief() + ".\n");
  }

  if(sizeof(contlist) > 1) {
    user->message("Jest więcej niż jeden obiekt pasujący do '" + obj2 + "'.  "
		  + "Wybrałeś " + portlist[0]->get_brief() + ".\n");
  }

  port = portlist[0];
  cont = contlist[0];

  if (!(err = mobile->place(port, cont))) {
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
  int     ctr;
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

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj2 + "' w okolicy.\n");
    return;
  }

  if(sizeof(contlist) > 1) {
    user->message("Więcej niż jeden pojemnik pasuje do '" + obj2 + "'.\n");
    user->message("Wybrałeś " + contlist[0]->get_brief() + ".\n");
  }
  cont = contlist[0];

  portlist = cont->find_contained_objects(user, obj1);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj1 + "' w ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("Więcej niż jedna rzecz pasuje do '" + obj1 + "'.\n");
    user->message("Wybrałeś " + portlist[0]->get_brief() + ".\n");
  }
  port = portlist[0];

  if (!(err = mobile->place(port, body))) {
    user->message("Wziąłeś ");
    user->send_phrase(port->get_brief());
    user->message(" z ");
    user->send_phrase(cont->get_brief());
  } else {
    user->message(err + "\n");
  }
}


static void cmd_users(object user, string cmd, string str) {
  int i, sz;
  object* users;
  string name_idx;

  users = users();
  message("Zalogowani: \n");
  str = "";
  for (i = 0, sz = sizeof(users); i < sz; i++) {
    name_idx = users[i]->query_name();
    if (name_idx) {
      str += "   " + name_idx + "       Bezczynny: " + users[i]->get_idle_time()
	+ " sekund\n";
    }
  }
  message(str + "\n");
}

/* List available social commands */
static void cmd_socials(object user, string cmd, string str)
{
  int i, sz;
  string* scommands;
  
  message("Dostępne komendy socjalne:\n");
  scommands = SOULD->all_socials();
  str = "";
  for (i = 0, sz = sizeof(scommands); i < sz; i++)
    {
      str += scommands[i] + "\n";
    }
  message_scroll(str + "\n");
}

static void cmd_movement(object user, string cmd, string str) {
  int    dir;
  string reason;

  /* Currently, we ignore modifiers (str) and just move */

  dir = EXITD->direction_by_string(cmd);
  if(dir == -1) {
    user->message("'" + cmd + "' nie wygląda na poprawny kierunek.\n");
    return;
  }

  if (reason = mobile->move(dir)) {
    user->message(reason + "\n");

    /* don't show the room to the player if they havn't gone anywhere */
    return;
  }

  show_room_to_player(location);
}


/* This one is special, and is called specially... */
static void cmd_social(object user, string cmd, string str) {
  object* targets;

  if(!SOULD->is_social_verb(cmd)) {
    message(cmd + " nie wygląda na poprawną komendę socjalną.\n");
    return;
  }

  if(str && str != "") {
    targets = location->find_contained_objects(user, str);
    if(!targets) {
      message("Nie widzisz niczego co by pasowało do '" + str
	      + "' w okolicy.\n");
      return;
    }
    
    /* For the moment, just pick the first one */
    mobile->social(cmd, targets[0]);
    return;
  }

  mobile->social(cmd, nil);
}

static void cmd_get(object user, string cmd, string str) {
  object* tmp;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " <opis w pomocy>\n");
    return;
  }

  if(sscanf(str, "%*s z wnetrza %*s") == 2
     || sscanf(str, "%*s ze srodka %*s") == 2
     || sscanf(str, "%*s z %*s") == 2
     || sscanf(str, "%*s ze %*s") == 2) {
    cmd_remove(user, cmd, str);
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    message("Więcej niż jedna z tych rzeczy leży w okolicy.\n");
    message("Wybrałeś ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  }

  if(tmp[0] == location) {
    message("Nie możesz tego zabrać. Znajdujesz się wewnątrz tego.\n");
    return;
  }

  if(tmp[0]->get_detail_of()) {
    message("Nie możesz tego wziąć. Jest częścią ");
    send_phrase(tmp[0]->get_detail_of()->get_brief());
    message(".\n");
    return;
  }

  if(!(err = mobile->place(tmp[0], body))) {
    message("Bierzesz ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

static void cmd_drop(object user, string cmd, string str) {
  object* tmp;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " <obiekt>\n");
    return;
  }

  tmp = find_first_objects(str, LOC_INVENTORY, LOC_BODY);
  if(!tmp || !sizeof(tmp)) {
    message("Nie niesiesz ze sobą '" + str + "'.\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    message("Masz więcej niż jedną taką rzecz.\n");
    message("Wybierasz " + tmp[0]->get_brief() + ".\n");
  }

  if (!(err = mobile->place(tmp[0], location))) {
    message("Upuszczasz ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

static void cmd_open(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " <obiekt>\n");
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  ctr = 0;
  if(sizeof(tmp) > 1) {
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      if(tmp[ctr]->is_openable())
	break;
    }
    if(ctr >= sizeof(tmp)) {
      message("Żadne z wybranych nie może być otwarte.\n");
      return;
    }

    message("Więcej niż jedno takie jest w okolicy.\n");
    message("Wybierasz ");
    send_phrase(tmp[ctr]->get_brief());
    message(".\n");
  }

  if(!tmp[ctr]->is_openable()) {
    message("Nie możesz tego otworzyć!\n");
    return;
  }

  if(!(err = mobile->open(tmp[ctr]))) {
    message("Otwierasz ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

static void cmd_close(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " <obiekt>\n");
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  ctr = 0;
  if(sizeof(tmp) > 1) {
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      if(tmp[ctr]->is_openable())
	break;
    }
    if(ctr >= sizeof(tmp)) {
      message("Żadne z tych nie może być zamknięte.\n");
      return;
    }

    message("Więcej niż jedno takie jest w okolicy.\n");
    message("Wybierasz ");
    send_phrase(tmp[ctr]->get_brief());
    message(".\n");
  }

  if(!tmp[ctr]->is_openable()) {
    message("Nie możesz tego zamknąć!\n");
    return;
  }

  if(!(err = mobile->close(tmp[ctr]))) {
    message("Zamknąłeś ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

/* Show list of available commands */
static void cmd_commands(object user, string cmd, string str)
{
  string msg;
  int i;
  mixed indices;

  /* Standard commands */
  msg = "Dostępne komendy: \n";
  indices = map_indices(commands_map);
  for (i = 0; i < sizeof(indices); i++)
    {
      msg += indices[i] + " ";
    }
  msg += "wyloguj \n";
  /* Admin commands */
  if (is_admin())
    {
      mixed* admin_commands;
      msg += "Komendy administracyjne: \n";
      admin_commands = SYSTEM_WIZTOOL->get_command_sets(this_object());
      indices = map_indices(admin_commands[0]);
      for (i = 0; i < sizeof(indices); i++)
	{
	  msg += indices[i] + " ";
	}
	msg += "\n";
    }
  message_scroll(msg);
}

/* Wear weapons, armors, etc */
static void cmd_wear(object user, string cmd, string str)
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
      message("Użycie: " + cmd + " <obiekt>\n");
      return;
    }
  tmp = find_first_objects(str, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp))
    {
      message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
      return;
    }
  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if ((tmp[ctr]->is_wearable() || tmp[ctr]->is_weapon()) && !tmp[ctr]->is_dressed())
	    {
	      break;
	    }
	}
      if(ctr >= sizeof(tmp))
	{
	  message("Żadna z tych rzeczy nie może być założona.\n");
	  return;
	}
      message("Więcej niż jedna taka rzecz jest w ekwipunku.\n");
      message("Wybierasz ");
      send_phrase(tmp[ctr]->get_brief());
      message(".\n");
    }
  if(!tmp[ctr]->is_wearable() && !tmp[ctr]->is_weapon())
    {
      message("Nie możesz tego założyć!\n");
      return;
    }
  if (tmp[ctr]->is_dressed())
    {
      message("Ten obiekt jest już założony!\n");
      return;
    }
  items = body->objects_in_container();
  if (tmp[ctr]->is_weapon())
    {
      for (i = 0; i < sizeof(items); i++)
	{
	  if (items[i]->is_weapon() && items[i]->is_dressed())
	    {
	      message("Chowasz " + items[i]->get_brief()->to_string(user) + ".\n");
	      items[i]->set_dressed(0);
	      break;
	    }
	}
      message("Bierzesz " + tmp[ctr]->get_brief()->to_string(user) + " do ręki.\n");
    }
  else
    {
      
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
      message("Użycie: " + cmd + " <obiekt>\n");
      return;
    }
  tmp = find_first_objects(str, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp))
    {
      message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
      return;
    }
  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if ((tmp[ctr]->is_wearable() || tmp[ctr]->is_weapon()) && tmp[ctr]->is_dressed())
	    {
	      break;
	    }
	}
      if(ctr >= sizeof(tmp))
	{
	  message("Żadna z tych rzeczy nie może być zdjęta.\n");
	  return;
	}
    }
  if (!tmp[ctr]->is_dressed())
    {
      message("Ten obiekt nie jest założony!\n");
      return;
    }
  items = body->objects_in_container();
  if (tmp[ctr]->is_weapon())
    {
      message("Chowasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }
  else
    {
      message("Zdejmujesz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }
  tmp[ctr]->set_dressed(0);
}
