#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

#include <type.h>

/* room.c:

   A basic MUD room or object with standard trimmings.  This includes
   portables and containers, though not exits.
*/

inherit obj OBJECT;

private mixed* exits;

private int  pending_location;
private int* pending_parents;
private int  pending_detail_of;
private int* pending_removed_details;
private object* pending_removed_nouns;
private object* pending_removed_adjectives;

/* Flags */
/* The objflags field contains a set of boolean object flags */
#define OF_CONTAINER          1
#define OF_OPEN               2
#define OF_OPENABLE           8
#define OF_WEARABLE          128
#define OF_DRESSED           256


private int objflags;

/* These numbers will be used to determine what a player can carry,
   and what objects can fit into what other objects.  Weight is
   in units of kilograms, and volume is in cubic decimeters
   (note 1 cu meter is 1000 sq decimeters).  Length represents the
   greatest extent of the longest axis, and is in units of
   centimeters.  The capacities are the largest values that are
   acceptable for objects put into this object.  Weight and
   volume are totalled among all objects put inside and compared to
   the capacity, while length is only compared to make sure it's
   no more than the capacity -- a quiver of arrows can accept a very
   large number of arrows, even though they're all of the maximum
   acceptable length. */
private float weight, volume, length;
private float weight_capacity, volume_capacity, length_capacity;
private int damage, armor, price, hp, combat_rating, quality, durability, cur_durability;
private int* wearlocations;
private string *body_locations;
private string skill, damage_type, craft_skill;
private mapping damage_res;

/* These are the total current amount of weight and volume
   being held in the object. */
private float current_weight, current_volume;


#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  obj::create(clone);
  if(clone) {
    exits = ({ });

    current_weight = 0.0;
    current_volume = 0.0;
    durability = 100;
    cur_durability = 100;

    weight = volume = length = -1.0;
    weight_capacity = volume_capacity = length_capacity = -1.0;
    damage = armor = price = hp = combat_rating = quality = 0;
    wearlocations = ({ });
    body_locations = ({ });
    damage_res = ([ ]);
    damage_type = skill = craft_skill = "";

    pending_parents = nil;
    pending_location = -1;
    pending_detail_of = -1;
    pending_removed_details = ({ });
    pending_removed_nouns = ({ });
    pending_removed_adjectives = ({ });
  }
}

void destructed(int clone) {
  int index;
  mixed *objs;

  /* Remove contained objects, put them where this object used to
     be. */
  objs = objects_in_container();
  for(index = 0; index < sizeof(objs); index++) {
    remove_from_container(objs[index]);

    if(obj::get_location())
      obj::get_location()->add_to_container(objs[index]);
  }

  /* Destruct all details */
  objs = get_immediate_details();
  if(!objs) objs = ({ }); /* Prevent error below */
  for(index = 0; index < sizeof(objs); index++) {
    remove_detail(objs[index]);
    EXITD->clear_all_exits(objs[index]);
    destruct_object(objs[index]);
  }

  if(obj::get_detail_of())
    obj::get_location()->remove_detail(this_object());

  if(obj::get_location()) {
    LOGD->write_syslog("Niszczenie POKOJU bez usunięcia go!",
		       LOG_WARN);
  }

  obj::destructed(clone);
}

void upgraded(varargs int clone) {
  /* TODO:  ROOM should recalculate weights and volumes when
     upgraded.  @stat might want to check the object as well. */

  obj::upgraded();
}


/*
 * Get and set functions for fields
 */

void enum_room_mobiles(string cmd, object *except, mixed args...) {
  object *mobiles;
  int i;

  mobiles = mobiles_in_container();
  for (i = sizeof(mobiles); --i >= 0; ) {
    if (sizeof( ({ mobiles[i] }) & except ) == 0) {
      call_other(mobiles[i], cmd, args...);
    }
  }
}

