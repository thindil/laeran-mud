#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/exit.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <status.h>
#include <limits.h>

inherit access API_ACCESS;
/* prototypes */
static void cmd_list_exit(object user, string cmd, string str);

#define SPACE10 "          "
#define SPACE40 (SPACE10 + SPACE10 + SPACE10 + SPACE10)

private object room_dtd;        /* DTD for room def'n */

static void upgraded(varargs int clone);

private string ralign10(mixed num, int width)
{
    string str;

    str = SPACE10 + (string) num;
    return str[strlen(str) - width ..];
}

private string ralign40(mixed num, int width)
{
    string str;

    str = SPACE40 + (string) num;
    return str[strlen(str) - width ..];
}


static string read_entire_file(string file) {
  string ret;

  ret = read_file(file);
  if (ret == nil) { return nil; }
  if(strlen(ret) > status(ST_STRSIZE) - 3) {
    error("Plik '" + file + "' jest za duży!");
  }

  return ret;
}


/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{
  if(!find_object(US_MAKE_ROOM)) compile_object(US_MAKE_ROOM);
}

static void upgraded(varargs int clone) {
  string dtd_file;

  /* Set up room DTD */
  if(room_dtd)
    room_dtd->clear();
  else
    room_dtd = ::clone_object(UNQ_DTD);

  dtd_file = read_entire_file(MAPD_ROOM_DTD);
  room_dtd->load(dtd_file);
}

static void destructed(varargs int clone) {
    if(!clone && room_dtd) {
      destruct_object(room_dtd);
    }
}


/********** Room Functions *****************************************/

/* List MUD rooms */
static void cmd_list_room(object user, string cmd, string str) 
{
    int*    rooms;
    int     ctr, zone;
    string  tmp, type;
    object  room, phr;

    if(str) {
        str = STRINGD->trim_whitespace(str);
        str = STRINGD->to_lower(str);
    } else 
        str = "";

    sscanf(str, "%s %s", str, type);
    switch (type) {
        case "body":
            tmp = "Lista ciał";
            break;
        case "det":
            tmp = "Lista detali";
            break;
        case "port":
            tmp = "Lista przenośnych";
            break;
        case "room":
            tmp = "Lista pokoi";
            break;
        default:
            tmp = "Lista wszystkich obiektów";
            break;
    }
    if (!type)
        type = "all";

    if(str == "all") {
        user->message(tmp + " w całej grze (" + ZONED->num_zones() + " stref):\n");

        rooms = ({ });
        for(zone = 0; zone < ZONED->num_zones(); zone++) 
            rooms += MAPD->rooms_in_zone(zone);
    } else if(str == "" || str == "zone") {
        user->message(tmp + " w strefie:\n");

        room = user->get_location();
        zone = ZONED->get_zone_for_room(room);
        if(zone == -1)
            zone = 0;  /* Unzoned rooms */

        rooms = MAPD->rooms_in_zone(zone);
    }

    tmp = "";
    for(ctr = 0; ctr < sizeof(rooms); ctr++) {
        room = MAPD->get_room_by_num(rooms[ctr]);
        if (type != "all")
            if (type == "body" && !room->get_mobile())
                continue;
            else if (type == "det" && !room->get_detail_of())
                continue;
            else if (type == "port" && (!room->get_location()
                        || room->get_location() == MAPD->get_room_by_num(0)
                        || room->get_mobile()))
                continue;
            else if (type == "room" && (room->get_mobile()
                        || room->get_detail_of()
                        || room->get_location() != MAPD->get_room_by_num(0)))
                continue;
        phr = room->get_brief();
        tmp += ralign10("" + rooms[ctr], 6) + "  ";
        if (room->get_mobile()) 
            tmp += "(ciało) ";
        else if (room->get_detail_of())
            tmp += "(det)  ";
        else if (room->get_location()
                && room->get_location()!=MAPD->get_room_by_num(0))
            tmp += "(przen) ";
        else 
            tmp += "(pokój) ";
        tmp += phr->to_string(user);
        tmp += "\n";
    }

    tmp += "-----\n";
    user->message_scroll(tmp);
}


