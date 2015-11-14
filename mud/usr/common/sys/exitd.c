#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/exit.h>
#include <phantasmal/map.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

inherit PHRASE_REPOSITORY;

private int*    exit_segments;
private mapping name_for_dir;
private mapping shortname_for_dir;
private mapping builder_directions;

/* When loading files, this lets us resolve room numbers *after* all rooms
   load... */
private mixed*  deferred_add_newexit;

/* Prototypes */
private void add_complex_exit_by_unq(int roomnum1, mixed value);
void upgraded(varargs int clone);

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  ::create(clone);

  if(!find_object(SIMPLE_EXIT))
    compile_object(SIMPLE_EXIT);

  exit_segments = ({ });

  deferred_add_newexit = ({ });

  upgraded();
}

void upgraded(varargs int clone) {
  if(!SYSTEM() && !COMMON())
    return;

  ::upgraded();

  name_for_dir = ([ DIR_NORTH : PHR("północ"),
		    DIR_SOUTH : PHR("południe"),
		    DIR_EAST : PHR("wschód"),
		    DIR_WEST : PHR("zachód"),
		    DIR_NORTHWEST : PHR("północnyzachód"),
		    DIR_NORTHEAST : PHR("północnywschód"),
		    DIR_SOUTHWEST : PHR("południowyzachód"),
		    DIR_SOUTHEAST : PHR("południowywschód"),
		    DIR_IN : PHR("do środka"),
		    DIR_OUT : PHR("na zewnątrz"),
		    DIR_UP : PHR("góra"),
		    DIR_DOWN : PHR("dół"),
		    ]);

  shortname_for_dir = ([ DIR_NORTH : PHR("pn"),
			 DIR_SOUTH : PHR("pd"),
			 DIR_EAST : PHR("w"),
			 DIR_WEST : PHR("z"),
			 DIR_NORTHWEST : PHR("pnz"),
			 DIR_NORTHEAST : PHR("pnw"),
			 DIR_SOUTHWEST : PHR("pdz"),
			 DIR_SOUTHEAST : PHR("pdw"),
			 DIR_IN : PHR("ds"),
			 DIR_OUT : PHR("nz"),
			 DIR_UP : PHR("g"),
			 DIR_DOWN : PHR("d"),
			 ]);

  /* This is the set of direction string acceptable for
     builder commands (currently) and file formats (probably forever) */
  builder_directions = ([ "północ"            : DIR_NORTH,
			  "południe"          : DIR_SOUTH,
			  "wschód"            : DIR_EAST,
			  "zachód"            : DIR_WEST,
			  "północnywschód"    : DIR_NORTHEAST,
			  "północnyzachód"    : DIR_NORTHWEST,
			  "południowywschód"  : DIR_SOUTHEAST,
			  "południowyzachód"  : DIR_SOUTHWEST,
			  "góra"              : DIR_UP,
			  "dół"               : DIR_DOWN,
			  "do środka"         : DIR_IN,
			  "na zewnątrz"       : DIR_OUT,

			  "pn"  : DIR_NORTH,
			  "pd"  : DIR_SOUTH,
			  "w"   : DIR_EAST,
			  "z"   : DIR_WEST,
			  "pnw" : DIR_NORTHEAST,
			  "pnz" : DIR_NORTHWEST,
			  "pdw" : DIR_SOUTHEAST,
			  "pdz" : DIR_SOUTHWEST,
			  "g"   : DIR_UP,
			  "d"   : DIR_DOWN,
			  ]);
}

void destructed(int clone) {
  if(SYSTEM()) {

  }
}


/* Return phrase for direction name */
object get_name_for_dir(int direction) {
  if (direction <= 0) {
    error("Ujemna wartość dla kierunku!");
  }
  return name_for_dir[direction];
}

/* Return phrase for direction short name */
object get_short_for_dir(int direction) {
  object phr;
  if (direction <= 0) {
    error("Ujemna wartość dla kierunku!");
  }
  phr = shortname_for_dir[direction];
  if(!phr) error("Nie mogę pobrać krótkiej nazwy dla kierunku " + direction);
  return phr;
}

int direction_by_string(string direc) {
  if(builder_directions[direc])
    return builder_directions[direc];

  return DIR_ERR;
}

int opposite_direction(int direction) {
  if (direction <= 0) {
    error("Ujemna wartość dla kierunku!");
  }

  if(direction % 2) {
    return direction + 1;
  }

  return direction - 1;
}

