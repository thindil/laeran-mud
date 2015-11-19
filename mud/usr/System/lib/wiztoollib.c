#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>
#include <gameconfig.h>

#include <trace.h>
#include <type.h>
#include <status.h>
#include <limits.h>

#define SYSTEM_ROOMWIZTOOLLIB  "/usr/System/lib/room_wiztool"
#define SYSTEM_OBJWIZTOOLLIB   "/usr/System/lib/obj_wiztool"

inherit auto AUTO;
inherit wiz LIB_WIZTOOL;
inherit access API_ACCESS;

inherit roomwiz SYSTEM_ROOMWIZTOOLLIB;
inherit objwiz  SYSTEM_OBJWIZTOOLLIB;

private string owner;		/* owner of this object */
private string directory;	/* current directory */

mixed* command_sets;


#define SPACE16 "                "

/*
 * NAME:	ralign()
 * DESCRIPTION:	return a number as a right-aligned string
 */
private string ralign(mixed num, int width)
{
    string str;

    str = SPACE16 + (string) num;
    return str[strlen(str) - width ..];
}


/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{
  wiz::create(200);
  access::create();
  roomwiz::create(clone);
  objwiz::create(clone);

  if(clone) {
    owner = query_owner();
    directory = USR_DIR + "/" + owner;
  } else {
    if(!find_object(US_OBJ_DESC))
      auto::compile_object(US_OBJ_DESC);
    if(!find_object(US_ENTER_DATA))
      auto::compile_object(US_ENTER_DATA);
    if(!find_object(UNQ_DTD))
      auto::compile_object(UNQ_DTD);
    if(!find_object(SIMPLE_ROOM))
      auto::compile_object(SIMPLE_ROOM);
    if (!find_object(US_MAKE_SOCIAL))
        auto::compile_object(US_MAKE_SOCIAL);
    if (!find_object(US_MAKE_QUEST))
        auto::compile_object(US_MAKE_QUEST);
  }
}

static void destructed(varargs int clone) {
  roomwiz::destructed(clone);
  objwiz::destructed(clone);
}

static void upgraded(varargs int clone) {
  roomwiz::upgraded(clone);
  objwiz::upgraded(clone);
}

mixed* get_command_sets(object wiztool) {
  if(!SYSTEM() && !GAME())
    return nil;

  return command_sets;
}

/******************* Command functions *********************/

/**** Repackaged wiztool commands from default wiztoollib ***/

static void cmd_shutdown(object user, string cmd, string str)
{
  if(str && str != "") {
    if(str == "force") {
      find_object(INITD)->force_shutdown();
    } else {
      user->message("Nieznany argument. Spróbuj ponownie.\n");
      return;
    }
  } else {
    find_object(INITD)->prepare_shutdown();
  }
}

static void cmd_reboot(object user, string cmd, string str) {
  /* DRIVER will do this for us, so we don't need to */
  /* find_object(INITD)->prepare_reboot(); */
  wiz::cmd_reboot(user, cmd, str);
}

static void cmd_compile(object user, string cmd, string str)
{
  string objname;

  user->message("Kompilowanie '" + str + "'.\n");

  if(!sscanf(str, "$%*d") && sscanf(str, "%s.c", objname)) {
    mixed* status;

    status = OBJECTD->od_status(objname);
    if(status) {
      /* Check to see if there are children and most recent issue is
	 destroyed... */
      if(status[3] && sizeof(status[3]) && !status[6]) {
	user->message("Nie mogę rekompilować -- obiekt posiada potomków!\n");
	return;
      }
    }
  }

  if(!sscanf(str, "$%*d") && !sscanf(str, "%*s.c")) {
    if(!read_file(str, 0, 1) && read_file(str + ".c", 0, 1)) {
      user->message("(kompilowanie " + str + ".c)\n");
      str += ".c";
    }
  }

  catch {
    wiz::cmd_compile(user, cmd, str);
  } : {
    if(ERRORD->last_compile_errors()) {
      user->message("===Błędy kompilacji:\n" + ERRORD->last_compile_errors());
      user->message("---\n");
    }

    if(ERRORD->last_runtime_error()) {
      if(sscanf(ERRORD->last_runtime_error(),
		"%*sFailed to compile%*s") == 2) {
	return;
      }

      user->message("===Błąd wykonania: '" + ERRORD->last_runtime_error()
		    + "'.\n");
      user->message("---\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Ślad stosu: '" + ERRORD->last_stack_trace()
		    + "'.\n");
      user->message("---\n");
    }

    return;
  }

  user->message("Wykonane.\n");
}


