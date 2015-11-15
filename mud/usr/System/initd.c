#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/version.h>

#include <phantasmal/log.h>
#include <phantasmal/version.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/timed.h>

#include <status.h>
#include <type.h>
#include <config.h>
#include <gameconfig.h>

inherit COMMON_AUTO;
inherit access API_ACCESS;
inherit rsrc   API_RSRC;

#define HELP_DTD   "/usr/System/sys/help.dtd"
#define THE_VOID   "/usr/System/obj/void"

/* How many objects can be saved to file in a single call_out? */
#define SAVE_CHUNK   10

private string pending_callback;
private int __sys_suspended;
private int errors_in_writing;


/* Prototypes */
private void suspend_system();
private void release_system();
static  void __co_write_rooms(object user, int* objects, int* zones,
			      int ctr, int zone_ctr, string roomfile,
			      string mobfile, string zonefile,
			      int filesize, int extension, string socialdir, string questdir);
static  void __co_write_mobs(object user, int* objects, int ctr,
			     string mobfile, string zonefile, int filesize,
                 int extension, string socialdir, string questdir);
static  void __co_write_zones(object user, int* objects, int ctr,
			      string zonefile, int filesize, int extension,
                  string socialdir, string questdir);
static  void __co_write_users(object user, int ctr);
static  void __co_write_socials(object user, string socialdir,
                  int filesize, int extension, int ctr, string questdir);
static  void __co_write_quests(object user, string questdir, int filesize,
                  int extension, int ctr);
static  void __co_write_tags(object user, int filesize, int extension,
                  int ctr);
static  void __co_write_banned(object user);
static  void __reboot_callback(void);
static  void __shutdown_callback(void);
        void set_path_special_object(object new_obj);


static int delete_directory(string dirname) {
  mixed **dirlisting;
  int     ctr;

  dirlisting = get_dir(dirname + "/*");
  if(sizeof(dirlisting[0]) == 0) {
    /* Nothing.  Maybe it was a file and not a directory? */

    return remove_file(dirname);
  }
  for(ctr = 0; ctr < sizeof(dirlisting[0]); ctr++) {
    if(dirlisting[1][ctr] == -2) {
      delete_directory(dirname + "/" + dirlisting[0][ctr]);
    } else {
      /* Don't check the return code, we wouldn't do anything
	 different anyway. */
      remove_file(dirname + "/" + dirlisting[0][ctr]);
    }
  }

  return remove_dir(dirname);
}

static int read_zones_dir(void)
{
    mixed **dir;
    int i;
    string file;

    dir = get_dir(ZONE_DIR + "/zones*.unq");
    if (!sizeof(dir[0]))
        return -1;

    for (i = 0; i < sizeof(dir[0]); i++) {
        if (dir[1][i] == -2)
            continue;
        file = read_file(ZONE_DIR + "/" + dir[0][i]);
        if (!file || !strlen(file))
            return -1;
        ZONED->init_from_file(file);
    }
    return 0;
}