float get_weight(void) {
  if(weight < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_weight();

  return weight < 0.0 ? 0.0 : weight;
}

float get_volume(void) {
  if(volume < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_volume();

  return volume < 0.0 ? 0.0 : volume;
}

float get_length(void) {
  if(length < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_length();

  return length < 0.0 ? 0.0 : length;
}

float get_weight_capacity(void) {
  if(weight_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_weight_capacity();

  return weight_capacity;
}

float get_volume_capacity(void) {
  if(volume_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_volume_capacity();

  return volume_capacity;
}

float get_length_capacity(void) {
  if(length_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_length_capacity();

  return length_capacity;
}

float get_current_weight(void) {
  return current_weight;
}

float get_current_volume(void) {
  return current_volume;
}

int get_damage(void)
{
  if(damage < 1 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_damage();

  return damage;
}

int get_armor(void)
{
  if(armor < 1 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_armor();

  return armor;
}

int get_price(void)
{
  if(price < 1 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_price();

  return price;
}

int* get_wearlocations(void)
{
  if(!sizeof(wearlocations) && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_wearlocations();

  return wearlocations;
}

int get_hp(void)
{
  if(hp < 1 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_hp();

  return hp;
}

int get_combat_rating(void)
{
  if(combat_rating < 1 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_combat_rating();

  return combat_rating;
}

string* get_body_locations(void)
{
  if(!sizeof(body_locations) && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_body_locations();

  return body_locations;
}

string get_skill(void)
{
  if(skill == "" && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_skill();

  return skill;
}

string get_damage_type(void)
{
  if(damage_type == "" && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_damage_type();

  return damage_type;
}

mapping get_damage_res(void)
{
  if(!sizeof(map_indices(damage_res)) && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_damage_res();

  return damage_res;
}

int get_quality(void)
{
    if(quality < 1 && sizeof(obj::get_archetypes()))
        return obj::get_archetypes()[0]->get_quality();

    return quality;
}

int get_durability(void)
{
    return durability;
}

int get_cur_durability(void)
{
    return cur_durability;
}

string get_craft_skill(void)
{
  if(craft_skill == "" && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_craft_skill();

  return craft_skill;
}

void set_weight(float new_weight) {
  object loc;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać wagę!");

  /* Remove from container and add back -- that way the weight
     will be correct */
  loc = obj::get_location();
  if(loc && !obj::get_detail_of()) {
    loc->remove_from_container(this_object());
  } else loc = nil;

  weight = new_weight;

  if(loc)
    loc->add_to_container(this_object());
}

void set_volume(float new_volume) {
  object loc;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać objętość!");

  /* Remove from container and add back -- that way the weight
     will be correct */
  loc = obj::get_location();
  if(loc && !obj::get_detail_of()) {
    loc->remove_from_container(this_object());
  } else loc = nil;

  volume = new_volume;

  if(loc)
    loc->add_to_container(this_object());
}

void set_length(float new_length) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać długość!");

  length = new_length;
}

void set_weight_capacity(float new_weight_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać udźwig!");

  weight_capacity = new_weight_capacity;
}

void set_volume_capacity(float new_volume_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać pojemność!");

  volume_capacity = new_volume_capacity;
}

void set_length_capacity(float new_length_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać maksymalną długość!");

  length_capacity = new_length_capacity;
}

void set_damage(int new_damage) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać obrażenia!");

  damage = new_damage;
}

void set_armor(int new_armor) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać zbroję!");

  armor = new_armor;
}

void set_price(int new_price) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać cenę!");

  price = new_price;
}

void set_wearlocations(int* new_wearlocations) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać lokacje do noszenia!");

  wearlocations = new_wearlocations;
}

void set_hp(int new_hp) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać punkty życia!");

  hp = new_hp;
}

void set_combat_rating(int new_combat_rating) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać poziom bojowy!");

  combat_rating = new_combat_rating;
}

void set_body_locations(string* new_body_locations) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać lokacje ciała!");

  body_locations = new_body_locations;
}

void set_skill(string new_skill) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać umiejętność!");

  skill = new_skill;
}

void set_damage_type(string new_damage_type) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać typ obrażeń!");

  damage_type = new_damage_type;
}

void set_damage_res(mapping new_damage_res) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać odporności!");

  damage_res = new_damage_res;
}

void set_quality(int new_quality) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        error("Tylko autoryzowany kod może ustawiać jakość!");

    quality = new_quality;
}

void set_durability(int new_durability) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        error("Tylko autoryzowany kod może ustawiać wytrzymałość!");

    durability = new_durability;
}

void set_cur_durablity(int new_cur_durability) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        error("Tylko autoryzowany kod może ustawiać obecną wytrzymałość!");

    cur_durability = new_cur_durability;
}

void set_craft_skill(string new_craft_skill) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        error("Tylko autoryzowany kod może ustawiać umiejętność rzemieślniczą!");

    craft_skill = new_craft_skill;
}
/*** Functions dealing with Exits ***/

void clear_exits(void) {
  if(previous_program() == EXITD)
    exits = ({ });
  else error("Tylko EXITD może czyścić wyjścia!");
}

void add_exit(int dir, object exit) {
  if(previous_program() == EXITD) {
    exits = exits + ({ ({ dir, exit }) });
  } else error("Tylko EXITD może dodawać!");
}

int num_exits(void) {
  return sizeof(exits);
}

object get_exit_num(int index) {
  if(index < 0) return nil;
  if(index >= sizeof(exits)) return nil;

  return exits[index][1];
}

object get_exit(int dir) {
  int ctr;

  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    if(exits[ctr][0] == dir)
      return exits[ctr][1];
  }

  return nil;
}

void remove_exit(object exit) {
  if(previous_program() == EXITD) {
    int ctr;

    for(ctr = 0; ctr < sizeof(exits); ctr++) {
      if(exits[ctr][1] == exit) {
	exits = exits[..ctr-1] + exits[ctr+1..];
	return;
      }
    }

    LOGD->write_syslog("Nie można znaleźć wyjścia do usunięcia [" + sizeof(exits)
		       + " wyjść]!", LOG_ERR);
  } else error("Tylko EXITD może usuwać wyjścia!");
}


/*
 * flag overrides
 */

int is_container() {
  return objflags & OF_CONTAINER;
}

int is_open() {
  return objflags & OF_OPEN;
}

int is_openable() {
  return objflags & OF_OPENABLE;
}

int is_wearable() {
  return objflags & OF_WEARABLE;
}

int is_dressed() {
  return objflags & OF_DRESSED;
}

private void set_flags(int flags, int value) {
  if(value) {
    objflags |= flags;
  } else {
    objflags &= ~flags;
  }
}

void set_container(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać obiekt jako pojemnik!");

  set_flags(OF_CONTAINER, value);
}

void set_open(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać obiekt jako otwarty!");

  set_flags(OF_OPEN, value);
}

void set_openable(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać jako otwieralny!");

  set_flags(OF_OPENABLE, value);
}

void set_wearable(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać obiekt jako ubieralny!");

  set_flags(OF_WEARABLE, value);
}

void set_dressed(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko autoryzowany kod może ustawiać obiekt jako założony!");

  set_flags(OF_DRESSED, value);
}

/*
 * overloaded room notification functions
 */

/****** Functions dealing with entering and leaving a room ********/
/* return nil if the user can leave/enter, or a string indicating the
 * reason why they cannot.
 */

/* function which returns an appropriate error message if this object
 * isn't a container or isn't open
 */
static string is_open_cont(object user) {
  if (!is_container()) {
    if(!user) return "nie pojemnik";
    return get_brief()->to_string(user) + " nie jest pojemnikiem!";
  }
  if (!is_open()) {
    if(!user) return "nie otwarty";
    return get_brief()->to_string(user) + " nie jest otwarty!";
  }
  return nil;
}

/* Check item for damage and destroy it when needed */
int damage_item(object user)
{
    if (!this_object()->get_quality())
        return 0;
    if (random(100) <= this_object()->get_quality()) {
        cur_durability--;
        if (cur_durability > 0)
            return 1;
        else {
            this_object()->get_location()->remove_from_container(this_object());
            destruct_object(this_object());
            return 2;
        }
    }
    return 0;
}

/* Note: Many functions are trivial here (always returning nil) but
 * can be overriden to control access into or out of a room 
 * Reason is printed out to the user if the user can't enter */

/* Note also: These functions can be used by the child, even if it
   overrides.  So it's easy for a child to add reasons why an
   object can't be put, entered, etc, but also easy for them to
   change the rules entirely. */

/* The can_XXX functions all take a user.  That user is necessary
   because to return the perceived reason a user can't do something,
   it's often necessary to know what that user can perceive. */

/* user is the user who will see the reason returned,
   leave_object is the body attempting to leave,
   dir is the direction. */
string can_leave(object user, object leave_object, int dir) {
  if(obj::get_mobile())
    return "Nie możesz opuszczać żywych istot!"
      + " Prawdę mówiąc, nawet nie powinieneś tu być.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "nie pojemnik";
      return get_brief()->to_string(user) + " nie jest pojemnikiem.";
    } else {
      return nil;
    }
  } else {
    return is_open_cont(user);
  }
}


