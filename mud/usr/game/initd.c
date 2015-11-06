#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <gameconfig.h>
#include <config.h>
#include <version.h>

static mixed* load_file_with_dtd(string file_path, string dtd_path);
static void load_sould(void);
static void load_tagd(void);
static void set_up_heart_beat(void);
static int read_object_dir(string pathi, int type);

static void create(void) {
  string throwaway, mob_file;
  int i;

  /* Build game driver and set it */
  throwaway = catch (find_object(GAME_DRIVER) ? nil
		     : compile_object(GAME_DRIVER));
  if(find_object(GAME_DRIVER))
    CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Set up TagD */
  load_tagd();

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/usr/game/help");

  /* Load the SoulD with social commands */
  read_object_dir(SOCIAL_DIR, 3);

  /* Load stuff into MAPD and EXITD */
  if(read_object_dir(ROOM_DIR, 1) >= 0) {
    EXITD->add_deferred_exits();
    MAPD->do_room_resolution(1);
  } else 
    error("Can't read object files!  Dying!\n");

  /* Load the mobilefile into MOBILED */
  read_object_dir(MOB_DIR, 2);

  /* Set up heart_beat functions */
  if(!find_object(HEART_BEAT))
      compile_object(HEART_BEAT);
  HEART_BEAT->set_up_heart_beat();

  if(!find_object(COMBAT))
      compile_object(COMBAT);

  LOGD->write_syslog("Zakończono konfigurację gry! Wersja gry: " + GAME_VERSION);
}

static mixed* load_file_with_dtd(string file_path, string dtd_path) {
  string file_tmp, dtd_tmp;
  object dtd;
  mixed* dtd_unq;

  dtd_tmp = read_file(dtd_path);
  file_tmp = read_file(file_path);

  dtd = clone_object(UNQ_DTD);
  dtd->load(dtd_tmp);

  dtd_unq = UNQ_PARSER->unq_parse_with_dtd(file_tmp, dtd, file_path);
  destruct_object(dtd);

  return dtd_unq;
}

static void load_tagd(void) {
  mixed *dtd_unq;
  int    ctr, ctr2;

  dtd_unq = load_file_with_dtd("/usr/game/tagd.unq", "/usr/game/dtd/tagd.dtd");

  for(ctr = 0; ctr < sizeof(dtd_unq); ctr += 2) {
    string tag_name, tag_get, tag_set, add_func;
    int    tag_type;

    if(typeof(dtd_unq[ctr + 1]) != T_ARRAY)
      error("Internal error parsing TAGD file!");

    switch(dtd_unq[ctr]) {
    case "mobile_tag":
      add_func = "new_mobile_tag";
      break;
    case "object_tag":
      add_func = "new_object_tag";
      break;
    default:
      error("Unknown tag '" + STRINGD->mixed_sprint(dtd_unq[ctr]) + "'!");
    }

    tag_get = tag_set = nil;
    tag_type = -1;

    for(ctr2 = 0; ctr2 < sizeof(dtd_unq[ctr + 1]); ctr2++) {
      switch(dtd_unq[ctr + 1][ctr2][0]) {
      case "name":
	tag_name = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      case "type":
	tag_type = dtd_unq[ctr + 1][ctr2][1];
	break;
      case "getter":
	tag_get = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      case "setter":
	tag_set = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      default:
	error("Unrecognized label in switch for TagD UNQ: "
	      + STRINGD->mixed_sprint(dtd_unq[ctr + 1][ctr2]) + "!");
      }
    }

    call_other(TAGD, add_func, tag_name, tag_type, tag_get, tag_set);
  }
}

/* read_object_dir loads all rooms and exits from the specified directory,
   which should be in canonical Phantasmal saved format.  This is used to
   restore saved data from %shutdown and from %datadump. */
static int read_object_dir(string path, int type) {
  mixed** dir;
  int     ctr;
  string  file;

  switch (type) {
      case 1:
          dir = get_dir(path + "/zone*.unq");
          break;
      case 2:
          dir = get_dir(path + "/mobiles*.unq");
          break;
      case 3:
          dir = get_dir(path + "/socials*.unq");
          break;
      default:
          break;
  }
  if(!sizeof(dir[0])) {
    LOGD->write_syslog("Can't find any '" + path
		       + " files to load!", LOG_ERR);
    return -1;
  }

  for(ctr = 0; ctr < sizeof(dir[0]); ctr++) {
    /* Skip directories */
    if(dir[1][ctr] == -2)
      continue;

    file = read_file(path + "/" + dir[0][ctr]);
    if(!file || !strlen(file)) {
      /* Nothing was read.  Return error. */
      return -1;
    }

    switch (type) {
        case 1:
            MAPD->add_unq_text_rooms(file, ROOM_DIR + "/" + dir[0][ctr]);
            break;
        case 2:
            MOBILED->add_unq_text_mobiles(file, MOB_DIR + "/" + dir[0][ctr]);
            break;
        case 3:
            SOULD->from_unq_text(file);
            break;
        default:break;
    }
  }
}