static void cmd_destruct(object user, string cmd, string str)
{
  user->message("Niszczenie '" + str + "'.\n");

  catch {
    wiz::cmd_destruct(user, cmd, str);
  } : {
    if(ERRORD->last_runtime_error()) {
      user->message("===Błąd wykonania: '" + ERRORD->last_runtime_error()
		    + "'.\n");
      user->message("---\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Ślad stosu: '" + ERRORD->last_stack_trace()
		    + "'.\n");
      user->message("---\n");
    }

    return;
  }

  user->message("Wykonane.\n");
}

/* This currently extracts only alphabetic characters from a name, and
   converts it to lowercase.  If this changes, change it in
   PHANTASMAL_USER as well. */
static string username_to_filename(string str) {
  int iter;
  int len;
  string ret;

  ret = "";
  if(!str) return nil;

  if(str == "Ecru" || str == "System") return str;

  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(str[iter] >= 'a' && str[iter] <= 'z')
      ret += str[iter..iter];
    else if(str[iter] >= 'A' && str[iter] <= 'Z') {
      str[iter] += 'a' - 'A';
      ret += str[iter..iter];
    }
  }
  return ret;
}

static void cmd_access(object user, string cmd, string str) {
  if(str)
    str = username_to_filename(str);

  wiz::cmd_access(user, cmd, str);
}

static void cmd_grant(object user, string cmd, string str) {
  string who, dir_and_type;

  if (str &&
      (sscanf(str, "%s %s", who, dir_and_type) == 2)) {
    who = username_to_filename(who);
    str = who + " " + dir_and_type;
  }

  wiz::cmd_grant(user, cmd, str);
}

static void cmd_ungrant(object user, string cmd, string str) {
  string who, dir;

  if (str &&
      (sscanf(str, "%s %s", who, dir) == 2)) {
    who = username_to_filename(who);
    str = who + " " + dir;
  }

  wiz::cmd_ungrant(user, cmd, str);
}

static void cmd_quota(object user, string cmd, string str) {
  string who, what;

  if(str) {
    if(sscanf(str, "%s %s", who, what) == 2) {
      who = username_to_filename(who);
      str = who + " " + what;
    } else {
      str = username_to_filename(str);
    }
  }

  wiz::cmd_quota(user, cmd, str);
}

/**** Phantasmal-specific wiztool commands */

static void cmd_datadump(object user, string cmd, string str) {
  find_object(INITD)->save_mud_data(user, ROOM_DIR, MOB_DIR, ZONE_DIR,
				    SOCIAL_DIR, QUEST_DIR, nil);
  user->message("Rozpoczęto zapis danych.\n");
}

static void cmd_safesave(object user, string cmd, string str) {
  find_object(INITD)->save_mud_data(user, SAFE_ROOM_DIR, SAFE_MOB_DIR,
				    SAFE_ZONE_DIR, SAFE_SOCIAL_DIR, SAFE_QUEST_DIR, nil);
  user->message("Rozpoczęto bezpieczny zapis danych.\n");
}


/*
 * NAME:	evaluate_lpc_code()
 * DESCRIPTION:	Evaluate a piece of LPC code, returning a result.
 *              Based on implementation of the code command.
 */
static mixed evaluate_lpc_code(object user, string lpc_code)
{
  mixed *parsed, result;
  object obj;
  string name, str;

  if (!lpc_code) {
    error("Nie mogę określić (nil) jako kodu LPC!");
  }

  parsed = parse_code(lpc_code);
  if (!parsed) {
    error("Nie mogę sparsować kodu!");
  }
  name = USR_DIR + "/" + owner + "/_code";
  obj = find_object(name);
  if (obj) {
    destruct_object(obj);
  }

  str = USR_DIR + "/" + owner + "/include/code.h";
  if (file_info(str)) {
    str = "# include \"~/include/code.h\"\n";
  } else {
    str = "";
  }
  str = "# include <float.h>\n# include <limits.h>\n" +
    "# include <status.h>\n# include <trace.h>\n" +
    "# include <type.h>\n" + str + "\n" +
    "mixed exec(object user, mixed argv...) {\n" +
    "    mixed a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z;\n\n" +
    "    " + parsed[0] + "\n}\n";
  str = catch(obj = compile_object(name, str),
	      result = obj->exec(user, parsed[1 ..]...));
  if (str) {
    error(str);
    result = nil;
  }

  if (obj) {
    destruct_object(obj);
  }

  return result;
}


static void cmd_whoami(object user, string cmd, string str)
{
  if (str && str != "") {
    message("Użycie: " + cmd + "\n");
    return;
  }

  message("Jesteś '" + user->get_Name() + "'. Nazwa konta: '"
	  + user->get_name() + "'.\n");
}