static void create(varargs int clone)
{
  object driver, obj, the_void;
  string zone_file, mapd_dtd, help_dtd, mobfile_dtd;
  string bind_dtd;
  int major, minor, patch;

  /* First things first -- this release needs one of the
     latest versions of DGD, so let's make sure. */

  /* Phantasmal specifically requires a fix from DGD version 1.2.57.  This
     checks for that fix. */
  if(sscanf(status()[ST_VERSION], "DGD %d.%d.%d", major, minor, patch) != 3) {
    patch = 0;
    if(sscanf(status()[ST_VERSION], "DGD %d.%d", major, minor) != 2) {
      minor = 0;
      if(sscanf(status()[ST_VERSION], "DGD %d", major) != 1) {
        error("DGD driver version unparseable!");
      }
    }
  }
  
  if((major == 1 && minor < 4)) {
    error("Need to upgrade to DGD version 1.5 or higher!");
  } else if (major > 1 || (major == 1 && minor > 5)) {
    DRIVER->message("This version of Phantasmal is not tested\n");
    DRIVER->message("with DGD beyond 1.5.X.  Please upgrade Phantasmal!\n");
    error("Upgrade Phantasmal!");
  }

  /* DGD keeps a separate version number for the Kernel Library.  We
     check for this instead of the specific DGD patch number so that
     Phantasmal doesn't give a big nasty error message every time a
     new tiny bugfix comes out.  However, Phantasmal *will* give a
     disclaimer when the Kernel Library changes.  Seems like a good
     compromise. */
  if(sscanf(KERNEL_LIB_VERSION, "%d.%d.%d", major, minor, patch) != 3) {
    patch = 0;
    if(sscanf(KERNEL_LIB_VERSION, "%d.%d", major, minor) != 2) {
      minor = 0;
      if(sscanf(KERNEL_LIB_VERSION, "%d", major) != 1) {
        error("Kernel Library version unparseable!");
      }
    }
  }
  if(major < 1
     || (major == 1 && minor < 3)) {
    error("Need to upgrade to Kernel Library version 1.3 or higher!");
  } else if (major > 1 || (major == 1 && minor > 6)) {
    DRIVER->message("This version of Phantasmal is not tested\n");
    DRIVER->message("with Kernel Library beyond 1.6.  Please upgrade Phantasmal!\n");
    error("Upgrade Phantasmal!");
  } else if (minor >= 3 && patch > 3) {
    DRIVER->message("This is a very new Kernel Library version, or at\n");
    DRIVER->message("least newer than this version of Phantasmal.  If\n");
    DRIVER->message("you have problems, please upgrade Phantasmal!\n");
  }

  access::create();
  rsrc::create();

  /* To do things like compile objects from these directories, we need
     them to be users from the rsrcd's point of view. */
  rsrc::add_owner("common");
  rsrc::add_owner("game");

  /* Give objects more time to call upgraded() if they need it since
     they may be rereading large files all at once.  Currently this
     doesn't vary per user. */
  rsrc::set_rsrc("upgrade ticks",
		 rsrc::query_rsrc("ticks")[RSRC_MAX] * 4,
		 0, 0);

  /* Set this to enable SSHD to do its thing.  Otherwise it can't get
     enough ticks. */
  rsrc::rsrc_set_limit("System", "ticks", 3000000);

  access::set_global_access("common", READ_ACCESS);

  driver = find_object(DRIVER);

  /* driver->message("Loading system objects...\n"); */

  /* Start LOGD and log MUD startup */
  if(!find_object(LOGD)) { compile_object(LOGD); }
  /* Channels aren't set yet... */
  LOGD->write_syslog("\n-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n"
		     + "Starting Phantasmal v" + PHANTASMAL_VERSION
		     + "...\n"
		     + "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");

  /* Compile, find and install the Errord */
  if(!find_object(ERRORD)) { compile_object(ERRORD); }
  driver->set_error_manager(find_object(ERRORD));

  /* Compile, find and install the Objectd */
  if(!find_object(OBJECTD)) { compile_object(OBJECTD); }
  driver->set_object_manager(find_object(OBJECTD));
  OBJECTD->do_initial_obj_setup();

  /* Start the ConfigD so that the new AUTO object will be included */
  if(!find_object(CONFIGD)) compile_object(CONFIGD);

  /* Compile the StringD for use by LogD and HelpD */
  if(!find_object(STRINGD)) { compile_object(STRINGD); }

  /* Start up logging channels in the LogD */
  LOGD->start_channels();

  /* Start up ChannelD, TimeD and SoulD so that they'll be available
     to GAME_INITD */
  if(!find_object(CHANNELD)) 
      compile_object(CHANNELD);
  if(!find_object(SOULD)) 
      compile_object(SOULD);
  if(!find_object(TIMED)) 
      compile_object(TIMED); 


  /* Compile, find and install the TelnetD */
  if(!find_object(TELNETD)) { compile_object(TELNETD); }
  "/kernel/sys/userd"->set_telnet_manager(0,find_object(TELNETD));

  /* MUDCLIENTD manages the first binary port */
  if(!find_object(MUDCLIENTD)) { compile_object(MUDCLIENTD); }
  "/kernel/sys/userd"->set_binary_manager(0, find_object(MUDCLIENTD));

  /* Compile the Phrase manager (before HelpD) */
  if(!find_object(PHRASED)) { compile_object(PHRASED); }

  /* Set up online help */
  if(!find_object(HELPD)) { compile_object(HELPD); }

  help_dtd = read_file(HELP_DTD);
  if(!help_dtd)
    error("Can't load file " + HELP_DTD + "!");

  HELPD->load_help_dtd(help_dtd);

  /* Compile the Objnumd and ZoneD */
  if(!find_object(OBJNUMD)) { compile_object(OBJNUMD); }
  if(!find_object(ZONED)) { compile_object(ZONED); }

  bind_dtd = read_file(BIND_DTD);
  if (!bind_dtd) {
    error("Can't read file " + BIND_DTD + "!");
  }

  /* Load zone name list information -- BEFORE first call to
     any MAPD function. */ 
  if (read_zones_dir() == -1) {
      DRIVER->message("Can't read zone list!  Starting blank!\n");
      LOGD->write_syslog("Can't read zone list!  Starting blank!\n", LOG_WARN);
  }

  /* Start appropriate daemons for object, mobile and zone loading */
  if(!find_object(MAPD)) { compile_object(MAPD); }
  if(!find_object(EXITD)) { compile_object(EXITD); }
  if(!find_object(MOBILED)) { compile_object(MOBILED); }
  if(!find_object(TAGD)) { compile_object(TAGD); }

  /* Load command parser *before* loading objects/rooms/etc!  It needs
   * to be around to receive noun/adj/etc registration. */
  if (!find_object(PARSED)) { compile_object(PARSED); }

  mapd_dtd = read_file(MAPD_ROOM_DTD);
  if(!mapd_dtd)
    error("Can't read file " + MAPD_ROOM_DTD + "!");
  MAPD->init(mapd_dtd, bind_dtd);

  /* Set up The Void (room #0) */
  if(!find_object(THE_VOID)) { compile_object(THE_VOID); }
  the_void = clone_object(THE_VOID);
  if(!the_void)
    error("Error occurred while cloning The Void!");
  MAPD->add_room_to_zone(the_void, 0, 0);

  /* Set up the MOBILED */
  MOBILED->init();

  /* Make sure ConfigD is set up for scripting */
  CONFIGD->set_path_special_object(nil);

  /* Now delegate to the initd in /usr/game, if any. */
  if(read_file(GAME_INITD + ".c", 0, 1) != nil) {
    catch {
      if(!find_object(GAME_INITD))
	compile_object(GAME_INITD);

      call_other_unprotected(find_object(GAME_INITD), "???");
    } : {
      error("Error in GAME_INITD:create()!");
    }
  }

  TIMED->set_heart_beat(TIMED_ONE_HOUR, "statedump");
  TIMED->set_heart_beat(TIMED_ONE_DAY, "save_mud_data", nil, ROOM_DIR, MOB_DIR, ZONE_DIR, SOCIAL_DIR, QUEST_DIR, nil);

  ERRORD->done_with_init();
}