static void cmd_new_room(object user, string cmd, string str) {
  object room;
  int    roomnum, zonenum;
  string segown, zonename;

  if(!str || STRINGD->is_whitespace(str)) {
    roomnum = -1;
  } else if(sscanf(str, "%*s %*s") == 2
	    || sscanf(str, "#%d", roomnum) != 1) {
    user->message("Użycie: " + cmd + " [#numer pokoju]\n");
    return;
  }

  if(MAPD->get_room_by_num(roomnum)) {
    user->message("Już istnieje pokój o tym numerze!\n");
    return;
  }

  segown = OBJNUMD->get_segment_owner(roomnum / 100);
  if(roomnum >= 0 && segown && segown != MAPD) {
    user->message("Pokój numer " + roomnum
		  + " jest w segmencie zarezerwowanym dla nie-pokoi!\n");
    return;
  }

  room = clone_object(SIMPLE_ROOM);
  zonenum = -1;
  if(roomnum < 0) {
    zonenum = ZONED->get_zone_for_room(user->get_location());
    if(zonenum < 0) {
      LOGD->write_syslog("Dziwne, strefa ma wartość mniejszą od zera podczas robienia nowego pokoju...",
			 LOG_WARN);
      zonenum = 0;
    }
  }
  MAPD->add_room_to_zone(room, roomnum, zonenum);

  zonenum = ZONED->get_zone_for_room(room);
  if (zonenum < 0)
    {
      zonename = "nowa strefa";
    }
  else
    {
      zonename = ZONED->get_name_for_zone(zonenum);
    }
  user->message("Dodano pokój #" + (string)room->get_number()
		+ " do strefy #" + (string)zonenum
		+ " (" + zonename + ")" + ".\n");
}


static void cmd_delete_room(object user, string cmd, string str) {
  int    roomnum;
  object room;

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "%*s %*s") == 2 || !sscanf(str, "#%d", roomnum)) {
    user->message("Użycie: " + cmd + " #<numer pokoju>\n");
    return;
  }

  room = MAPD->get_room_by_num(roomnum);
  if(!room) {
    user->message("Nie ma takiego pokoju jak #" + roomnum + ".\n");
    return;
  }

  EXITD->clear_all_exits(room);

  if(room->get_detail_of()) {
    room->get_detail_of()->remove_detail(room);
  }
  if(room->get_location()) {
    room->get_location()->remove_from_container(room);
  }

  destruct_object(room);

  user->message("Pokój/przenośny #" + roomnum + " został zniszczony.\n");
}


static void cmd_save_rooms(object user, string cmd, string str) {
    string unq_str, argstr;
    mixed* rooms, *args;
    object room;
    int    ctr, zones;

    if(!str || STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + " <plik do zapisu>\n");
        user->message("   lub  " + cmd
                + " <plik do zapisu> #<numer> #<numer> #<numer>...\n");
        return;
    }

    str = STRINGD->trim_whitespace(str);
    remove_file(str + ".old");
    rename_file(str, str + ".old");  /* Try to remove & rename, just in case */

    if(sizeof(get_dir(str)[0])) {
        user->message("Nie udało się zrobić miejsca dla pliku -- nie mogę nadpisywać!\n");
        return;
    }

    if(sscanf(str, "%*s %*s") != 2) {
        zones = ZONED->num_zones();
        rooms = MAPD->rooms_in_zone(0) - ({ 0 });
        for (ctr = 1; ctr < zones; ctr++) 
            rooms += MAPD->rooms_in_zone(ctr);
    } else {
        int roomnum;

        rooms = ({ });
        sscanf(str, "%s %s", str, argstr);
        args = explode(argstr, " ");
        for(ctr = 0; ctr < sizeof(args); ctr++) {
            if(sscanf(args[ctr], "#%d", roomnum)) {
                rooms += ({ roomnum });
            } else {
                user->message("'" + args[ctr] + "' nie jest prawidłowym numerem pokoju.\n");
                return;
            }
        }
    }

    if(!rooms || !sizeof(rooms)) {
        user->message("Nie ma pokojów do zapisu!\n");
        return;
    }

    user->message("Zapisywanie pokojów: ");
    for(ctr = 0; ctr < sizeof(rooms); ctr++) {
        room = MAPD->get_room_by_num(rooms[ctr]);

        unq_str = room->to_unq_text();

        if(!write_file(str, unq_str))
            error("Nie można zapisać pokojów do pliku " + str + "!");
        user->message(".");
    }

    user->message("\nWykonane!\n");
}