static void cmd_people(object user, string cmd, string str)
{
  object *users, usr;
  string *owners, name, ipstr;
  int i, sz;

  if (str && str != "") {
    message("Użycie: " + cmd + "\n");
    return;
  }

  str = "";
  users = users();
  owners = query_owners();
  for (i = 0, sz = sizeof(users); i < sz; i++) {
    usr = users[i];
    name = usr->query_name();
    if(usr->query_conn()) {
      ipstr = query_ip_number(usr->query_conn());
      if(!ipstr)
	ipstr = "--host?--";

      str += (ipstr + SPACE16)[.. 15];
    } else {
      str += ("--rozł--" + SPACE16)[.. 15];
    }
    str += (usr->get_idle_time() + " sekund bezczynny" + SPACE16)[..18];
    str += ((sizeof(owners & ({ name })) == 0) ? " " : "*");
    str += name + "\n";
  }
  message(str);
}

static void cmd_writelog(object user, string cmd, string str)
{
  if(str) {
    LOGD->write_syslog(str, LOG_ERR_FATAL);
  } else {
    user->message("Użycie: " + cmd + " <tekst do zapisania>\n");
  }
}

static void cmd_log_subscribe(object user, string cmd, string str) {
  string chan, levname;
  int    lev;

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user
      ->message("Nie można ustawić subskrypcji plików dziennika bez pełnych uprawnień administracyjnych!");
    return;
  }

  if(str && sscanf(str, "%s %d", chan, lev) == 2) {
    LOGD->set_channel_sub(chan, lev);
    user->message("Ustawianie subskrypcji kanału '" + chan + "' na "
		  + lev + "\n");
    return;
  } else if (str && sscanf(str, "%s %s", chan, levname) == 2
	     && LOGD->get_level_by_name(levname)) {
    int level;

    level = LOGD->get_level_by_name(levname);
    LOGD->set_channel_sub(chan, level);
    user->message("Ustawianie subskrypcji kanału '" + chan + "' na "
		  + level + "\n");
    return;
  } else if (str && sscanf(str, "%s", chan)) {
    lev = LOGD->channel_sub(chan);
    if(lev == -1) {
      user->message("Brak subskrypcji do kanału '" + chan + "'\n");
    } else {
      user->message("Subskrypcja do kanału '" + chan + "' to " + lev + "\n");
    }
    return;
  } else {
    user->message("Użycie: %log_subscribe <kanał> <poziom>\n");
  }
}

static void cmd_list_dest(object user, string cmd, string str)
{
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  if(!find_object(OBJECTD))
    auto::compile_object(OBJECTD);

  user->message(OBJECTD->destroyed_obj_list());
}

static void cmd_od_report(object user, string cmd, string str)
{
  int    i, hmax;
  mixed  obj;
  string report;

  if(!find_object(OBJECTD))
    auto::compile_object(OBJECTD);

  hmax = sizeof(::query_history());

  i = -1;
  if(!str || (sscanf(str, "$%d%s", i, str) == 2 &&
	      (i < 0 || i >= hmax || str != ""))) {
    message("Użycie: " + cmd + " <obiekt> | $<identyfikator>\n");
    return;
  }

  if (i >= 0) {
    obj = fetch(i);
    if(typeof(obj) != T_OBJECT) {
      message("Nie jest obiektem.\n");
      return;
    }
  } else if (sscanf(str, "$%s", str)) {
    obj = ::ident(str);
    if (!obj) {
      message("Nieznany: $identyfikator.\n");
      return;
    }
  } else if (sscanf(str, "#%*d")) {
    obj = str;
  } else if (sscanf(str, "%*d")) {
    obj = str;
  } else {
    obj = DRIVER->normalize_path(str, directory, owner);
  }

  str = catch(report = OBJECTD->report_on_object(obj));
  if(str) {
    str += "\n";
  } else if (!report) {
    str = "Zgłoszono nil z Menadżera Obiektów!\n";
  } else {
    str = report;
  }

  message(str);
}


static void cmd_full_rebuild(object user, string cmd, string str) {
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Obecnie tylko osoby z pełnymi prawami administracyjnymi "
		  + "mogą robić pełną przebudowę.\n");
    return;
  }

  user->message("Rekompilacja obiektu auto...\n");

  catch {
    OBJECTD->recompile_auto_object(user);
  } : {
    if(ERRORD->last_compile_errors()) {
      user->message("===Błędy kompilacji:\n" + ERRORD->last_compile_errors());
      user->message("---\n");
    }

    if(ERRORD->last_runtime_error()) {
      if(sscanf(ERRORD->last_runtime_error(),
		"%*sFailed to compile%*s") == 2) {
	return;
      }

      user->message("===Błąd wykonania: '" + ERRORD->last_runtime_error()
		    + "'.\n");
      user->message("---\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Ślad stosu: '" + ERRORD->last_stack_trace()
		    + "'.\n");
      user->message("---\n");
    }

    return;
  }

  user->message("Wykonane.\n");
}