void statedump(void)
{
    dump_state();
}

void save_mud_data(object user, string room_dirname, string mob_dirname,
		   string zone_dirname, string social_dirname, string quest_dirname, 
           string callback) 
{
    int   *objects, *save_zones;
    int    cohandle, iter;
    mixed  tmp;

    ACCESSD->save();

    if(!SYSTEM() && previous_program() != TIMED) {
        error("Only privileged code can call save_mud_data!");
        return;
    }

    if(pending_callback) {
        error("Somebody else is already saving!");
    }
    pending_callback = callback;

    if(__sys_suspended) {
        LOGD->write_syslog("System was still suspended! (unsuspending)",
                LOG_ERROR);
        release_system();
    }

    errors_in_writing = 0;

    LOGD->write_syslog("Writing World Data to files...", LOG_NORMAL);
    LOGD->write_syslog("Rooms: '" + room_dirname + "/*', Mobiles: '"
            + mob_dirname + "/*', Zones: '"
            + zone_dirname + "/*'", LOG_VERBOSE);

    delete_directory(room_dirname + ".old");
    rename_file(room_dirname, room_dirname + ".old");
    delete_directory(room_dirname);
    make_dir(room_dirname);

    LOGD->write_syslog("Writing rooms to files " + room_dirname, LOG_VERBOSE);

    objects = MAPD->rooms_in_zone(0);
    if(!objects) objects = ({ });
    objects -= ({ 0 });

    save_zones = ({ });
    for(iter = 0; iter < ZONED->num_zones(); iter++) {
        save_zones += ({ iter });
    }

    cohandle = call_out("__co_write_rooms", 0, user, objects, save_zones,
            0, 0, room_dirname, mob_dirname, zone_dirname, 0, -1, social_dirname,
            quest_dirname);
    if(cohandle < 1) {
        error("Can't schedule call_out to save objects!");
    } else {
        suspend_system();
    }
}