mixed* parse_to_room(string room_file) {
  if(!SYSTEM())
    return nil;

  return UNQ_PARSER->unq_parse_with_dtd(room_file, room_dtd);
}

static void cmd_load_rooms(object user, string cmd, string str) {
  string room_file, argstr;
  mixed* unq_data, *tmp, *args, *rooms;
  int    iter, roomnum;

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Obecnie tylko osoba z pełnymi uprawnieniami administratorskimi "
		  + "może ładować pokoje.\n");
    return;
  }

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + " <plik do załadowania>\n");
    return;
  }

  /* If it looks like the admin specified rooms, parse them */
  if(sscanf(str, "%s %s", str, argstr) == 2) {
    rooms = ({ });
    args = explode(argstr, " ");
    for(iter = 0; iter < sizeof(args); iter++) {
      if(sscanf(args[iter], "#%d", roomnum)) {
	rooms += ({ roomnum });
      } else {
	user->message("'" + args[iter]
		      + "' nie wygląda jak numer pokoju!\n");
	return;
      }
    }
  }

  /* Check validity of file */
  str = STRINGD->trim_whitespace(str);
  if(!sizeof(get_dir(str)[0])) {
    user->message("Nie mogę odnaleźć pliku: " + str + "\n");
    return;
  }
  room_file = read_file(str);
  if(!room_file || !strlen(room_file)) {
    user->message("Błąd podczas odczytu pliku albo plik jest pusty.\n");
    return;
  }
  if(strlen(room_file) > status(ST_STRSIZE) - 3) {
    user->message("Plik z pokojami jest zbyt duży!");
    return;
  }

  tmp = UNQ_PARSER->basic_unq_parse(room_file);
  if(!tmp) {
    user->message("Nie mogę sparsować tekstu jako UNQ podczas dodawania UNQ pokoi!\n");
    return;
  }
  tmp = SYSTEM_WIZTOOL->parse_to_room(room_file);
  if(!tmp) {
    user->message("Nie mogę sparsować UNQ jako pokój podczas dodawania UNQ pokoi!\n");
    return;
  }

  if(rooms) {
    unq_data = ({ });
    for(iter = 0; iter < sizeof(tmp); iter += 2) {
      if(tmp[iter + 1][1][0] == "number") {
	roomnum = tmp[iter + 1][1][1];
	if( sizeof(({ roomnum }) & rooms) ) {
	  unq_data += tmp[iter..iter + 1];
	  rooms -= ({ roomnum });
	}
      }
    }
  } else {
    unq_data = tmp[..];
  }

  if(rooms && sizeof(rooms)) {
    string tmp;

    tmp = "Nie ma pokoi do załadowania o numerach: ";
    for(iter = 0; iter < sizeof(rooms); iter++) {
      tmp += "#" + rooms[iter] + " ";
    }
    tmp += "\n";
    user->message(tmp);

    if(sizeof(unq_data)) {
      user->message("Próbujemy pozostałe pokoje:\n\n");
    } else {
      user->message("Nie znaleziono wybranych pokojów, ignorowanie.\n");
      return;
    }
  }

  user->message("Rejestracja pokoi...\n");
  MAPD->add_dtd_unq_rooms(unq_data, str);
  user->message("Rozwiązywanie wyjść...\n");
  EXITD->add_deferred_exits();
  user->message("Wykonane.\n");
}

static void cmd_goto_room(object user, string cmd, string str) {
  int    roomnum;
  object exit, room, mob;

  if(str && sscanf(str, "#%d", roomnum)==1) {
    room = MAPD->get_room_by_num(roomnum);
    if(!room) { /* Not a room, maybe an exit? */
      exit = EXITD->get_exit_by_num(roomnum);
      room = exit->get_from_location();
    }
    if(!room) {
      user->message("Nie mogę zlokalizować pokoju #" + roomnum + "\n");
      return;
    }
  } else {
    user->message("Użycie: " + cmd + " #<numer lokacji>\n");
    return;
  }

  /* Do we really want people teleporting into a details? */
  if(room->get_detail_of()) {
    user->message("Nie mogę teleportować do detalu.\n");
    return;
  }

  mob = user->get_body()->get_mobile();

  user->message("Teleportowałeś się do " + room->get_brief()->to_string(user)
		+ ".\n");
  mob->teleport(room, 1);
  user->show_room_to_player(user->get_location());
}