static void cmd_list_mobiles(object user, string cmd, string str) {
    int*   mobiles;
    int    ctr;
    object mob, phr;
    string tmp;

    if(str && !STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    mobiles = MOBILED->all_mobiles();

    tmp = "";
    for(ctr = 0; ctr < sizeof(mobiles); ctr++) {
        mob = MOBILED->get_mobile_by_num(mobiles[ctr]);
        tmp += ralign("" + mobiles[ctr], 8);
        tmp += "   ";

        tmp += ralign(mob->get_type(), 8);
        tmp += "     ";

        if(mob->get_body()) {
            phr = mob->get_body()->get_brief();
            tmp += phr->to_string(user);
        } else {
            tmp += "<mob bez ciała>";
        }
        tmp += "\n";
    }
    tmp += "-----\n";
    user->message_scroll(tmp);
}


static void cmd_delete_mobile(object user, string cmd, string str) {
  int    mobnum;
  object mob, body, location;

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "%*s %*s") == 2
     || sscanf(str, "#%d", mobnum) != 1) {
    user->message("Użycie: " + cmd + " #<numer mobka>\n");
    return;
  }

  mob = MOBILED->get_mobile_by_num(mobnum);
  if(!mob) {
    user->message("Nie ma mobka #" + mobnum
		  + " zarejestrowanego z MOBILED. Nieudane.\n");
    return;
  }

  if(mob->get_user()) {
    user->message("Mobek jest ciągle powiązany z połączeniem sieciowym."
		  + " Nieudane.\n");
    return;
  }

  /* Need to remove mobile from any room lists it currently occupies. */
  MOBILED->remove_mobile(mob);

  user->message("Mobek #" + mobnum + " został pomyślnie usunięty.\n");
}


static void cmd_delete_obj(object user, string cmd, string str) {
  object *objs;
  int     obj_num;

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + " #<numer obiektu>\n");
    user->message("   lub  " + cmd + " opis obiektu\n");
    return;
  }

  if(sscanf(str, "#%*d %*s") != 2
     && sscanf(str, "%*s #%*d") != 2
     && sscanf(str, "#%d", obj_num) == 1) {
    /* Delete by object number */

  } else {
    /* Delete by object name */

    str = STRINGD->trim_whitespace(str);
    if(user->get_location()) {
      objs = user->get_location()->find_contained_objects(user, str);
      if(!objs)
	objs = user->get_body()->find_contained_objects(user, str);
      if(!objs || !sizeof(objs)) {
	user->message("Nie ma nic pasującego do '" + str + "'.\n");
	return;
      }
      if(sizeof(objs) > 1) {
	user->message("Istnieje wiele rzeczy pasujących do '" + str + "'.\n");
	user->message("Okreść dokładnie jedną.\n");
	return;
      }
      obj_num = objs[0]->get_number();
    } else {
      user->message("Jesteś w pustce. Nie możesz kasować rzeczy tutaj.\n");
      return;
    }
  }

  if(MOBILED->get_mobile_by_num(obj_num)) {
    /* Do a mobile delete */
    cmd_delete_mobile(user, "@delete_mobile", "#" + obj_num);
  } else if(MAPD->get_room_by_num(obj_num)) {
    /* Do a room delete */
    cmd_delete_room(user, "@delete_room", "#" + obj_num);
  } else if(EXITD->get_exit_by_num(obj_num)) {
    object exit;

    user->message("Usuwanie wyjścia...\n");
    /* Do an exit delete */
    exit = EXITD->get_exit_by_num(obj_num);
    EXITD->clear_exit(exit);

    user->message("Wykonane.\n");
  } else {
    user->message("To nie jest przenośny, pokój, mob lub wyjście.\n");
    user->message("Albo to nie istnieje albo @delete nie może tego usunąć.\n");
    return;
  }
}


static void cmd_segment_map(object user, string cmd, string str) {
  int hs, ctr;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  user->message("Segmenty:\n");
  hs = OBJNUMD->get_highest_segment();
  for(ctr = 0; ctr <= hs; ctr++) {
    user->message((OBJNUMD->get_segment_owner(ctr) != nil) ?
                ((ctr + SPACE16)[..6]
                + (OBJNUMD->get_segment_owner(ctr) + SPACE16)[..30]
                + ZONED->get_segment_zone(ctr)
                + "\n") : "");
  }
  user->message("--------\n");
}