/* user is the user who will see the reason returned,
   enter_object is the body attempting to enter,
   dir is the direction. */
string can_enter(object user, object enter_object, int dir) {
  string reason;
  object body;

  if(obj::get_mobile())
    return "Nie możesz wejść do żywej istoty. Nie bądź niemądry.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "nie pojemnik";
      return get_brief()->to_string(user) + " nie jest pojemnikiem.";
    } else {
      return nil;
    }
  } else {
    reason = is_open_cont(user);
    if(reason)
      return reason;
  }

  if(enter_object
     && (current_weight + enter_object->get_weight() > weight_capacity)) {
    if(!user)
      return "przepełniony";
    else
      return get_brief()->to_string(user) + " jest przepełniony!";
  }

  if(enter_object
     && (current_volume + enter_object->get_volume() > volume_capacity)) {
    if(!user)
      return "przepełniony";
    else
      return get_brief()->to_string(user) + " jest przepełniony!";
  }

  return nil;
} 


/* leave_object is the body leaving, dir is the direction */
void leave(object leave_object, int dir) {
  object mob;

  mob = leave_object->get_mobile();

  /* 
   * notify all mobiles in the room that this person is leaving.
   * any user mobiles are then responsable for writing this to the 
   * user's terminal
   */
  enum_room_mobiles("hook_leave", ({ mob }), leave_object, dir );
}

