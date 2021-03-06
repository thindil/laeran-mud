#include <kernel/kernel.h>

#include <phantasmal/map.h>
#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/* Mobile: structure for a sentient, not-necessarily-player critter's
   mind.  The mobile will be attached to a body under any normal
   circumstances.
*/

inherit unq DTD_UNQABLE;
inherit tag TAGGED;

/*
 * cached vars
 */

static object body;     /* The mobile's body -- an OBJECT of some type */
static object location;
static int    number;
static int    parentbody;
static int    spawnroom;

static void create(varargs int clone) {
  unq::create();
  tag::create();
}

void upgraded(void) {
  unq::upgraded();
  tag::upgraded();
}


/*
 * System functions
 *
 * Functions used to deal with the mobile elsewhere in the MUD
 */

void assign_body(object new_body) {
  if(!SYSTEM() && !COMMON() && !GAME()) {
    error("Tylko autoryzowany kod może przypisywać nowe ciało mobkowi!");
  }

  if(body) {
    body->set_mobile(nil);
    body = nil;
  }

  if(new_body) {
    new_body->set_mobile(this_object());
  }

  body = new_body;
  location = body->get_location();
}

object get_body(void) {
  return body;
}

object get_user(void) {
  /* return nil, the default mobile doesn't have a user */
  return nil;
}

void set_user(object new_user) {
  error("Nie mogę ustawić użytkownika dla tego typu mobków");
}

int get_number(void) {
  return number;
}

int get_parentbody(void)
{
  return parentbody;
}

void set_parentbody(int new_parentbody)
{
  parentbody = new_parentbody;
}

int get_spawnroom(void)
{
  return spawnroom;
}

void set_spawnroom(int new_spawnroom)
{
  spawnroom = new_spawnroom;
}

nomask void set_number(int new_num) {
  if(previous_program() != MOBILED) {
    error("Tylko MOBILED może ustawiać numery mobkom!");
  }
  number = new_num;
}

void notify_moved(object obj) {
  if(SYSTEM() || COMMON())
    location = body->get_location();
}

/*
 * Action functions
 * 
 * Functions called by the user object or inherited objects to do
 * stuff through mobile's body
 */

/*
 * Say
 *
 * Tells something to everyone in the room
 */

nomask void say(string msg) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  location->enum_room_mobiles("hook_say", ({ this_object() }), body, msg );

  if (get_user()) {
    get_user()->message("Mówisz: " + msg + "\r\n");
  }
}

/*
 * emote()
 *
 * Sends an emote to everyone in the room.  Emotes may be completely replaced
 * by souls eventually. (Keith Dunwoody's personal preference).
 */

nomask void emote(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  /* For an emote, show the user the same message everybody else sees.
     For instance "Bob sits still." rather than "You sits still.". */
  location->enum_room_mobiles("hook_emote", ({ }), body, str);
}

/*
 * social()
 *
 * Does a social/soul action.  This will be visible to everyone in the
 * room and may appear different to the (optional) target.  The "target"
 * parameter should point to the target's body.
 */

nomask void social(string verb, object target) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  this_object()->hook_social(body, target, verb);
  location->enum_room_mobiles("hook_social", ({ this_object() }), body, target, verb);
  return;
}

/*
 * void whisper()
 *
 * object to: body of the object to whisper to.
 *
 * Whisper to someone or something.  They must be in the same location as you.
 */
nomask void whisper(object to, string str) {
  object mob;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if (to->get_location() != location)
    {
      get_user()->message("Nie ma takiej osoby w okolicy.\n");
      return;
    }

  mob = to->get_mobile();
  if (mob == nil) {
    return;
  }
  if (get_user()) {
    get_user()->message("Szepczesz do ");
    get_user()->send_phrase(to->get_brief());
    get_user()->message(": " + str + "\n");
  }
  mob->hook_whisper(body, str);
  location->enum_room_mobiles("hook_whisper_other",
                              ({ this_object(), mob }), body, to);
  
  return;
}

/*
 * path_place()
 *
 * moves the object along the path, removing the object from all objects
 * in rem_path, and adding it to all objects in add_path, in order.
 * rem_path & add_path must already be in the proper order -- no checks
 * are performed.
 */