/******  Exit Functions ****************************************/

static void cmd_new_exit(object user, string cmd, string str) {
  int    roomnum, dir, retcode;
  object room;
  string dirname;
  string type;

  if(str)
    retcode = sscanf(str, "%s #%d %s", dirname, roomnum, type);

  if (str && retcode == 2) type = "twoway";

  if(str && (retcode == 3 || retcode == 2)) {
    room = MAPD->get_room_by_num(roomnum);
    if(!room) {
      user->message("Nie mogę znaleźć pokoju #" + roomnum + "\n");
      return;
    }
    dir = EXITD->direction_by_string(dirname);
    if(dir == -1) {
      user->message("Nie rozpoznaję " + dirname + " jako kierunku.\n");
      return;
    }
  } else {
    user->message("Użycie: " + cmd + " <kierunek> #<numer pokoju> <typ>\n");
    return;
  }

  if(!user->get_location()) {
    user->message("Znajdujesz się w pustce więc nie możesz stworzyć tutaj"
		  + " wyjścia!\n");
    return;
  }
  if(user->get_location()->get_exit(dir)) {
    user->message("Wygląda na to, że jest już wyjście w tym"
		  + " kierunku.\n");
    return;
  }

  if (roomnum == 0) {
    user->message("Nie można linkować do pustki.\n");
    return;
  }

  if (type=="oneway" || type=="one-way" || type=="1") {
    user->message("Zaczynasz tworzyć jednokierunkowe wyjście do '"
		+ room->get_brief()->to_string(user) + "'.\n");
    EXITD->add_oneway_exit_between(user->get_location(), room, dir, -1);

  } else if (type=="twoway" || type=="two-way" || type=="2") {
    if (room->get_exit(EXITD->opposite_direction(dir))) {
      user->message("Wygląda na to, że jest już wyjście w tym kierunku w drugim pokoju.\n");
      return;
    }
    user->message("Zaczynasz tworzyć dwukierunkowe wyjście do '"
		+ room->get_brief()->to_string(user) + "'.\n");
    EXITD->add_twoway_exit_between(user->get_location(), room, dir, -1, -1);

  }
  user->message("Z powodzeniem stworzyłeś wyjście.\n");
}

static void cmd_clear_exits(object user, string cmd, string str) {
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  if(!user->get_location()) {
    user->message("Jesteś w pustce więc nie możesz wykonać " + cmd + "!\n");
    return;
  }
  EXITD->clear_all_exits(user->get_location());
}

