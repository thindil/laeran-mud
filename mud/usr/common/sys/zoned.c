/* ZoneD -- daemon that tracks zones */

#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <limits.h>
#include <status.h>

inherit dtd DTD_UNQABLE;


/* Zone Attributes */
#define ZONE_ATTR_VOLATILE         1


/* Prototypes */
void upgraded(varargs int clone);
void set_segment_zone(int segment, int zonenum);

mixed*  zone_table;
mapping segment_map;
int     player_zone;

static void create(varargs int clone) {
  if(clone)
    error("Cloning zoned is not allowed!");

  dtd::create(clone);

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);

  zone_table = ({ ({ "Unzoned", ([ ]), 0 }) });
  segment_map = ([ ]);
  player_zone = -1;

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON()) {
    set_dtd_file(ZONED_DTD);
    dtd::upgraded(clone);
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    dtd::destructed(clone);
  }
}

/******* Init Function called by INITD *********************/

/* The zonefile's contents are passed through the string
   argument.  Currently zone files must be DGD's string
   size or less. */
void init_from_file(string file) {
  if(!SYSTEM())
    return;

  if(strlen(file) > status(ST_STRSIZE) - 3)
    error("Zonefile is too large in ZONED->init_from_file!");
  from_unq_text(file);
}


/******* Functions for DTD_UNQABLE *************************/

string to_unq_text(int number)
{
    string zonetmp;

    if (!SYSTEM() && !COMMON())
        return nil;

    if (number < sizeof(zone_table)) {
        zonetmp = "~zone{\n"
                + "     ~zonenum{" + number + "}\n"
                + "     ~name{" + zone_table[number][0] + "}\n"
                + "     ~attributes{" + zone_table[number][2] + "}\n"
                + "}\n";
        return zonetmp;
    }
    else
        number -= sizeof(zone_table);

    if (number <= OBJNUMD->get_highest_segment()) {
        if (!OBJNUMD->get_segment_owner(number))
            return nil;
        zonetmp = "~segment{\n"
                + "     ~segnum{" + number + "}\n"
                + "     ~zonenum{" + segment_map[number] + "}\n"
                + "}\n";
        return zonetmp;
    }
    return nil;
}

string get_parse_error_stack(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return ::get_parse_error_stack();
}

void from_dtd_unq(mixed* unq) {
  mixed *zones, *segment_unq;
  int segnum,zonenum;
  string zonename;

  mixed *ctr;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Calling ZoneD:from_dtd_unq unprivileged!");

  ctr = unq;
  while (sizeof(ctr) > 0) {
      switch (ctr[0]) {
          case "zone":
            segment_unq = ctr[1];
            zonenum = segment_unq[0][1];
            zonename = segment_unq[1][1];
            if (zonename != "Unzoned")
                zone_table += ({ ({ zonename, ([ ]), 0 }) });
            break;
          case "segment":
            segment_unq = ctr[1];
            segnum = segment_unq[0][1];
            zonenum = segment_unq[1][1];
            set_segment_zone(segnum, zonenum);
            break;
          default:
            break;
      }
      ctr = ctr[2..];
  }
}

void write_to_file(string filename) {
  string str;
  string unq_str;

  if(previous_program() != "/usr/System/initd")
    error("Only INITD may instruct the ZONED to write files!");

  unq_str = dtd::to_unq_text();
  if(!unq_str)
    error("To_unq_text() returned nil!");
  if(!write_file(filename, unq_str)) {
    error("Error writing file!");
  }

}


/******* Regular ZONED functions ***************************/

int num_zones(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  return sizeof(zone_table);
}

string get_name_for_zone(int zonenum) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(zonenum >= 0 && sizeof(zone_table) > zonenum) {
    return zone_table[zonenum][0];
  } else {
    return nil;
  }
}

int* get_segments_in_zone(int zonenum) {
  mixed* keys;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  keys = map_indices(zone_table[zonenum][1]);
  return keys;
}

int get_segment_zone(int segment) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  return (segment_map[segment] != nil ? segment_map[segment] : -1);
}

void set_segment_zone(int segment, int zonenum) {
  if(segment < 0 || zonenum < 0 || zonenum > sizeof(zone_table))
    error("Segment or zone doesn't exist in set_segment_zone!");

  if(segment_map[segment] != nil) {
    int oldzone;
    oldzone = segment_map[segment];
    zone_table[oldzone][1][segment] = nil;
  }
  segment_map[segment] = zonenum;
  zone_table[zonenum][1][segment] = 1;
}

int get_zone_for_room(object room) {
  int roomnum, segment, zone;

  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(!room)
    error("Invalid room passed to ZoneD:get_zone_for_room!");

  roomnum = room->get_number();
  segment = roomnum / 100;
  zone = get_segment_zone(segment);

  return zone;
}

int add_new_zone( string zonename ){
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if (zonename && zonename != ""){
    int zonenum;
    zone_table += ({ ({ zonename, ([ ]), 0 }) });

    return num_zones()-1;
  } else {
    error("Illegal or (nil) zone name given to ZONED!");
  }
}

int is_zone_volatile(int zonenum) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(zonenum < 0 || zonenum > sizeof(zone_table))
    error("Invalid zone number passed to ZoneD:is_zone_volatile!");

  if(zone_table[zonenum][2] & ZONE_ATTR_VOLATILE)
    return 1;

  return 0;
}

int get_player_zone(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(player_zone < 0)
    return -1;

  return player_zone;
}

void set_player_zone(int zonenum) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(zonenum < 0 || zonenum > sizeof(zone_table))
    error("Invalid zone number passed to ZoneD:set_player_zone!");

  if(player_zone >= 0) {
    /* Set former player zone as non-volatile */
    zone_table[player_zone][2] &= ~ZONE_ATTR_VOLATILE;
  }

  player_zone = zonenum;

  if(player_zone >= 0) {
    /* Set new player zone as volatile */
    zone_table[player_zone][2] |= ZONE_ATTR_VOLATILE;
  }
}