/* enter_object is the body entering, dir is the direction */
void enter(object enter_object, int dir) 
{
    object mob;
    string *condition;
    float quest;

    mob = enter_object->get_mobile();
    if (mob->get_user() && TAGD->get_tag_value(mob, "Quest") != nil) {
        quest = TAGD->get_tag_value(mob, "Quest");
        condition = QUESTD->get_condition(quest);
        if (condition[0] == "room" && (int)condition[1] == this_object()->get_number()) 
            QUESTD->progress_quest(mob);
    }

    enum_room_mobiles("hook_enter", ({ mob }), enter_object, dir );
}


/****** Picking up/dropping functions *********/

/*
 * remove functions are called when the movee object is being
 * removed from this object.  The object can be being moved into
 * the parent or a child of this object.
 */

/*
  user is the user who will see the reason returned
  mover is the body of the mobile doing the moving,
  movee is the object being moved (one of this object's contained objects)
  new_env is the location that the object will shortly be in, which is
           contained by this object or contains this object.
*/
string can_remove(object user, object mover, object movee, object new_env) {
  return is_open_cont(user);
}


/* This function notifies us that an object has been removed from us.

   mover is the body of the mobile doing the moving
   movee is the object being removed from us
   env is the location it will shortly be moved to

   Note:  this function does *not* need to call remove_from_container
   or otherwise remove the object from itself.  That will be done
   separately.  This is just notification that the removal is
   going to occur.
*/
void remove(object mover, object movee, object new_env) {
}

/*
 * get functions are called in this object when this object is being
 * moved from its current room into new_room as part of a move operation
 */

/* This function is called to see whether this object may be taken.

  user is the user who will see the reason returned
  mover is the body of the mobile attempting to move this object
  new_env is where it will be moving it to
*/
string can_get(object user, object mover, object new_env) {
  if(obj::get_mobile()) {
    if(!user) return "żywa istota";
    return get_brief()->to_string(user)
      + " jest żywą istotą! Nie możesz ich podnosić.";
  }
  return nil;
}