void prepare_reboot(void)
{
  if(previous_program() != SYSTEM_WIZTOOLLIB
     && previous_program() != DRIVER)
    error("Can't call prepare_reboot from there!");

  if(find_object(LOGD)) {
    LOGD->write_syslog("Zapisywanie stanu gry...", LOG_NORMAL);
  }

  ACCESSD->save();

  /* save_mud_data(nil, "__reboot_callback"); */
}

void prepare_shutdown(void)
{
  if(previous_program() != SYSTEM_WIZTOOLLIB
     && previous_program() != DRIVER)
    error("Can't call prepare_shutdown from there!");

  if(find_object(LOGD)) {
    LOGD->write_syslog("Shutting down MUD...", LOG_NORMAL);
  }

  save_mud_data(this_user(), ROOM_DIR, MOB_DIR, ZONE_DIR, SOCIAL_DIR, QUEST_DIR,
		"__shutdown_callback");
}

void force_shutdown(void) {
  if(previous_program() != SYSTEM_WIZTOOLLIB)
    error("Nope, you can't just shut down the system.  Nice try.");

  ::shutdown();
}

void reboot(void)
{
    if(find_object(LOGD)) 
        LOGD->write_syslog("Rebooting!", LOG_NORMAL);
  
}

void set_path_special_object(object new_obj) {
  if(previous_program() == CONFIGD) {
    OBJECTD->set_path_special(new_obj);
  }
}




/********* Helper and callout functions ***********************/

/* suspend_system and release_system based on
   /usr/System/sys/objectd.c */

/*
  Suspend_system suspends network input, new logins and callouts
  except in this object.  (idea stolen from Geir Harald Hansen's
  ObjectD).  This will need to be copied to any and every object
  that suspends callouts -- the RSRCD checks previous_object()
  to find out who *isn't* suspended.  TelnetD only suspends
  new incoming network activity.
*/
private void suspend_system() {
  if(__sys_suspended) {
    LOGD->write_syslog("System already suspended...", LOG_ERROR);
  }
  __sys_suspended = 1;
  RSRCD->suspend_callouts();
  TELNETD->suspend_input(0);  /* 0 means "not shutdown" */
  MUDCLIENTD->suspend_input(0);  /* 0 means "not shutdown" */
}

/*
  Releases everything that suspend_system suspends.
*/
private void release_system() {
  if(!__sys_suspended) {
    LOGD->write_syslog("System not suspended, won't unsuspend.", LOG_ERROR);
    return;
  }
  __sys_suspended = 0;
  RSRCD->release_callouts();
  TELNETD->release_input();
  MUDCLIENTD->release_input();
  pending_callback = nil;
}


