/* ZoneD -- daemon that tracks zones */

#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <limits.h>
#include <status.h>

inherit dtd DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);
void set_segment_zone(int segment, int zonenum);

mixed*  zone_table;
mapping segment_map;

static void create(varargs int clone) {
  if(clone)
    error("Nie można sklonować ZONED!");

  dtd::create(clone);

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);

  zone_table = ({ ({ "Unzoned", ([ ]), ([ "weather": "none" ]) }) });
  segment_map = ([ ]);

  upgraded();
}

void upgraded(varargs int clone) 
{
    if(!SYSTEM() && !COMMON()) 
        return;
    set_dtd_file(ZONED_DTD);
    dtd::upgraded(clone);
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
    error("Plik stref jest za duży w ZONED->init_from_file!");
  from_unq_text(file);
}


/******* Functions for DTD_UNQABLE *************************/

string serialize_list(mapping value)
{
    int i;
    string result;
    string *indices;

    indices = map_indices(value);
    result = "";
    for (i = 0; i < sizeof(indices); i++) {
        result += indices[i] + ":" + value[indices[i]];
        if ((i + 1) < sizeof(indices))
            result += ", ";
    }

    return result;
}

string to_unq_text(int number)
{
    string zonetmp;

    if (!SYSTEM() && !COMMON())
        return nil;

    if (number < sizeof(zone_table)) {
        zonetmp = "~zone{\n"
                + "     ~zonenum{" + number + "}\n"
                + "     ~name{" + zone_table[number][0] + "}\n";
        if (map_sizeof(zone_table[number][2]))
            zonetmp += "     ~attributes{" + serialize_list(zone_table[number][2]) + "}\n";
        zonetmp += "}\n";
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

void from_dtd_unq(mixed* unq) 
{
    mixed *zones, *segment_unq;
    int segnum, zonenum, i;
    string zonename;
    string *pairs, *pair;
    mixed *ctr;
    mapping attributes;

    if(!SYSTEM() && !COMMON() && !GAME())
        error("Wywołanie ZoneD:from_dtd_unq bez uprawnień!");

    ctr = unq;
    while (sizeof(ctr) > 0) {
        switch (ctr[0]) {
            case "zone":
                segment_unq = ctr[1];
                zonenum = segment_unq[0][1];
                zonename = segment_unq[1][1];
                attributes = ([ "weather": "none" ]);
                if (sizeof(segment_unq) > 2) {
                    pairs = explode(segment_unq[2][1], ", ");
                    for (i = 0; i < sizeof(pairs); i++) {
                        pair = explode(pairs[i], ":");
                        attributes[pair[0]] = pair[1];
                    }
                } 
                if (zonename != "Unzoned")
                    zone_table += ({ ({ zonename, ([ ]), attributes }) });
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
    error("Segment lub strefa nie istnieją w set_segment_zone!");

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
    error("Nieprawidłowy pokój przekazany do ZoneD:get_zone_for_room!");

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
    zone_table += ({ ({ zonename, ([ ]), ([ "weather": "none" ]) }) });

    return num_zones()-1;
  } else {
    error("Nieprawidłowa lub (nil) nazwa strefy podana do ZONED!");
  }
}

string get_attribute(int zonenum, string attribute)
{
    if(!SYSTEM() && !COMMON() && !GAME())
        return nil;

    return zone_table[zonenum][2][attribute];
}

void set_attribute(int zonenum, string attribute, string value)
{
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    zone_table[zonenum][2][attribute] = value;
}