/* This function notifies us that somebody has gotten/moved this
   object.

   user is the user who will see the reason returned
   mover is the body of the mobile doing the moving
   new_env is where it is moving us to

   This function doesn't need to move the object from its parent
   to the new environment.  That'll be done separately.  This is
   just notification that that has happened.
*/
void get(object mover, object new_env) 
{
    string *condition;
    float quest;

    if (mover->get_mobile()->get_user() && mover == new_env
            && TAGD->get_tag_value(mover->get_mobile(), "Quest") != nil
            && sizeof(obj::get_archetypes())) {
        quest = TAGD->get_tag_value(mover->get_mobile(), "Quest");
        condition = QUESTD->get_condition(quest);
        if (condition[0] == "item" && obj::get_archetypes() & ({ condition[1] })) 
            QUESTD->progress_quest(mover->get_mobile());
    }
}


/* 
 * put functions are called when the movee object is being
 * moved from another object into this room.  The other
 * object can be the parent or a child.
 */

/*
  This function determines whether an object may be put
  somewhere.

  user is the user who will see the reason returned
  mover is the body of the one doing the moving
  movee is the object being moved
  old_env is the location of the object now containing the movee
*/

string can_put(object user, object mover, object movee, object old_env) {
  string reason;

  reason = is_open_cont(user);
  if(reason)
    return reason;

  if(movee && (current_weight + movee->get_weight() > weight_capacity)) {
    if(!user)
      return "niesie zbyt wiele";
    else
      return get_brief()->to_string(user) + " niesie zbyt wiele!";
  }

  if(movee && (current_volume + movee->get_volume() > volume_capacity)) {
    if(!user)
      return "niesie zbyt wiele";
    else
      return get_brief()->to_string(user) + " niesie zbyt wiele!";
  }

  if(movee && (movee->get_length() > length_capacity)) {
    if(!user)
      return "nie ma odpowiedniej długości";
    else
      return get_brief()->to_string(user)
	+ " nie może trzymać czegoś tak długiego!";
  }
}

/*
  This function notifies us that an object has been put.

  mover is the body of the one doing the moving
  movee is the object being moved
  old_env is the location of the object that just contained movee
*/
void put(object mover, object movee, object old_env) {
}


/********* Overrides of OBJECT functions for containers */

/* Note:  add_to_container calls append_to_container, so we don't
   need to explicitly override it.  If we did, we'd get double-count
   on weight and volume added that way */

void append_to_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    current_weight += obj_weight;

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    current_volume += obj_volume;

  obj::append_to_container(obj);
}


void prepend_to_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    current_weight += obj_weight;
  else
    LOGD->write_syslog("Ujemna waga w prepend_to_container!",
		       LOG_WARN);

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    current_volume += obj_volume;
  else
    LOGD->write_syslog("Ujemna objętość w prepend_to_container!",
		       LOG_WARN);

  obj::prepend_to_container(obj);
}

void remove_from_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    {
      current_weight -= obj_weight;
      if (current_weight < 0.0)
	{
	  current_weight = 0.0;
	}
    }

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    {
      current_volume -= obj_volume;
      if (current_volume < 0.0)
	{
	  current_volume = 0.0;
	}
    }

  obj::remove_from_container(obj);
}


/******** Functions to manage pending fields ***********/

int get_pending_location(void) {
  return pending_location;
}

/* Can't override set_location, we don't have the necessary
   privilege to call it! */

int* get_pending_parents(void) {
  return pending_parents;
}

void set_archetypes(object* new_arch) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko SYSTEM, COMMON i GAME mogą ustawiać archetypy!");

  ::set_archetypes(new_arch);
  pending_parents = nil;
}

int get_pending_detail_of(void) {
  return pending_detail_of;
}

int* get_pending_removed_details(void) {
  return pending_removed_details;
}

object* get_pending_removed_nouns(void) {
  return pending_removed_nouns;
}

object* get_pending_removed_adjectives(void) {
  return pending_removed_adjectives;
}

void set_removed_details(object *new_removed_details) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko SYSTEM, COMMON i GAME mogą ustawiać removed_details!");

  pending_removed_details = ({ });
  ::set_removed_details(new_removed_details);
}

void clear_pending(void) {
  pending_location = pending_detail_of = -1;
  pending_removed_details = ({ });
  pending_parents = ({ });
  pending_removed_nouns = ({ });
  pending_removed_adjectives = ({ });
}


/********* UNQ serialization helper functions */

/* Include only exits that appear to have been created from this room
   so that they aren't doubled up when reloaded */