static void cmd_set_segment_zone(object user, string cmd, string str) {
  int segnum, zonenum;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "#%d #%d", segnum, zonenum) != 2) {
    user->message("Użycie: " + cmd + " #<numer segmentu> #<numer strefy>\n");
    return;
  }

  if(!OBJNUMD->get_segment_owner(segnum)) {
    user->message("Nie mogę znaleźć segmentu #" + segnum + ". Sprawdź @segmap.\n");
    return;
  }
  if(zonenum >= ZONED->num_zones()) {
    user->message("Nie mogę znaleźć strefy #" + zonenum + ". Sprawdź @zonemap.\n");
    return;
  }
  ZONED->set_segment_zone(segnum, zonenum);

  user->message("Ustawiono segment #" + segnum + " (obiekty #" + (segnum * 100)
		+ "-#" + (segnum * 100 + 99) + ") aby był w strefie #" + zonenum
		+ " (" + ZONED->get_name_for_zone(zonenum) + ").\n");
}


static void cmd_zone_map(object user, string cmd, string str) {
  int ctr, num_zones;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  num_zones = ZONED->num_zones();

  user->message("Strefy:\n");
  for(ctr = 0; ctr < num_zones; ctr++) {
    user->message(ralign(ctr + "", 3) + ": " + ZONED->get_name_for_zone(ctr)
		  + "\n");
  }
  user->message("-----\n");
}

static void cmd_new_zone(object user, string cmd, string str) {
  int ctr, new_zonenum;
  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + " <nazwa strefy>\n");
    return;
  }

  new_zonenum = ZONED->add_new_zone( str );

  user->message("Dodano strefę #"+new_zonenum+"\n");
}

static void cmd_new_mobile(object user, string cmd, string str) {
  int    mobnum, bodynum, parentnum, spawnroom;
  string mobtype;
  object mobile, body;

  mobnum = -1;
  parentnum = 0;
  spawnroom = 0;
  if(!str) {
    user->message("Użycie: " + cmd + " #<numer ciała> <typ mobka> #<ciało rodzica> #<pokój spawnu>\n");
    return;
  }

  if((sscanf(str, "#%d %s", bodynum, mobtype) != 2)
     || (sscanf(str, "#%d %s #%d", bodynum, mobtype, parentnum) != 3)
     || (sscanf(str, "#%d %s #%d #%d", bodynum, mobtype, parentnum, spawnroom) != 4))
    {
      user->message("Użycie: " + cmd
		    + " #<numer ciała> <typ mobka> #<ciało rodzica> #<pokój spawnu>\n");
      return;
    }

  mobtype = STRINGD->to_lower(mobtype);

  if(mobtype == "user") {
    user->message("Wiem, że jesteś administratorem ale to jest zły pomysł "
		  + "aby tworzyć losowe\n"
		  + " mobki użytkowników. Zatrzymuję Ciebie.\n");
    return;
  }

  body = MAPD->get_room_by_num(bodynum);

  if(!MOBILED->get_file_by_mobile_type(mobtype)) {
    user->message("MOBILED nie rozpoznaje typu '" + mobtype
		  + "',\n więc nie możesz stworzyć tego mobka.\n");
    user->message(" Może musisz dodać go do bindera?\n");
    return;
  }

  mobile = MOBILED->clone_mobile_by_type(mobtype);
  if(!mobile)
    error("Nie można sklonować mobka typu '" + mobtype + "'!");

  if (body)
    {
      mobile->assign_body(body);
    }
  mobile->set_parentbody(parentnum);
  mobile->set_spawnroom(spawnroom);

  MOBILED->add_mobile_number(mobile, mobnum);
  user->message("Dodano mobka #" + mobile->get_number() + ".\n");
}

static void cmd_new_tag_type(object user, string cmd, string str) {
  string scope, name, type, getter, setter, unqstring;
  mapping scope_strings, type_strings;

  scope_strings = ([ "object" : "object",
		   "obj" : "object",
		   "mobile" : "mobile",
		   "mob" : "mobile" ]);

  type_strings = ([ "int" : T_INT,
		  "1" : T_INT,
		  "integer" : T_INT,	
		  "float" : T_FLOAT,
		  "real" : T_FLOAT,
		  "2" : T_FLOAT ]);

  if(!str || !strlen(str) || (sscanf(str, "%*s %*s %*s %*s %*s %*s") == 6)
     || ((sscanf(str, "%s %s %s %s %s", scope, name, type, getter,
		 setter) != 5)
	 && (sscanf(str, "%s %s %s %s", scope, name, type, getter) != 4)
	 && (sscanf(str, "%s %s %s", scope, name, type) != 3))) {
    user->message("Użycie: " + cmd
		  + " <obiekt|mob> <nazwa> <typ> [<getter> [<setter>]]\n");
    return;
  }

  scope = STRINGD->trim_whitespace(STRINGD->to_lower(scope));
  if(!scope_strings[scope]) {
    user->message("Rodzaj '" + scope + "' nie jest znany.\n");
    user->message("Powinno być 'object' lub 'mobile'.\n");
    return;
  }

  type = STRINGD->trim_whitespace(STRINGD->to_lower(type));
  if(!type_strings[type]) {
    user->message("Typ '" + type + "' nie jest rozpoznany.\n");
    user->message("Powinno być 'int' lub 'float'.\n");
    return;
  }

  name = STRINGD->trim_whitespace(name);

  call_other(TAGD, "new_" + scope_strings[scope] + "_tag",
	     name, type_strings[type], getter, setter);
  user->message("Dodano nowy typ tagu '" + name + "'.\n");
}