private atomic void path_place(object user, object *rem_path,
                               object *add_path, object obj) {
  int i;
  object env, my_user;
  string reason;

  /* assume the full move can be performed.  If it can't we'll throw an error,
     which will cause this function to be completely undone when the error
     occurs.  Hurray for atomics!
  */

  my_user = get_user();

  for (i = 0; i < sizeof(rem_path); ++i) {
    env = rem_path[i]->get_location();
    if ((reason = rem_path[i]->can_remove(my_user, body, obj, env)) ||
        (reason = obj->can_get(my_user, body, env)) ||
        (reason = env->can_put(my_user, body, obj, rem_path[i]))) {
      error("$" + reason);
    } else {
      /* call function in object for removing from the current room */
      obj->get(body, env);
      rem_path[i]->remove(body, obj, env);
      rem_path[i]->remove_from_container(obj);
      env->add_to_container(obj);
      env->put(body, obj, rem_path[i]);
    }
  }

  /* now add this object to the objects in the add path in order */
  for (i = 0; i < sizeof(add_path); ++i) {
    env = add_path[i]->get_location();
    if ((reason = add_path[i]->can_put(my_user, body, obj, env)) ||
        (reason = obj->can_get(my_user, body, add_path[i])) ||
        (reason = env->can_remove(my_user, body, obj, add_path[i]))) {
      error("$" + reason);
    } else {
      obj->get(body, add_path[i]);
      env->remove(body, obj, add_path[i]);
      env->remove_from_container(obj);
      add_path[i]->add_to_container(obj);
      add_path[i]->put(body, obj, env);
    }
  }
}

/* 
 * place()
 *
 * move the object obj from its current position into the object to.
 * obj and to must both be contained somewhere in the current room,
 * though not necessarily directly in it.
 *
 * returns nil on success, a reason for the failure on failure
 */
nomask string place(object obj, object to) {
  object cur_loc;
  object *rem_tree, *add_tree;
  int common;
  string err;    
  object user;
  int i;

  if(!SYSTEM() && !COMMON() && !GAME())
    return "Brak dostępu!";

  /* find out how many rooms this object can be removed from, ending 
   * when we find the mobile's location.
   */

  rem_tree = ({ });
  add_tree = ({ });

  cur_loc = obj->get_location();
  while(cur_loc != location) {
    if (cur_loc == nil) {
      /* the object to move isn't a descendent of the mobile's current
       * location
       */
      if (get_user()) {
        err = obj->get_brief()->to_string(get_user());
      } else {
        err = obj->get_brief()->as_markup(MARKUP_DEBUG);
      }
      err += " nie jest w tym pokoju";
      return err;
    }

    rem_tree += ({ cur_loc });
    cur_loc = cur_loc->get_location();
  }

  /* do the same thing for moving the object into the container, except
   * include the container */

  cur_loc = to;
  while (cur_loc != location) {
    if (cur_loc == nil) {
      /* the place to move to is not a descendent of the mob's location
       * so return an error
       */
      if (get_user()) {
        err = to->get_brief()->to_string(get_user());
      } else {
        err = to->get_brief()->as_markup(MARKUP_DEBUG);
      }
      err += " nie jest w tym pokoju";

      return err;
    }

    add_tree += ({ cur_loc });
    cur_loc = cur_loc->get_location();
  }

  /* remove all common elements from the ends of the remove & add lists */

  common = sizeof(add_tree & rem_tree);

  if (common != 0) {
    add_tree = add_tree[..(sizeof(add_tree)-common-1)];
    rem_tree = rem_tree[..(sizeof(rem_tree)-common-1)];
  }

  err = catch(path_place(get_user(), rem_tree, add_tree, obj));

  if (err) {
    /* non-serious errors will be prefixed with a '$' -- in which case we
     * strip the leading $, and return the error.
     */
    if (err[0] == '$') {
      return err[1..];
    } else {
      /* serious error -- re-throw */
      error(err);
    }
  }

  return nil;
}

/*
 * string open()
 *
 * have the mobile attempt to open the given object.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */
nomask string open(object obj) {
  object link_exit, obj2;
  int isexit, objnum;

  if(!SYSTEM() && !COMMON() && !GAME())
    return "Dostęp zabroniony!";

  isexit = 0;

  objnum = obj->get_number();
  obj2 = EXITD->get_exit_by_num(objnum);
  if (obj2) {
    isexit = 1;
  }

  if(!obj->is_openable() || (!obj->is_container())) {
    return "To nie może być otwarte!";
  }

  if(obj->is_open()) {
    return "To już jest otwarte.";
  }

  if(obj->is_locked()) {
    return "Wygląda na zablokowane.";
  }

  if (isexit) {
    obj->set_open(1);
    if (obj->get_link()!=-1) {
      link_exit = EXITD->get_exit_by_num(obj->get_link());
      link_exit->set_open(1);
    }
  } else {
    obj->set_open(1);
  }

  if(!isexit && obj->get_location() != body
     && obj->get_location() != location) {
    return "Nie możesz tego dosięgnąć.";
  }

  return nil;
}