private void push_or_add_newexit(int roomnum1, mixed *value) {
  int ctr, roomnum2;
  object exit1, exit2;
  object room1, room2;

  for(ctr=0;ctr<sizeof(value);ctr++) {
    if (value[ctr][0]=="destination") {
      roomnum2 = value[ctr][1];
      room1 = MAPD->get_room_by_num(roomnum1);
      if (roomnum2 > 0) {
        room2 = MAPD->get_room_by_num(roomnum2);
        if(room1 && room2) {
          add_complex_exit_by_unq(roomnum1, value);
        } else {
          deferred_add_newexit += ({ ({ roomnum1, value }) });
        }
      } else if (room1) {
          add_complex_exit_by_unq(roomnum1, value);
      } else {
          deferred_add_newexit += ({ ({ roomnum1, value }) });
      }
    }
  }
}

void room_request_complex_exit(int roomnum1, mixed value) {
  if(previous_program() != ROOM) {
    error("Tylko ROOM może żądać stworzenia wiszącego wejścia!");
  }
  push_or_add_newexit(roomnum1, value);
}

void add_deferred_exits(void) {
  int    ctr;
  mixed* exits, *ex;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  exits = deferred_add_newexit;
  deferred_add_newexit = ({ });

  if (sizeof(exits)) {
    for(ctr = 0; ctr < sizeof(exits); ctr++) {
      ex = exits[ctr];
      push_or_add_newexit(ex[0], ex[1]);
    }
  }

}

int num_deferred_exits(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(!deferred_add_newexit) return -1;

  return sizeof(deferred_add_newexit);
}

private int allocate_exit_obj(int num, object obj) {
  int segment;

  if(num >= 0 && OBJNUMD->get_object(num))
    error("Istnieje już obiekt z numerem " + num);

  if(num != -1) {
    OBJNUMD->allocate_in_segment(num / 100, num, obj);

    /* If that succeeded, add it to exit_segments */
    if(!(sizeof( ({ num / 100 }) & exit_segments ))) {
      string tmp;

      exit_segments |= ({ num / 100 });
    }

    return num;
  }

  for(segment = 0; segment < sizeof(exit_segments); segment++) {
    num = OBJNUMD->new_in_segment(exit_segments[segment], obj);
    if(num != -1) {
      return num;
    }
  }

  segment = OBJNUMD->allocate_new_segment();

  exit_segments += ({ segment });
  num = OBJNUMD->new_in_segment(segment, obj);

  return num;
}

private void add_complex_exit_by_unq(int roomnum1, mixed value) {
  int ctr, ctr2, num1, num2, roomnum2;
  object exit1, exit2;
  object room1, room2;
  mixed nouns, adjectives;

  exit1 = clone_object(SIMPLE_EXIT);
  room1 = MAPD->get_room_by_num(roomnum1);
  exit1->set_from_location(room1);

  for(ctr=0;ctr<sizeof(value);ctr++) {
    if (value[ctr][0]=="rnumber") {
      exit1->set_number(value[ctr][1]);
    } else if (value[ctr][0]=="direction") {
      exit1->set_direction(value[ctr][1]);
    } else if (value[ctr][0]=="destination") {
        room2 = MAPD->get_room_by_num(value[ctr][1]);
        exit1->set_destination(room2);
	exit1->set_from_location(room1);
    } else if (value[ctr][0]=="return") {
      exit1->set_link(value[ctr][1]);
    } else if (value[ctr][0]=="type") {
      exit1->set_exit_type(value[ctr][1]);
    } else if (value[ctr][0]=="rdetail") {
      /* what to do? */
    } else if (value[ctr][0]=="rbdesc") {
      exit1->set_brief(value[ctr][1]);
    } else if (value[ctr][0]=="rgdesc") {
      /* exit1->set_glance(value[ctr][1]) */ ;
    } else if (value[ctr][0]=="rldesc") {
      exit1->set_look(value[ctr][1]);
    } else if (value[ctr][0]=="redesc") {
      exit1->set_examine(value[ctr][1]);
    } else if (value[ctr][0]=="rflags") {
      exit1->set_all_flags(value[ctr][1]);
    } else if(value[ctr][0]=="rnouns") {
      for(ctr2 = 0; ctr2 < sizeof(value[ctr][1]); ctr2++) {
        exit1->add_noun(value[ctr][1][ctr2]);
      }
    } else if(value[ctr][0]=="radjectives") {
      for(ctr2 = 0; ctr2 < sizeof(value[ctr][1]); ctr2++) {
        exit1->add_adjective(value[ctr][1][ctr2]);
      }
    }
  }

  room1->add_exit(exit1->get_direction(), exit1);
  num1 = allocate_exit_obj(exit1->get_number(), exit1);
}