static void cmd_set_tag(object user, string cmd, string str) {
  object obj_to_set;
  string usage_string, str2, err, tag_name;
  int    index;
  mixed  chk, *split_tmp;

  usage_string = "Użycie: " + cmd + " #<obiekt> <nazwa tagu> <wartość>\n"
    + "       " + cmd + " $<historia> <nazwa tagu> <wartość>\n";
  if(sscanf(str, "#%d %s", index, str2) == 2) {
    obj_to_set = MAPD->get_room_by_num(index);
    if(!obj_to_set)
      obj_to_set = MOBILED->get_mobile_by_num(index);

    if(!obj_to_set) {
      user->message("Nie mogę znaleźć obiektu #" + index + "!\n" + usage_string);
      return;
    }
  } else if (sscanf(str, "$%d %s", index, str2) == 2) {
    if(index >= 0 && index <= sizeof(::query_history())) {
      chk = fetch(index);
      if(typeof(chk) == T_OBJECT)
	obj_to_set = chk;
      else {
	user->message("Wpis historii $" + index + " nie jest obiektem!\n");
	return;
      }

      if(function_object("get_tag", obj_to_set) != TAGGED) {
	user->message("Wpis historii $" + index
		      + " nie jest stagowanym obiektem!\n");
	return;
      }

    } else {
      user->message("Nie mogę znaleźć wpisu historii $" + index + ".\n");
      return;
    }

  } else {
    user->message(usage_string);
    return;
  }

  split_tmp = explode(str2, " ");
  if(sizeof(split_tmp) < 2) {
    user->message(usage_string);
    return;
  }
  tag_name = split_tmp[0];
  str2 = implode(split_tmp[1..], " ");

  user->message("Szacowanie kodu obiektu, '" + tag_name + "', kod: '"
		+ str2 + "'.\n");

  /* Now we have obj_to_set, and str2 contains the remaining command line */
  /* err = catch (chk = evaluate_lpc_code(user, str2)); */
  chk = evaluate_lpc_code(user, str2);
  if(err) {
    user->message("Błąd w szacowaniu kodu: " + err + "\n");
    return;
  }

  store(chk);
  TAGD->set_tag_value(obj_to_set, tag_name, chk);
  user->message("Ustawiono wartość tagu '" + tag_name + "' na "
		+ STRINGD->mixed_sprint(chk) + ".\n");
}

static void cmd_list_tags(object user, string cmd, string str) {
  string  *all_tags, msg;
  mapping  type_names;
  int      ctr;

  type_names = ([ T_INT : "int",
		T_FLOAT : "flt",
		T_MAPPING : "map",
		T_ARRAY : "arr",
		T_NIL   : "nil" ]);

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || !strlen(str)
     || ((str != "object") && (str != "mobile"))) {
    user->message("Użycie: " + cmd + " <object|mobile>\n");
    return;
  }

  switch(str) {
  case "object":
    all_tags = TAGD->object_tag_names();
    break;
  case "mobile":
    all_tags = TAGD->mobile_tag_names();
    break;
  default:
    error("Wewnętrzny błąd!");
  }

  if(sizeof(all_tags) == 0) {
    user->message("Nie ma jakichkolwiek tagów.\n");
    return;
  }

  msg = "Nazwy tagów i typy:\n";
  for(ctr = 0; ctr < sizeof(all_tags); ctr++) {
    int type;

    switch(str) {
    case "object":
      type = TAGD->object_tag_type(all_tags[ctr]);
      break;
    case "mobile":
      type = TAGD->mobile_tag_type(all_tags[ctr]);
      break;
    default:
      error("Wewnętrzny błąd!");
    }

    msg += "  " + type_names[type] + "  " + all_tags[ctr] + "\n";
  }

  user->message_scroll(msg);
}

static void cmd_reports(object user, string cmd, string str)
{
  string report;
  
  if (!str)
    {
      user->message("Użycie: " + cmd + " bug, idea, literowka, pomoc\n");
      return;
    }

  switch(str)
    {
    case "bug":
      report = read_file("/usr/game/text/bug_reports.txt");
      break;
    case "idea":
      report = read_file("/usr/game/text/idea_reports.txt");
      break;
    case "literowka":
      report = read_file("/usr/game/text/typo_reports.txt");
      break;
    case "pomoc":
      report = read_file("/usr/game/text/help_reports.txt");
      break;
    default:
      report = nil;
      user->message("Użycie: " + cmd + " bug, idea, literowka, pomoc\n");
      return;
    }

  if (!report)
    {
      user->message("Brak zgłoszeń na ten temat.\n");
    }
  else
    {
      user->message_scroll(report + "\n");
    }
}