private string exits_to_unq(void) {
  object exit, dest, other_exit;
  mixed* exit_arr;
  int    ctr, opp_dir;
  string ret;
  object shortphr;

  ret = "";

  /* new style */
  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    exit_arr = exits[ctr];
    exit = exit_arr[1];

    if (exit->get_link() > 0) {
      dest = exit->get_destination();
      if(dest) {
        opp_dir = EXITD->opposite_direction(exit->get_direction());
        other_exit = dest->get_exit(opp_dir);
        if(!other_exit || other_exit->get_destination() != this_object()) {
          LOGD->write_syslog("Problem ze znalezieniem wyjścia powrotnego!", LOG_WARN);
        } else {
	    ret += exit->to_unq_text();
	}
      } else {
        LOGD->write_syslog("Nie mogę znaleźć celu!", LOG_WARNING);
      }
    } else {
      ret += exit->to_unq_text();
    }
  }

  return ret;
}


/* This function serializes tags from TagD so that they can be written
   into the object file. */
private string all_tags_to_unq(void) {
  mixed *all_tags;
  string ret;
  int    ctr;

  all_tags = TAGD->object_all_tags(this_object());
  ret = "";

  for(ctr = 0; ctr < sizeof(all_tags); ctr += 2) {
    ret += "~" + all_tags[ctr] + "{";
    switch(TAGD->object_tag_type(all_tags[ctr])) {
    case T_INT:
    case T_FLOAT:
      ret += all_tags[ctr + 1];
      break;

    case T_STRING:
      ret += STRINGD->unq_escape(all_tags[ctr + 1]);
      break;

    case T_OBJECT:
    case T_ARRAY:
    case T_MAPPING:
    default:
      error("Nie obsługuję jeszcze tego typu tagów!");
    }
    ret += "}\n        ";
  }

  return ret;
}


/* This is used to take a string** array, indexed first by locale
   and then by individual item, into a phrase double-array which
   can be parsed as UNQ. */
private string serialize_wordlist(string **wlist) {
  string tmp;
  int    locale;

  /* Skip debug locale */
  tmp = "";
  for(locale = 1; locale < sizeof(wlist); locale++) {
    if(wlist[locale] && sizeof(wlist[locale])) {
      tmp += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(wlist[locale], ", ") + "}";
    }
  }

  return tmp;
}

/* This is used to serialize a list of numbers or objects into
   a comma-separated list of integers. */
private string serialize_list(mixed *list) {
  string *str_list;
  int     ctr;

  str_list = ({ });
  for(ctr = 0; ctr < sizeof(list); ctr++) {
    switch(typeof(list[ctr])) {
    case T_INT:
      str_list += ({ "" + list[ctr] });
      break;
    case T_OBJECT:
      str_list += ({ list[ctr]->get_number() + "" });
      break;
    case T_STRING:
      str_list += ({ list[ctr] });
      break;
    default:
      error("Błąd w serialize_list - nieakceptowalny obiekt "
	    + STRINGD->mixed_sprint(list[ctr]));
    }
  }

  return implode(str_list, ", ");
}

private string serialize_mapping(mapping map) 
{
    int i;
    string *str_list, *indices;

    indices = map_indices(map);
    str_list = ({ });
    for (i = 0; i < sizeof(indices); i++) {
        str_list += ({ map[indices[i]] + " " + indices[i] });
    }

    return implode(str_list, ", ");
}

/*
 * string to_unq_flags(void)
 *
 * creates a string out of the object flags.
 */