static void cmd_remove_exit(object user, string cmd, string str) {
  int    dir;
  object exit, exit2;

  if(!str || STRINGD->is_whitespace(str) || sscanf(str, "%*s %*s") == 2) {
    user->message("Użycie: " + cmd + " <kierunek>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);
  dir = EXITD->direction_by_string(str);
  if(dir == -1) {
    user->message("'" + str + "' nie jest kierunkiem.\n");
    return;
  }

  if(!user->get_location()) {
    user->message("Jesteś w pustce więc nie możesz wykonać " + cmd + "!\n");
    return;
  }
  exit = user->get_location()->get_exit(dir);
  if(!exit) {
    user->message("Nie znaleziono wyjścia w prowadzącego na " + str + ".\n");
    return;
  }

  EXITD->clear_exit(exit);

  user->message("Wyjście " + str + " zostało zniszczone.\n");
}

void list_exits(object user, int* exits, int ctr, string tmpstr)
{
    int    dir, type, chunk_ctr;
    object exit, dest, phr, from;

    chunk_ctr = 0;
    while(chunk_ctr < 20  && (exits && ctr < sizeof(exits))) {
        for(; ctr < sizeof(exits) && chunk_ctr < 20; ctr++, chunk_ctr++) {
            exit = EXITD->get_exit_by_num(exits[ctr]);
            dest = exit->get_destination();
            from = exit->get_from_location();

            tmpstr += "Wyjście #" + exit->get_number();

            if(exit->get_brief())
                tmpstr += " (" + exit->get_brief()->to_string(user) + ") ";
            else
                tmpstr += " (brak opisu) ";

            dir = exit->get_direction();
            if(dir != -1) {
                phr = EXITD->get_name_for_dir(dir);
                if(!phr)
                    tmpstr += "niezdefiniowany kierunek " + dir;
                else
                    tmpstr += phr->to_string(user);
            } else
                tmpstr += "bez kierunku";
            /* Show "from" location as number or commentary */
            tmpstr += " z #";
            if (!from)
                tmpstr += "<brak>";
            else if (from->get_number() == -1)
                tmpstr += "<niezarejestrowany pokój>";
            else
                tmpstr += from->get_number();

            /* Show "to" location as number or commentary */
            tmpstr += " do #";
            if(!dest)
                tmpstr += "<brak>";
            else if (dest->get_number() == -1)
                tmpstr += "<niezarejestrowany pokój>";
            else
                tmpstr += dest->get_number();
            /* Show type: one-way, two-way, other */
            tmpstr += " typ: ";
            type = exit->get_exit_type();
            switch (type) {
                case 1: /* one-way */
                    tmpstr += "jednokierunkowe";
                    break;
                case 2: /* two-way */
                    tmpstr += "dwukierunkowe";
                    break;
                default: /* unknown */
                    tmpstr += type;
                    break;
            }

            /* Show "link" location as number or commentary */
            tmpstr += " link #";
            tmpstr += exit->get_link();

            if (exit->is_open())
                tmpstr += " Otwarte";
            else
                tmpstr += " Zamknięte";

            if (exit->is_openable())
                tmpstr += " Otwieralne";

            if (exit->is_container())
                tmpstr += " Pojemnik";

            if (exit->is_locked())
                tmpstr += " Zablokowane";

            if (exit->is_lockable())
                tmpstr += " Zablokowywalne";

            tmpstr += "\n";
        }
    }
    if (strlen(tmpstr) > 60000) {
        user->message(tmpstr);
        tmpstr = "";
    }
    if(exits && ctr < sizeof(exits))
        call_out("list_exits", 0, user, exits, ctr, tmpstr);
    else {
        user->message_scroll(tmpstr);
        ctr = EXITD->num_deferred_exits();
        if(ctr)
            user->message(ctr + " oczekujących odroczonych wyjść.\n");
    }
}

static void cmd_list_exits(object user, string cmd, string str) {
    int   *tmp, *segs, *exits;
    int    ctr;

    if(str && !STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + "\n");
        return;
    }

    /* Ignore cmd, str for the moment */
    segs = EXITD->get_exit_segments();
    if(!segs) {
        user->message("Nie mogę znaleźć segmentów z wyjściami!\n");
        return;
    }

    tmp = ({ });
    exits = ({ });
    for(ctr = 0; ctr < sizeof(segs); ctr++) {
        tmp = EXITD->exits_in_segment(segs[ctr]);
        if(tmp && sizeof(tmp))
            exits += tmp;
    }

    user->message("Wyjścia:\n");
    call_out("list_exits", 0, user, exits, 0, "");
}

static void cmd_add_deferred_exits(object user, string cmd, string str) {
  int num;

  num = EXITD->num_deferred_exits();
  user->message("Odroczonych wyjść: " + num + ".\n");

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Obecnie tylko osoby z pełnymi uprawnieniami administracyjnymi "
		  + "mogą rozwiązywać odroczone wyjścia i pokoje.\n");
    return;
  }

  EXITD->add_deferred_exits();
  MAPD->do_room_resolution(0);  /* Don't fully resolve, so no errors */

  num = EXITD->num_deferred_exits();
  user->message(num + " odroczonych wyjść ciągle oczekuje.\n");
  user->message(sizeof(MAPD->get_deferred_rooms())
		+ " odroczonych pokojów ciągle oczekuje.\n");
}


static void cmd_check_deferred_exits(object user, string cmd, string str) {
  int num;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(str && str != "") {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  num = EXITD->num_deferred_exits();

  user->message(num + " odroczonych wyjść ciągle oczekuje.\n");
  user->message(sizeof(MAPD->get_deferred_rooms())
		+ " odroczonych pokojów ciągle oczekuje.\n");
}