static void cmd_kick(object user, string cmd, string str)
{
    int i;
    object *users;
    string name;

    if (!str || STRINGD->is_whitespace(str)) {
        message("Użycie: " + cmd + " <imię postaci>\n");
        return;
    }

    str = STRINGD->to_lower(str);
    if (str == user->query_name()) {
        message("Nie możesz wykopać samego siebie.\n");
        return;
    }

    users = users();
    for (i = 0; i < sizeof(users); i++) {
        name = STRINGD->to_lower(users[i]->query_name());
        if (str == name) {
            if (users[i]->is_admin()) {
                message("Nie możesz wykopać Opiekuna z gry.\n");
                return;
            }
            users[i]->message("Zostałeś wyrzucony z gry przez " + user->query_name());
            users[i]->disconnect();
            message("Wykopałeś gracza: " + str + " z gry.\n");
            return;
        }
    }
    message("Nie ma w grze gracza o imieniu " + str + ".\n");
}

static void cmd_make_social(object user, string cmd, string str)
{
    object state;

    if(str && !STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    state = clone_object(US_MAKE_SOCIAL);
    user->push_new_state(US_MAKE_SOCIAL);
}

static void cmd_ban(object user, string cmd, string str)
{
    string *banned, *parts;
    string message;
    int i;

    if (!str || STRINGD->is_whitespace(str)) {
        message("Użycie: " + cmd + " [lista|dodaj|usun]\n");
        return;
    }

    parts = explode(str, " ");
    banned = GAME_DRIVER->get_banned_ip();
    switch (parts[0]) {
        case "lista":
            if (!sizeof(banned)) {
                message("Nikogo jeszcze nie zbanowano.\n");
                return;
            }
            user->message_scroll("Lista zbanowanych: \n" + implode(banned, "\n") + "\n");
            break;
        case "dodaj":
            if (!sizeof(banned))
                GAME_DRIVER->set_banned_ip(({ parts[1] }));
            else {
                for (i = 0; i < sizeof(banned); i++) {
                    if (banned[i] == parts[1]) {
                        message("Ten adres jest już zbanowany.\n");
                        return;
                    }
                }
                GAME_DRIVER->add_banned_ip(parts[1]);
            }
            message("Dodano IP: " + parts[1] + " do listy zbanowanych\n");
            break;
        case "usun":
            if (!sizeof(banned)) {
                message("Nikogo jeszcze nie zbanowano.\n");
                return;
            }
            for (i = 0; i < sizeof(banned); i++) {
                if (banned[i] == parts[1]) {
                    banned -= ({ parts[1] });
                    break;
                }
            }
            if (sizeof(banned) == sizeof(GAME_DRIVER->get_banned_ip()))
                message("Nie ma takiego adresu zbanowanego.\n");
            else
                message("Usunięto IP: " + parts[1] + " z listy zbanowanych.\n");
            GAME_DRIVER->set_banned_ip(banned);
            break;
        default:
            message("Użycie: " + cmd + " [lista|dodaj|usun]\n");
            break;
    }
}

static void cmd_change_password(object user, string cmd, string str)
{
    object player;
    int i;
    string *parts;
    object *users;

    if(!access(user->query_name(), "/", FULL_ACCESS)) {
        user->message("Tylko administrator z pełnymi prawami może zmieniać hasła!");
        return;
    }
    if (!str || STRINGD->is_whitespace(str)) {
        message("Użycie: " + cmd + " <gracz> <nowe hasło>\n");
        return;
    }
    
    parts = explode(str, " ");
    users = users();
    for (i = 0; i < sizeof(users); i++) {
        if (STRINGD->to_lower(parts[0]) == users[i]->get_name()) {
            users[i]->set_password(implode(parts[1..], " "));
            message("Hasło dla postaci " + parts[0] + " zostało zmienione.\n");
            return;
        }
    }

    player = clone_object("/usr/game/obj/user");
    if (!player->restore_user_from_file(parts[0])) {
        message("Nie ma gracza o imieniu " + parts[0] + ".\n");
        return;
    }
    player->set_password(implode(parts[1..], " "));
    player->save_user_to_file();
    destruct_object(player);
    message("Hasło dla postaci " + parts[0] + " zostało zmienione.\n");
}

static void cmd_set_mob_value(object user, string cmd, string str)
{
    string option, value;
    int number, type;
    object mobile;

    if (!str || STRINGD->is_whitespace(str) 
            || sscanf(str, "#%d %s %s", number, option, value) != 3) {
        message("Użycie: " + cmd + " #<numer mobka> <nazwa opcji> <wartość>\n");
        return;
    }

    mobile = MOBILED->get_mobile_by_num(number);
    if (!mobile) {
        message("Nie ma mobka o numerze #" + number + ".\n");
        return;
    }
    if (mobile->get_type() == "user") {
        message("Nie możesz zmieniać ustawień graczom.\n");
        return;
    }
    if (!function_object("set_" + option, mobile)) {
        message("Nie można ustawić tej opcji mobka.\n");
        return;
    }
    type = typeof(call_other(mobile, "get_" + option));
    switch (type) {
        case T_INT:
            call_other(mobile, "set_" + option, (int)value);
            break;
        case T_FLOAT:
            call_other(mobile, "set_" + option, (float)value);
            break;
        case T_STRING:
            call_other(mobile, "set_" + option, value);
            break;
        case T_ARRAY:
            call_other(mobile, "set_" + option, explode(value, ", "));
            break;
        default:
            message("Nie obsługuję tego typu zmiennych.\n");
            return;
    }
    message("Zmieniono ustawienie mobka #" + number + " " + option + " na wartość: " + value + ".\n");
}

static void cmd_log_user(object user, string cmd, string str)
{
    string name, option;
    object *users;
    int i;
    object player;

    if (!str || STRINGD->is_whitespace(str) || sscanf(str, "%s %s", name, option) != 2) {
        message("Użycie: " + cmd + " <gracz> [start lub stop]\n");
        return;
    }
    
    users = users();
    for (i = 0; i < sizeof(users); i++) {
        if (STRINGD->to_lower(name) == users[i]->get_name()) {
            if (option == "start") {
                TAGD->set_tag_value(users[i]->get_body()->get_mobile(), "Logged", 1);
                message("Rozpoczęto śledzenie gracza: " + name + ".\n");
                return;
            } else {
                TAGD->set_tag_value(users[i]->get_body()->get_mobile(), "Logged", nil);
                message("Zaprzestano śledzenia gracza: " + name + ".\n");
                return;
            }
        }
    }

    player = clone_object("/usr/game/obj/user");
    if (!player->restore_user_from_file(name)) {
        message("Nie ma gracza o imieniu " + name + ".\n");
        return;
    }
    if (option == "start") {
        TAGD->set_tag_value(users[i]->get_body()->get_mobile(), "Logged", 1);
        message("Rozpoczęto śledzenie gracza: " + name + ".\n");
    } else {
        TAGD->set_tag_value(users[i]->get_body()->get_mobile(), "Logged", nil);
        message("Zaprzestano śledzenia gracza: " + name + ".\n");
    }
}

static void cmd_make_quest(object user, string cmd, string str)
{
    object state;

    if(str && !STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    state = clone_object(US_MAKE_QUEST);
    user->push_new_state(US_MAKE_QUEST);
}

static void cmd_list_quests(object user, string cmd, string str)
{
    string *quests;
    string msg;
    int i;

    quests = QUESTD->get_quests();
    if (!sizeof(quests)) {
        message("Nie ma jeszcze jakichkolwiek przygód w grze.\n");
        return;
    }

    msg = "Lista przygód w grze:\n";
    for (i = 0; i < sizeof(quests); i++) 
        msg += ralign((string)i, 5) + "   " + quests[i] + "\n";

    user->message_scroll(msg);
}

static void cmd_set_zone_attribute(object user, string cmd, string str) 
{
    string attribute, value, new_value;
    int number;

    number = -1;
    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "#%d %s", number, value) < 1) {
        user->message("Użycie: " + cmd + " #<numer strefy> <wartość>\n");
        return;
    }
    if (sscanf(cmd, "%*s_%*s_%s", attribute) < 1) {
        error("Coś poszło nie tak!");
    }

    if (number < 0 || number > ZONED->num_zones()) {
        user->message("Nieprawiłowy numer strefy!\n");
        return;
    }

    if (attribute == "weather") {
        switch (value) {
            case "brak":
                new_value = "none";
                break;
            case "pogodnie":
                new_value = "clear";
                break;
            case "pochmurno":
                new_value = "overcast";
                break;
            case "deszcz":
                new_value = "rain";
                break;
            default:
                user->message("Nieznana wartość pogody: " + value + "\n");
                return;
        }
    }
    ZONED->set_attribute(number, attribute, new_value);
    user->message("Ustawiono.\n");
}

static void cmd_reload_tags(object user, string cmd, string str) 
{
    if (str) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    TAGD->clear_tags();
    GAME_INITD->load_tagd();
    user->message("Przeładowano tagi\n");
}