string to_unq_flags(void) {
  string  ret;
  object *rem, *arch;
  int     locale, ctr;

  ret = "  ~number{" + tr_num + "}\n";
  if (get_detail_of()) {
    ret += "  ~detail{" + get_detail_of()->get_number() + "}\n";
  } else if (obj::get_location()) {
    ret += "  ~location{" + obj::get_location()->get_number() + "}\n";
  }

  if(bdesc)
    ret += "  ~bdesc{" + bdesc->to_unq_text() + "}\n";
  if(ldesc)
    ret += "  ~ldesc{" + ldesc->to_unq_text() + "}\n";
  if(edesc)
    ret += "  ~edesc{" + edesc->to_unq_text() + "}\n";

  ret += "  ~flags{" + objflags + "}\n";

  arch = obj::get_archetypes();
  if(arch && sizeof(arch)) {
    ret += "  ~parent{" + serialize_list(arch) + "}\n";
  }

  /* The double-braces are intentional -- this uses the efficient
     method of specifying nouns and adjectives rather than the human-
     friendly one.  Both are parseable, naturally. */
  ret += "  ~nouns{{" + serialize_wordlist(get_immediate_nouns()) + "}}\n";
  ret += "  ~adjectives{{" + serialize_wordlist(get_immediate_adjectives())
    + "}}\n";

  arch = get_archetypes();
  if(arch && sizeof(arch)) {
    ret += "  ~rem_nouns{{" + serialize_wordlist(removed_nouns) + "}}\n";
    ret += "  ~rem_adjectives{{" + serialize_wordlist(removed_adjectives)
      + "}}\n";
  }

  ret += exits_to_unq();

  if(weight >= 0.0)
    ret += "  ~weight{" + weight + "}\n";

  if(volume >= 0.0)
    ret += "  ~volume{" + volume + "}\n";

  if(length >= 0.0)
    ret += "  ~length{" + length + "}\n";

  if(is_container()) {
    if(weight_capacity >= 0.0)
      ret += "  ~weight_capacity{" + weight_capacity + "}\n";

    if(volume_capacity >= 0.0)
      ret += "  ~volume_capacity{" + volume_capacity + "}\n";

    if(length_capacity >= 0.0)
      ret += "  ~length_capacity{" + length_capacity + "}\n";
  }
  if (damage > 0)
      ret += "  ~damage{" + damage + "}\n";
  if (damage_type != "")
      ret += "  ~damage_type{" + damage_type + "}\n";
  if (is_wearable() && sizeof(wearlocations))
      ret += "  ~wearlocations{" + serialize_list(wearlocations) + "}\n";
  if (armor > 0)
      ret += "  ~armor{" + armor + "}\n";
  if (sizeof(map_indices(damage_res)))
      ret += "  ~damage_res{" + serialize_mapping(damage_res) + "}\n";
  if (price > 0)
      ret += "  ~price{" + price + "}\n";
  if (hp > 0)
      ret += "  ~hp{" + hp + "}\n";
  if (combat_rating > 0)
      ret += "  ~combat_rating{" + combat_rating + "}\n";
  if (sizeof(body_locations))
      ret += "  ~body_locations{" + serialize_list(body_locations) + "}\n";
  if (skill != "")
      ret += "  ~skill{" + skill + "}\n";
  if (quality > 0) {
      ret += "  ~quality{" + quality + "}\n";
      ret += "  ~durability{" + durability + "}\n";
      ret += "  ~cur_durability{" + cur_durability + "}\n";
  }
  if (craft_skill != "")
      ret += "  ~craft_skill{" + craft_skill + "}\n";

  rem = get_removed_details();
  if(rem && sizeof(rem)) {
    ret += "  ~removed_details{" + serialize_list(rem) + "}\n";
  }

  ret += "  ~tags{" + all_tags_to_unq() + "}\n";

  return ret;
}

private void parse_all_tags(mixed* value) 
{
    int    ctr, type, do_sscanf;
    string format_code;
    mixed  new_val;

    value = UNQ_PARSER->trim_empty_tags(value);

    for(ctr = 0; ctr < sizeof(value); ctr += 2) {
        value[ctr] = STRINGD->trim_whitespace(value[ctr]);
        type = TAGD->object_tag_type(value[ctr]);

        switch(type) {
            case -1:
                error("Nie ma takiego taga jak '" + STRINGD->mixed_sprint(value[ctr])
                        + "' zdefiniowanego w TagD!");

            case T_INT:
                do_sscanf = 1;
                format_code = "%d";
                break;

            case T_FLOAT:
                do_sscanf = 1;
                format_code = "%f";
                break;

            default:
                error("Nie mogę parsować typów tagów " + type + "!");
        }

        if(do_sscanf) {
            if(typeof(value[ctr + 1]) != T_STRING)
                error("Wewnętrzy błąd: Nie mogę odczytywać tagów wartości nie-string!");

            value[ctr + 1] = STRINGD->trim_whitespace(value[ctr + 1]);

            sscanf(value[ctr + 1], format_code, new_val);
            TAGD->set_tag_value(this_object(), value[ctr], new_val);
        } else {
            /* Nothing yet, if not sscanf */
        }
    } 
}