/*
 * string close()
 *
 * have the mobile attempt to close the given object.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */
nomask string close(object obj) {
  object link_exit, obj2;
  int isexit, objnum;

  if(!SYSTEM() && !COMMON() && !GAME())
    return "Dostęp zabroniony!";

  isexit = 0;

  objnum = obj->get_number();
  obj2 = EXITD->get_exit_by_num(objnum);
  if (obj2) {
    isexit = 1;
  }

  if(!obj->is_openable() || (!obj->is_container())) {
    return "To nie może być zamknięte!";
  }

  if(!obj->is_open()) {
    return "To już jest zamknięte.";
  }

  if(obj->get_location() != location
     && obj->get_location() != body
     && !isexit) {
    return "Nie możesz tego dosięgnąć.";
  }

  if (isexit) {
    if (obj->get_link()!=-1) {
      link_exit = EXITD->get_exit_by_num(obj->get_link());
      link_exit->set_open(0);
    }
  }
  obj->set_open(0);

  return nil;
}


/*
 * string move()
 *
 * move's the mobile's body through the given exit.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */

nomask string move(int dir) 
{
    object dest;
    object exit;
    string reason;
    int fatigue;

    if(!SYSTEM() && !COMMON() && !GAME())
        return "Dostęp zabroniony!";

    exit = location->get_exit(dir);
    if (!exit) 
        return "Nie ma wyjścia w tym kierunku!";

    dest = exit->get_destination();
    if (!dest) 
        return "To wyjście prowadzi donikąd!";

    if (get_user()) {
        if (TAGD->get_tag_value(body, "Fatigue"))
            fatigue = TAGD->get_tag_value(body, "Fatigue");
        else
            fatigue = 0;
        if (fatigue >= (get_user()->get_stat_val("kondycja") * 10)) 
            return "Jesteś zbyt zmęczony aby podróżować. Odpocznij chwilę.";
    }

    /* NB.  I do want a = (not == ), as in other places like this*/
    if (reason = location->can_leave(get_user(), body, dir)) 
        return reason;

    if (reason = exit->can_pass(get_user(), body)) 
        return reason;

    if (reason = dest->can_enter(get_user(), body,
                EXITD->opposite_direction(dir))) 
        return reason;


    location->leave(body, dir);
    location->remove_from_container(body);
    exit->pass(body);
    dest->add_to_container(body);
    dest->enter(body, EXITD->opposite_direction(dir));

    return nil;
}


/*
 * string teleport()
 *
 * teleport's the mobile's body to the given destination.
 * 
 * parameters:
 * force -- if true, forces the teleport to always succeed.
 *
 * return -- the reason why the teleport didn't succeed, or nil on success
 */

nomask string teleport(object dest, int force) {
  string reason;

  if(!SYSTEM() && !COMMON() && !GAME())
    return "Dostęp zabroniony!";

  if (!force) {
    if (location) {
      if (reason = location->can_leave(get_user(), body, DIR_TELEPORT)) {
        return reason;
      }
    }
    
    if (reason = dest->can_enter(get_user(), body, DIR_TELEPORT)) {
      return reason;
    }
  }

  if (location) {
    location->leave(body, DIR_TELEPORT);
    location->remove_from_container(body);
  }

  dest->add_to_container(body);
  dest->enter(body, DIR_TELEPORT);

  return nil;
}

/* death of mobile, remove body */
nomask void death(object user)
{
    object PHRASE phr;
    string *nouns;
    string body_name;

    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    nouns = body->get_nouns(user->get_locale());
    if (sizeof(nouns) < 4)
        body_name = body->get_brief()->to_string(user);
    else
        body_name = nouns[3];
    phr = PHRASED->new_simple_english_phrase("zwłoki " + body_name);
    body->set_brief(phr);
    phr = PHRASED->new_simple_english_phrase("zwłoki " + body_name + " leżą tutaj.");
    body->set_look(phr);
    body->set_mobile(nil);
    body->clear_nouns();
    phr = PHRASED->new_simple_english_phrase("zwłoki, zwloki");
    body->add_noun(phr);
    TAGD->set_tag_value(body, "DropTime", time());
    body = nil;
}

/*
 * Hook functions
 *
 * Functions which can be overridden in a derived class to respond to external
 * events.  In the standard mobile object, these have empty definitions
 *
 * Don't bother calling these base functions, as they will never do anyting.
 * they're here as documentation, nothing else.
 */

void hook_say(object body, string message) {
}

void hook_emote(object body, string message) {
}

void hook_social(object body, object target, string verb) {
}

void hook_whisper(object body, string message) {
}