/* note:  caller must make sure not to override existing exits!!! */
void add_twoway_exit_between(object room1, object room2, int direction,
			     int num1, int num2) {
  object exit1, exit2;
  object dir, opp_dir;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if (direction <= 0) {
    error("Ujemna wartość kierunku!");
  }

  dir = get_name_for_dir(direction);
  opp_dir = get_name_for_dir(opposite_direction(direction));

  exit1 = clone_object(SIMPLE_EXIT);
  exit2 = clone_object(SIMPLE_EXIT);

  exit1->set_destination(room2);
  exit1->set_from_location(room1);
  exit1->set_direction(direction);
  exit1->set_exit_type(ET_TWOWAY);
  exit1->set_open(TRUE);
  exit1->set_container(TRUE);

  exit2->set_destination(room1);
  exit2->set_from_location(room2);
  exit2->set_direction(opposite_direction(direction));
  exit2->set_exit_type(ET_TWOWAY);

  exit2->set_open(TRUE);
  exit2->set_container(TRUE);

  room1->add_exit(direction, exit1);
  room2->add_exit(opposite_direction(direction), exit2);

  num1 = allocate_exit_obj(num1, exit1);
  num2 = allocate_exit_obj(num2, exit2);

  if(num1 < 0 || num2 < 0) {
    error("Numery wyjść nie przypisane poprawnie!");
  }

  exit1->set_number(num1);
  exit1->set_link(num2);
  exit2->set_number(num2);
  exit2->set_link(num1);

  if(exit1->get_number() < 0
     || exit2->get_number() < 0) {
    destruct_object(exit1);
    destruct_object(exit2);
    error("Numery wyjść nie przypisane poprawnie!");
  }

  exit1->set_brief(PHRASED->new_simple_english_phrase("exit"));
  exit2->set_brief(PHRASED->new_simple_english_phrase("exit"));
}

/* note:  caller must make sure not to override existing exits!!! */
void add_oneway_exit_between(object room1, object room2, int direction,
			     int num1) {
  object exit1, dir;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if (direction <= 0) {
    error("Podano ujemną wartość kierunku!");
  }

  exit1 = clone_object(SIMPLE_EXIT);
  dir = get_name_for_dir(direction);

  exit1->set_destination(room2);
  exit1->set_from_location(room1);
  exit1->set_direction(direction);
  exit1->set_exit_type(ET_ONEWAY);
  exit1->set_open(TRUE);
  exit1->set_container(TRUE);

  room1->add_exit(direction, exit1);

  num1 = allocate_exit_obj(num1, exit1);

  if(num1 < 0 ) {
    error("Numer wyjścia nie przypisany poprawnie!");
  }

  exit1->set_number(num1);
  exit1->set_link(-1);

  if(exit1->get_number() < 0) {
    error("Numer wyjścia nie przypisany poprawnie!");
    destruct_object(exit1);
  }
}

void remove_exit(object room, object exit) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  room->remove_exit(exit);
}

void clear_exit(object exit) {
  object exit2;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if (exit->get_exit_type() == ET_TWOWAY) {
    exit2 = EXITD->get_exit_by_num(exit->get_link());
    destruct_object(exit2);
  }
  destruct_object(exit);
}

void clear_all_exits(object room) {
  object exit, exit2;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(!room)
    error("Podano nil w clear_all_exits!");

  while(exit = room->get_exit_num(0)) {
    if (exit->get_exit_type() == ET_TWOWAY) {
      exit2 = EXITD->get_exit_by_num(exit->get_link());
      destruct_object(exit2);
    }
    destruct_object(exit);
  }

}

object get_exit_by_num(int num) {
  int seg;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(num < 0) return nil;
  seg = num / 100;
  if(sizeof( ({ seg }) & exit_segments )) {
    return OBJNUMD->get_object(num);
  }

  return nil;
}

int* get_exit_segments(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return exit_segments[..];
}

int* get_all_exits(void) {
  int* exits, *tmp;
  int  iter;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  exits = ({ });
  for(iter = 0; iter < sizeof(exit_segments); iter++) {
    tmp = OBJNUMD->objects_in_segment(exit_segments[iter]);
    if(tmp)
      exits += tmp;
  }

  if(!sizeof(exits))
    return nil;

  return exits;
}

int* exits_in_segment(int seg) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(sizeof( ({ seg}) & exit_segments )) {
    return OBJNUMD->objects_in_segment(seg);
  }

  return nil;
}