/*
 * void from_dtd_tag(string tag, mixed value)
 *
 * Grabs data from one field of the DTD-parsed UNQ.  This function
 * is so that child classes can easily add new fields, but still
 * have the parent parse the fields that are known to it.
 */

void from_dtd_tag(string tag, mixed value) {
    int ctr, ctr2;
    string *val;

    switch (tag)
    {
        case "number":
            tr_num = value;
            break;
        case "obj_type":
            break;
        case "detail":
            if(pending_location > -1) {
                LOGD->write_syslog("Detal podany pomimo oczekującej lokacji!",
                        LOG_ERR);
                LOGD->write_syslog("Obiekt #" + tr_num + ", pole detalu: "
                        + value + ", istniejąca lokacja/detal: "
                        + pending_location, LOG_ERR);
                error("Błąd przy ładowaniu obiektu #" + tr_num + "! Sprawdź logi.");
            }
            pending_detail_of = value;
            pending_location = value;
            break;
        case "location":
            if(pending_location > -1) {
                LOGD->write_syslog("Lokacja podana pomimo oczekującej lokacji!",
                        LOG_ERR);
                LOGD->write_syslog("Obiekt #" + tr_num + ", nowa lokacja: "
                        + value + ", istniejąca lokacja/detal: "
                        + pending_location, LOG_ERR);
                error("Błąd przy ładowaniu obiektu #" + tr_num + "! Sprawdź logi.");
            }
            pending_location = value;
            break;
        case "bdesc":
            set_brief(value);
            break;
        case "ldesc":
            set_look(value);
            break;
        case "edesc":
            set_examine(value);
            break;
        case "flags":
            objflags = value;
            break;
        case "parent":
            pending_parents = ({ value });
            set_brief(nil);
            set_look(nil);
            break;
        case "nouns":
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                add_noun(value[ctr2]);
            break;
        case "adjectives":
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                add_adjective(value[ctr2]);
            break;
        case "rem_nouns":
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                pending_removed_nouns += ({ value[ctr2] });
            break;
        case "rem_adjectives":
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                pending_removed_adjectives += ({ value[ctr2] });
            break;
        case "newexit":
            EXITD->room_request_complex_exit(tr_num, value);
            break;
        case "weight":
            weight = value;
            break;
        case "volume":
            volume = value;
            break;
        case "length":
            length = value;
            break;
        case "weight_capacity":
            weight_capacity = value;
            break;
        case "volume_capacity":
            volume_capacity = value;
            break;
        case "length_capacity":
            length_capacity = value;
            break;
        case "damage":
            damage = value;
            break;
        case "damage_type":
            damage_type = value;
            break;
        case "armor":
            armor = value;
            break;
        case "damage_res":
            value = explode(value, ", ");
            for (ctr2 = 0; ctr2 < sizeof(value); ctr2 ++) {
                val = explode(value[ctr2], " ");
                damage_res[val[1]] = val[0];
            }
            break;
        case "price":
            price = value;
            break;
        case "hp":
            hp = value;
            break;
        case "combat_rating":
            combat_rating = value;
            break;
        case "skill":
            skill = value;
            break;
        case "quality":
            quality = value;
            break;
        case "durability":
            durability = value;
            break;
        case "cur_durability":
            cur_durability = value;
            break;
        case "craft_skill":
            craft_skill = value;
            break;
        case "wearlocations":
            value = explode(value, ", ");
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                wearlocations += ({ (int)value[ctr2] });
            break;
        case "body_locations":
            value = explode(value, ", ");
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                body_locations += ({ value[ctr2] });
            break;
        case "removed_details":
            value = explode(value, ", ");
            for(ctr2 = 0; ctr2 < sizeof(value); ctr2++)
                pending_removed_details += ({ (int)value[ctr2] });
            break;
        case "tags":
            /* Fill in tags array for this object */
            parse_all_tags(value);
            break;
        default:
            error("Nie rozpoznaję tagu " + tag + " w funkcji from_dtd_tag()");
    }
}