void hook_whisper_other(object body, object target) {
}

void hook_leave(object leaving_body, int direction) {
}

void hook_enter(object entering_body, int direction) {
}

void hook_time(int hour)
{
}


/* This function serializes tags from TagD so that they can be written
   into the object file. */
private string all_tags_to_unq(void) 
{
    mixed *all_tags;
    string ret;
    int    ctr;

    all_tags = TAGD->mobile_all_tags(this_object());
    ret = "";

    for(ctr = 0; ctr < sizeof(all_tags); ctr += 2) {
        ret += "~" + all_tags[ctr] + "{";
        switch(TAGD->mobile_tag_type(all_tags[ctr])) {
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
        ret += "}\n";
    }

    return ret;
}

/*
 * UNQ functions
 */

string to_unq_text(void) 
{
    string ret;
    int    bodynum;

    if(!SYSTEM() && !COMMON() && !GAME())
        return nil;

    if(body)
        bodynum = body->get_number();
    else
        bodynum = -1;

    ret  = "~mobile{\n";
    ret += "  ~type{" + this_object()->get_type() + "}\n";
    if (bodynum > -1)
        ret += "  ~name{" + body->get_brief()->as_unq() + "}\n";
    ret += "  ~number{" + number + "}\n";
    ret += "  ~body{" + bodynum + "}\n";
    if (parentbody > 0)
        ret += "  ~parentbody{" + parentbody + "}\n";
    if (spawnroom > 0)
        ret += "  ~spawnroom{" + spawnroom + "}\n";
    if(function_object("mobile_unq_fields", this_object())) {
        ret += "  ~data{\n";
        ret += this_object()->mobile_unq_fields();
        ret += "  }\n";
    }
    ret += "  ~tags{" + all_tags_to_unq() + "}\n";
    ret += "}\n\n";

    return ret;
}

void from_dtd_unq(mixed* unq) {
  error("Nadpisz from_dtd_unq aby go wywoływać!");
}

private void parse_all_tags(mixed* value) 
{
    int    ctr, type, do_sscanf;
    string format_code;
    mixed  new_val;

    value = UNQ_PARSER->trim_empty_tags(value);

    for(ctr = 0; ctr < sizeof(value); ctr += 2) {
        value[ctr] = STRINGD->trim_whitespace(value[ctr]);
        type = TAGD->mobile_tag_type(value[ctr]);

        switch(type) {
            case -1:
                error("Nie ma takiego tagu jak '" + STRINGD->mixed_sprint(value[ctr])
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
                error("Nie mogę jeszcze obsłużyć tagów typu " + type + "!");
        }

        if(do_sscanf) {
            if(typeof(value[ctr + 1]) != T_STRING)
                error("Wewnętrzny błąd: Nie mogę odczytać tagów z innej wartości niż string!");

            value[ctr + 1] = STRINGD->trim_whitespace(value[ctr + 1]);

            sscanf(value[ctr + 1], format_code, new_val);
            TAGD->set_tag_value(this_object(), value[ctr], new_val);
        } else {
            /* Nothing yet, if not sscanf */
        }
    }
}

/* We don't override from_dtd_unq, but we *do* provide parsing functionality
   for the really basic stuff like number and body.  Child objects may
   choose to use this.  It extracts the fields it uses, leaving the
   rest.
*/
static mixed mobile_from_dtd_unq(mixed* unq) 
{
    mixed *ret, *ctr;
    int    bodynum;

    ctr = unq;

    while(sizeof(ctr) > 0)
    {
        switch (ctr[0][0])
        {
            case "body":
                bodynum = ctr[0][1];
                if(bodynum != -1)
                {
                    body = MAPD->get_room_by_num(bodynum);
                    if(!body)
                    {
                        error("Nie mogę znaleźć ciała dla mobka, obiekt #" + bodynum + "!\n");
                    }
                    location = body->get_location();
                }
                else
                {
                    body = nil;
                    location = nil;
                }
                break;
            case "number":
                number = ctr[0][1];
                break;
            case "type":
                break;
            case "data":
                ret = ({ ctr[0][1] });
                break;
            case "name":
                break;
            case "parentbody":
                parentbody = ctr[0][1];
                break;
            case "spawnroom":
                spawnroom = ctr[0][1];
                break;
            case "tags":
                /* Fill in tags array for this object */
                parse_all_tags(ctr[0][1]);
                break;
            default:
                error("Nieznane pole w strukturze mobile!");
                break;
        }
        ctr = ctr[1..];
    }

    return ret;
}

string get_type(void) {
  error("Wywołano get_type na /usr/common/lib/mobile bez nadpisania!");
}