static void __co_write_rooms(object user, int* objects, int* save_zones,
			     int ctr, int zone_ctr, string roomdir,
			     string mobfile, string zonefile, int filesize,
			     int extension, string socialdir, string questdir)
{
    string unq_str, roomfile, err;
    object obj;
    int    chunk_ctr, index;
    string *file_ext;

    file_ext = ({ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" });
    if (extension == -1)
        roomfile = roomdir + "/zone" + save_zones[zone_ctr] + ".unq";
    else
        roomfile = roomdir + "/zone" + save_zones[zone_ctr] + file_ext[extension] + ".unq";

    catch {
        chunk_ctr = 0;
        while(chunk_ctr < SAVE_CHUNK
                && ((objects && ctr < sizeof(objects))
                    || zone_ctr < sizeof(save_zones))) {

            /* Save up to SAVE_CHUNK objects from the objects array. */
            for(; ctr < sizeof(objects) && chunk_ctr < SAVE_CHUNK;
                    ctr++, chunk_ctr++) {
                obj = MAPD->get_room_by_num(objects[ctr]);

                err = catch(unq_str = obj->to_unq_text());

                filesize += strlen(unq_str);
                if (filesize > (status(ST_STRSIZE) - 1000)) {
                    extension ++;
                    roomfile = roomdir + "/zone" + save_zones[zone_ctr] + file_ext[extension] + ".unq";
                    filesize = strlen(unq_str);
                }

                if(err || !write_file(roomfile, unq_str)) {
                    LOGD->write_syslog("Couldn't write room " + objects[ctr]
                            + " to file!");
                    errors_in_writing = 1;
                }
            }

            /* Done with this zone with time to spare?  Great, move onto the
               next one. */
            while((!objects || ctr >= sizeof(objects))
                    && zone_ctr < sizeof(save_zones)) {

                zone_ctr++;

                if(zone_ctr < sizeof(save_zones)) {
                    roomfile = roomdir + "/zone" + save_zones[zone_ctr] + ".unq";
                    objects = MAPD->rooms_in_zone(save_zones[zone_ctr]);
                    ctr = 0;
                    filesize = 0;
                    extension = -1;
                }
            }

        }

        if((objects && ctr < sizeof(objects)) || zone_ctr < sizeof(save_zones)) {
            /* Still saving rooms... */
            if(call_out("__co_write_rooms", 0, user, objects, save_zones,
                        ctr, zone_ctr, roomdir, mobfile, zonefile, filesize, 
                        extension, socialdir, questdir) < 1) 
                error("Can't schedule call_out to continue writing rooms!");
            return;
        }

        /* Done with rooms, start on mobiles */
        LOGD->write_syslog("Writing mobiles to dir " + mobfile, LOG_VERBOSE);
        delete_directory(mobfile + ".old");
        rename_file(mobfile, mobfile + ".old");
        delete_directory(mobfile);
        make_dir(mobfile);

        objects = MOBILED->all_mobiles();
        if(call_out("__co_write_mobs", 0, user, objects, 0, mobfile,
                    zonefile, 0, 0, socialdir, questdir) < 1) {
            error("Can't schedule call_out to start writing mobiles!");
        }
    } : {
        release_system();
        if(user) user->message("Error writing rooms!\n");
        error("Error writing rooms!");
    }
}

static void __co_write_mobs(object user, int* objects, int ctr,
			    string mobsdir, string zonefile, int filesize, int extension,
                string socialdir, string questdir) {
  string unq_str, err, mobfile;
  object obj;
  int    chunk_ctr;

    if (extension == 0)
        mobfile = mobsdir + "/mobiles.unq";
    else
        mobfile = mobsdir + "/mobiles" + extension + ".unq";

  catch {
    for(chunk_ctr = 0; ctr < sizeof(objects) && chunk_ctr < SAVE_CHUNK;
	ctr++, chunk_ctr++) {
      obj = MOBILED->get_mobile_by_num(objects[ctr]);

      err = catch(unq_str = obj->to_unq_text());
      filesize += strlen(unq_str);
      if (filesize > (status(ST_STRSIZE) - 1000)) {
          extension ++;
          mobfile = mobsdir + "/mobiles" + extension + ".unq";
          filesize = strlen(unq_str);
      }

      if(err || !write_file(mobfile, unq_str)) {
	LOGD->write_syslog("Couldn't write mobile " + objects[ctr]
			   + " to file!");
	errors_in_writing = 1;
      }
    }

    if(ctr < sizeof(objects)) {
      /* Still saving mobiles... */
      if(call_out("__co_write_mobs", 0, user, objects, ctr,
		  mobsdir, zonefile, filesize, extension, socialdir, questdir) < 1) {
	error("Can't schedule call_out to continue writing mobiles!");
      }
      return;
    }
    delete_directory(zonefile + ".old");
    rename_file(zonefile, zonefile + ".old");
    delete_directory(zonefile);
    make_dir(zonefile);

    /* Done with mobiles, start on zones */
    LOGD->write_syslog("Writing zones to file " + zonefile, LOG_VERBOSE);

    if(call_out("__co_write_zones", 0, user, objects, 0, zonefile, 0, 0, socialdir, questdir) < 1) {
       error("Can't schedule call_out to start writing zones!");
     }
  } : {
    release_system();
    if(user) user->message("Error writing mobiles!\n");
    error("Error writing mobiles!");
  }
}

static void __co_write_zones(object user, int* objects, int ctr,
			     string zonedir, int filesize, int extension,
                 string socialdir, string questdir) {
    string unq_str, zonefile;
    int chunk_ctr, size;

    if (extension == 0)
        zonefile = zonedir + "/zones.unq";
    else
        zonefile = zonedir + "/zones" + extension + ".unq";

    size = ZONED->num_zones() + OBJNUMD->get_highest_segment() + 1;

    catch {
        for (chunk_ctr = 0; ctr < size && chunk_ctr < SAVE_CHUNK; ctr++, chunk_ctr++) {
            unq_str = ZONED->to_unq_text(ctr);
            filesize += strlen(unq_str);
            if (filesize > (status(ST_STRSIZE) - 1000)) {
                extension ++;
                zonefile = zonedir + "/zones" + extension + ".unq";
                filesize = strlen(unq_str);
            }
            write_file(zonefile, unq_str);
        }
    } : {
        release_system();
        if(user) user->message("Error writing zones!\n");
        error("Error writing zones!");
    }

    if (ctr < size)
    {
        if(call_out("__co_write_zones", 0, user, objects, ctr, zonedir, filesize, extension, socialdir, questdir) < 1) 
            error("Can't schedule call_out to start writing zones!");
        return;
    }

    if(errors_in_writing) {
        errors_in_writing = 0;

        release_system();

        if(user) user->message("Errors in writing saved data!\r\n");

        return;
    }
    delete_directory(socialdir + ".old");
    rename_file(socialdir, socialdir + ".old");
    delete_directory(socialdir);
    make_dir(socialdir);

    /* Done with zones, start on socials */
    LOGD->write_syslog("Writing socials to files.", LOG_VERBOSE);

    if(call_out("__co_write_socials", 0, user, socialdir, 0, 0, 0, questdir) < 1)
    {
        release_system();
        error("Can't schedule call_out to start writing users!");
    }
}

static void __co_write_socials(object user, string socialdir,
                  int filesize, int extension, int ctr, string questdir) 
{
    string unq_str, socialfile;
    int chunk_ctr;
    
    if (extension == 0)
        socialfile = socialdir + "/socials.unq";
    else
        socialfile = socialdir + "/socials" + extension + ".unq";

    catch {
        for (chunk_ctr = 0; ctr < SOULD->num_socials() && chunk_ctr < SAVE_CHUNK; ctr ++, chunk_ctr ++) {
            unq_str = SOULD->to_unq_text(ctr);
            filesize += strlen(unq_str);
            if (filesize > (status(ST_STRSIZE) - 1000)) {
                extension ++;
                socialfile = socialdir + "/socials" + extension + ".unq";
                filesize = strlen(unq_str);
            }
            write_file(socialfile, unq_str);
        }
    } : {
        release_system();
        if (user) user->message("Error writing socials!\n");
        error("Error writing socials!");
    }

    if (ctr < SOULD->num_socials()) {
        if (call_out("__co_write_socials", 0, user, socialdir, filesize, extension, ctr, questdir) < 1)
            error("Can't schedule call_out to start writing socials!");
        return;
    }

    delete_directory(questdir + ".old");
    rename_file(questdir, questdir + ".old");
    delete_directory(questdir);
    make_dir(questdir);
    /* Done with socials, start on quests */
    LOGD->write_syslog("Writing quests to files.", LOG_VERBOSE);

    if(call_out("__co_write_quests", 0, user, questdir, 0, 0, 0) < 1)
    {
        release_system();
        error("Can't schedule call_out to start writing users!");
    }
}

static void __co_write_quests(object user, string questdir, int filesize,
                int extension, int ctr)
{
    string unq_str, questfile;
    int chunk_ctr;
    
    if (extension == 0)
        questfile = questdir + "/quests.unq";
    else
        questfile = questdir + "/quests" + extension + ".unq";

    catch {
        for (chunk_ctr = 0; ctr < QUESTD->num_quests() && chunk_ctr < SAVE_CHUNK; ctr ++, chunk_ctr ++) {
            unq_str = QUESTD->to_unq_text(ctr);
            filesize += strlen(unq_str);
            if (filesize > (status(ST_STRSIZE) - 1000)) {
                extension ++;
                questfile = questdir + "/quests" + extension + ".unq";
                filesize = strlen(unq_str);
            }
            write_file(questfile, unq_str);
        }
    } : {
        release_system();
        if (user) user->message("Error writing quests!\n");
        error("Error writing quests!");
    }

    if (ctr < QUESTD->num_quests()) {
        if (call_out("__co_write_quests", 0, user, questdir, filesize, extension, ctr) < 1)
            error("Can't schedule call_out to start writing quests!");
        return;
    }
    /* Done with quests, start on users */
    LOGD->write_syslog("Writing users to files.", LOG_VERBOSE);

    if(call_out("__co_write_users", 0, user, 0) < 1)
    {
        release_system();
        error("Can't schedule call_out to start writing users!");
    }
}

static void __co_write_users(object user, int ctr)
{
    object *users;
    int i;

    users = users();
    catch {
        for (i = 0; ctr < sizeof(users) && i < SAVE_CHUNK; i++, ctr++)
            users[ctr]->save_user_to_file();
    } : {
        release_system();
        if (user) user->message("Error writing users!\n");
        error("Error writing users!");
    }

    if (ctr < sizeof(users)) {
        if (call_out("__co_write_users", 0, user, ctr) < 1)
            error("Can't schedule call_out to start writing users!");
        return;
    }

    /* Done with users, start on tags */
    LOGD->write_syslog("Writing tags to file.", LOG_VERBOSE);
    remove_file("/usr/game/tagd.unq");

    if(call_out("__co_write_tags", 0, user, 0, 0, 0) < 1)
    {
        release_system();
        error("Can't schedule call_out to start writing tags!");
    }
}

static void __co_write_tags(object user, int filesize, int extension, int ctr)
{
    string unq_str, tagfile;
    int chunk_ctr;

    if (extension == 0)
        tagfile = "/usr/game/tagd.unq";
    else
        tagfile = "/usr/game/tagd" + extension + ".unq";

    catch {
        for (chunk_ctr = 0; ctr < TAGD->num_tags() && chunk_ctr < SAVE_CHUNK; ctr ++, chunk_ctr ++) {
            unq_str = TAGD->to_unq_text(ctr);
            filesize += strlen(unq_str);
            if (filesize > (status(ST_STRSIZE) - 1000)) {
                extension ++;
                tagfile = "/usr/game/tagd" + extension + ".unq";
                filesize = strlen(unq_str);
            }
            write_file(tagfile, unq_str);
        }
    } : {
        release_system();
        if (user) user->message("Error writing tags!\n");
        error("Error writing tags!");
    }

    if (ctr < TAGD->num_tags()) {
        if (call_out("__co_write_tagd", 0, user, filesize, extension, ctr) < 1)
            error("Can't schedule call_out to start writing tags!");
        return;
    }

    /* Done with tagss, start on banned ip */
    LOGD->write_syslog("Writing bans to file.", LOG_VERBOSE);

    if(call_out("__co_write_banned", 0, user) < 1)
    {
        release_system();
        error("Can't schedule call_out to start writing banned!");
    }
}

static void __co_write_banned(object user)
{
    string bans;

    if (sizeof(GAME_DRIVER->get_banned_ip())) {
        bans = implode(GAME_DRIVER->get_banned_ip(), ", ");
        remove_file("/usr/game/text/bans.txt");
        write_file("/usr/game/text/bans.txt", bans);
    }

    WORLDD->save_world_data();

    /* This is the callback from %shutdown or %reboot or whatever,
       it's the function to call after all data has successfully
       been saved. */
    catch {
        if(pending_callback) {
            call_other(this_object(), pending_callback);
            pending_callback = nil;
        }
    } : {
        release_system();
        if(user) user->message("Error calling callback!\n");
        error("Error calling callback!");
    }

    release_system();
    LOGD->write_syslog("Finished writing saved data...", LOG_NORMAL);
    if(user) user->message("Finished writing data.\n"); 
}

static void __shutdown_callback(void) {
  ::shutdown();
}

static void __reboot_callback(void) {
  /* Dumping of state starts happening before we get this notification,
     so don't explicitly dump state again... */
}
