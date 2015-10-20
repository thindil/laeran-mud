#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>


inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;

private object new_obj;

/* Data specified by user */
private int    obj_number;
private int    obj_type;
private object obj_detail_of;


/* Valid object-type values */
#define OT_UNKNOWN                  1
#define OT_ROOM                     2
#define OT_PORTABLE                 3
#define OT_DETAIL                   4


/* Valid substate values */
#define SS_PROMPT_OBJ_TYPE          1
#define SS_PROMPT_OBJ_DETAIL_OF     2
#define SS_PROMPT_OBJ_NUMBER        3
#define SS_PROMPT_OBJ_PARENT        4
#define SS_PROMPT_BRIEF_DESC        5
#define SS_PROMPT_LOOK_DESC         6
#define SS_PROMPT_EXAMINE_DESC      7
#define SS_PROMPT_NOUNS             8
#define SS_PROMPT_ADJECTIVES        9
#define SS_PROMPT_WEIGHT           10
#define SS_PROMPT_VOLUME           11
#define SS_PROMPT_LENGTH           12
/* Note the gap -- that so that if we add states and recompile, anybody
   who is mid-object-creation won't find themselves mysteriously being
   prompted for something else.  Now we only need to renumber *very*
   rarely. */
#define SS_PROMPT_CONTAINER        20
#define SS_PROMPT_OPEN             21
#define SS_PROMPT_OPENABLE         22
#define SS_PROMPT_WEIGHT_CAPACITY  23
#define SS_PROMPT_VOLUME_CAPACITY  24
#define SS_PROMPT_LENGTH_CAPACITY  25
#define SS_PROMPT_WEAPON           26
#define SS_PROMPT_DAMAGE           27
#define SS_PROMPT_WEARABLE         28
#define SS_PROMPT_WLOCATION        29
#define SS_PROMPT_ARMOR            30
#define SS_PROMPT_PRICE            31
#define SS_PROMPT_HP               32
#define SS_PROMPT_COMBAT_RATING    33
#define SS_PROMPT_BODY_LOCATIONS   34
#define SS_PROMPT_SKILL            35


/* Input function return values */
#define RET_NORMAL                  1
#define RET_POP_STATE               2
#define RET_NO_PROMPT               3


/* Prototypes */
static int  prompt_obj_type_input(string input);
static int  prompt_obj_number_input(string input);
static int  prompt_obj_detail_of_input(string input);
static int  prompt_obj_parent_input(string input);
static int  prompt_brief_desc_input(string input);
static void prompt_look_desc_data(mixed data);
static void prompt_examine_desc_data(mixed data);
static int  prompt_nouns_input(string input);
static int  prompt_adjectives_input(string input);
static int  prompt_weight_input(string input);
static int  prompt_volume_input(string input);
static int  prompt_length_input(string input);
static void prompt_container_data(mixed data);
static void prompt_open_data(mixed data);
static void prompt_openable_data(mixed data);
static int  prompt_weight_capacity_input(string input);
static int  prompt_volume_capacity_input(string input);
static int  prompt_length_capacity_input(string input);
static void prompt_weapon_data(mixed data);
static int  prompt_damage_input(string input);
static void prompt_wearable_data(mixed data);
static int  prompt_wlocation_input(string input);
static int  prompt_armor_input(string input);
static int  prompt_price_input(string input);
static int  prompt_hp_input(string input);
static int  prompt_combat_rating_input(string input);
static int  prompt_body_locations_input(string input);
static int  prompt_skill_input(string input);

private string blurb_for_substate(int substate);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)


static void create(varargs int clone) {
  ::create();
  if(!find_object(US_ENTER_DATA)) compile_object(US_ENTER_DATA);
  if(!find_object(US_ENTER_YN)) compile_object(US_ENTER_YN);
  if(!find_object(LWO_PHRASE)) compile_object(LWO_PHRASE);
  if(!find_object(SIMPLE_ROOM)) compile_object(SIMPLE_ROOM);
  if(clone) {
    substate = SS_PROMPT_OBJ_TYPE;
    obj_type = OT_UNKNOWN;
    obj_number = -1;
  }
}

private void specify_type(string type) {
  if(type == "room" || type == "r")
    obj_type = OT_ROOM;
  else if(type == "port" || type == "portable" || type == "p")
    obj_type = OT_PORTABLE;
  else if(type == "det" || type == "detail" || type == "d")
    obj_type = OT_DETAIL;
  else
    error("Illegal value supplied to specify_type!");

  if(obj_type == OT_DETAIL) {
    substate = SS_PROMPT_OBJ_DETAIL_OF;
  } else {
    substate = SS_PROMPT_OBJ_NUMBER;
  }
}

void set_up_func(varargs string new_type) {
  if(new_type)
    specify_type(new_type);
}

/* This handles input directly from the user.  Handling depends on the
   current substate of this user state. */
int from_user(string input) {
  int    ret;
  string quitcheck;

  if(input) {
    quitcheck = STRINGD->trim_whitespace(input);
    if(!STRINGD->stricmp(quitcheck, "wyjdz")) {
      if(new_obj) {
	send_string("(Wychodzenie z OLC -- bez kasowania #"
		    + new_obj->get_number() + " -- Przerwano!)\r\n");
      } else {
	send_string("(Wychodzenie z OLC -- Przerwano!)\r\n");
      }
      pop_state();
      return MODE_ECHO;
    }
  }

  switch(substate) {
  case SS_PROMPT_OBJ_TYPE:
    ret = prompt_obj_type_input(input);
    break;
  case SS_PROMPT_OBJ_DETAIL_OF:
    ret = prompt_obj_detail_of_input(input);
    break;
  case SS_PROMPT_OBJ_NUMBER:
    ret = prompt_obj_number_input(input);
    break;
  case SS_PROMPT_OBJ_PARENT:
    ret = prompt_obj_parent_input(input);
    break;
  case SS_PROMPT_BRIEF_DESC:
    ret = prompt_brief_desc_input(input);
    break;
  case SS_PROMPT_NOUNS:
    ret = prompt_nouns_input(input);
    break;
  case SS_PROMPT_ADJECTIVES:
    ret = prompt_adjectives_input(input);
    break;
  case SS_PROMPT_WEIGHT:
    ret = prompt_weight_input(input);
    break;
  case SS_PROMPT_VOLUME:
    ret = prompt_volume_input(input);
    break;
  case SS_PROMPT_LENGTH:
    ret = prompt_length_input(input);
    break;
  case SS_PROMPT_WEIGHT_CAPACITY:
    ret = prompt_weight_capacity_input(input);
    break;
  case SS_PROMPT_VOLUME_CAPACITY:
    ret = prompt_volume_capacity_input(input);
    break;
  case SS_PROMPT_LENGTH_CAPACITY:
    ret = prompt_length_capacity_input(input);
    break;
  case SS_PROMPT_DAMAGE:
    ret = prompt_damage_input(input);
    break;
  case SS_PROMPT_WLOCATION:
    ret = prompt_wlocation_input(input);
    break;
  case SS_PROMPT_ARMOR:
    ret = prompt_armor_input(input);
    break;
  case SS_PROMPT_PRICE:
    ret = prompt_price_input(input);
    break;
  case SS_PROMPT_HP:
    ret = prompt_hp_input(input);
    break;
  case SS_PROMPT_COMBAT_RATING:
    ret = prompt_combat_rating_input(input);
    break;
  case SS_PROMPT_BODY_LOCATIONS:
    ret = prompt_body_locations_input(input);
    break;
  case SS_PROMPT_SKILL:
    ret = prompt_skill_input(input);
    break;
  case SS_PROMPT_LOOK_DESC:
  case SS_PROMPT_EXAMINE_DESC:
  case SS_PROMPT_CONTAINER:
  case SS_PROMPT_OPEN:
  case SS_PROMPT_OPENABLE:
  case SS_PROMPT_WEAPON:
  case SS_PROMPT_WEARABLE:
    send_string("Internal error in state machine!  Cancelling OLC!\r\n");
    LOGD->write_syslog("Reached from_user() in state " + substate
		       + " while doing @make_room.  Illegal!", LOG_ERR);
    pop_state();
    return MODE_ECHO;
    break;

  default:
    send_string("Nieznany stan! Przerwanie OLC!\r\n");
    pop_state();
    return MODE_ECHO;
  }

  switch(ret) {
  case RET_NORMAL:
    send_string(" > ");
    break;
  case RET_POP_STATE:
    pop_state();
    break;
  case RET_NO_PROMPT:
    break;
  default:
    send_string("Zwrócono nieznaną wartość! Przerwanie OLC!\r\n");
    pop_state();
    break;
  }

  return MODE_ECHO;
}


/* This is if somebody (other than us) is sending data to the user.
   This happens if, for instance, somebody moves into or out of the
   same room and the player sees a message. */
void to_user(string output) {
  /* For the moment, suspend output to the player who is mid-OLC */
  /* send_string(output); */
}


/* This function returns the text to send to the player in the given
   substate at the given moment.  Since we do a lot of "that was
   illegal, let's try it again" type prompting, it gets messy to have
   the text every place the player can screw something up.  The
   supplied substate is assumed to be the one the player is currently
   in, and the prompt is assumed to be prior to input (or prior
   to re-entering input after unacceptable input). */
private string blurb_for_substate(int substate) {
  string tmp;

  switch(substate) {

  case SS_PROMPT_OBJ_TYPE:
    return "Wpisz typ obiektu albo 'wyjdz' aby wyjść.\r\n"
      + "Prawidłowe wartości to:  room, portable, detail (r/p/d)\r\n";

  case SS_PROMPT_OBJ_NUMBER:
    return "Wprowadź wybrany numer dla tego obiektu\r\n"
      + " albo naciśnij enter aby przypisać numer automatycznie.\r\n";

  case SS_PROMPT_OBJ_DETAIL_OF:
    return "Wprowadź numer obiektu bazowego dla tego detalu albo wpisz 'wyjdz'.\r\n"
      + "Chodzi o numer istniejącego obiektu dla którego będzie ten detal.\r\n";

  case SS_PROMPT_OBJ_PARENT:
    return "Wprowadź numer obiektu rodzica z którego zostaną pobrane dane obiektu.\r\n"
      + "Przykład: #37 #247 #1343\r\n"
      + "Możesz również nacisnąć enter dla braku rodziców lub wpisać 'wyjdz' aby wyjść.\r\n"
      + "Rodzice są jak Skotos ur-objects (zobacz pomoc set_obj_parent).\r\n";

  case SS_PROMPT_BRIEF_DESC:
    return "Następnie, proszę wprowadzić jednoliniowy opis.\r\n"
      + "Przykłady krótkiego(brief) opisu:  "
      + "'miecz', 'John', 'trochę bekonu'.\r\n";

  case SS_PROMPT_LOOK_DESC:
    return "Teraz wprowadź wieloliniowy opis dla 'patrz'('look'). To jest to co gracz"
      + " zobaczy przy pomocy komendy 'patrz'.\r\n";

  case SS_PROMPT_EXAMINE_DESC:
    return "Teraz wprowadź wieloliniowy opis dla 'zbadaj'('examine'). To jest to co gracz"
      + " zobaczy przy pomocy komendy 'zbadaj'.\r\n"
      + "Albo naciśnij '~' i enter aby ustawić to na taki sam tekst jak dla komendy 'patrz'.\r\n";

  case SS_PROMPT_NOUNS:
    tmp = "Krótki opis (Brief):  " + new_obj->get_brief()->to_string(get_user()) + "\r\n";

    if(sizeof(new_obj->get_archetypes())) {
      tmp += "Rzeczowniki z rodzica: ";
      tmp += implode(new_obj->get_nouns(get_user()->get_locale()), ", ");
      tmp += "\r\n";
    }

    tmp += "\r\nPodaj oddzielone spacją nowe rzeczowniki odwołujące się do tego obiektu.\r\n"
      + "Przykład: miecz ostrze bron broń rękojeść rekojesc\r\n\r\n";

    return tmp;

  case SS_PROMPT_ADJECTIVES:
    tmp = "Krótki opis (Brief):  " + new_obj->get_brief()->to_string(get_user()) + "\r\n";

    if(sizeof(new_obj->get_archetypes())) {
      tmp += "Przymiotniki z rodzica: ";
      tmp += implode(new_obj->get_adjectives(get_user()->get_locale()), ", ");
      tmp += "\r\n";
    }

    tmp += "\nPodaj odddzielone spacją nowe przymiotniki odwołujące się do tego obiektu object.\r\n"
      + "Przykład: ciezki ciężki metaliczny czerwony\r\n";

    return tmp;

  case SS_PROMPT_WEIGHT:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj ciężar obiektu albo wpisz 'none' aby przyjąć wartości z rodzica.\r\n"
	+ "Domyślnie waga jest podana w kilogramach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metryczne: mg   g   kg     Standard: lb   oz   tons\r\n";
    return "Podaj ciężar obiektu.\r\n"
      + "Domyślnie waga jest podana w kilogramach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metryczne: mg   g   kg     Standard: lb   oz   tons\r\n";

  case SS_PROMPT_VOLUME:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj objętość obiektu albo wpisz 'none' aby przyjąć wartości z rodzica.\r\n"
	+ "Domyślnie objętość jest podana w litrach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metrycznec: L   mL   cc   cubic m\r\n"
	+ "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";
    return "Podaj objętość obiektu.\r\n"
      + "Domyślnie objętość jest podana w litrach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metryczne: L   mL   cc   cubic m\r\n"
      + "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";

  case SS_PROMPT_LENGTH:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź długość najdłuższej osi obiektu albo wpisz 'none' aby przyjąć wartości\r\n"
	+ "z rodzica.\r\n"
	+ "Domyślnie długość jest podana w centymetrach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metryczne: m   mm   cm   dm     Standard: in   ft   yd\r\n";
    return "Wprowadź długość najdłuższej osi obiektu.\r\n"
      + "Domyślnie długość jest podana w centymetrach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metryczne: m   mm   cm   dm     Standard: in   ft   yd\r\n";

    /* The following don't have full-on blurbs, just one-line prompts
       for the ENTER_YN user state.  So no blurb. */
  case SS_PROMPT_CONTAINER:
  case SS_PROMPT_OPEN:
  case SS_PROMPT_OPENABLE:
  case SS_PROMPT_WEAPON:
  case SS_PROMPT_WEARABLE:
    return "This blurb should never be used.  Oops!\r\n";

  case SS_PROMPT_WEIGHT_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj udźwig obiektu albo wpisz 'none' aby przyjąć wartości z rodzica.\r\n"
	+ "Domyślnie udźwig jest podany w kilogramach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metryczne: mg   g   kg     Standard: lb   oz   tons\r\n";
    return "Podaj udźwig obiektu.\r\n"
      + "Domyślnie udźwig jest podany w kilogramach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metric: mg   g   kg     Standard: lb   oz   tons\r\n";

  case SS_PROMPT_VOLUME_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj pojemność obiektu albo wpisz 'none' aby przyjąć wartości z rodzica.\r\n"
	+ "Domyślnie pojemność jest podana w litrach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metryczne: L   mL   cc   cubic m\r\n"
	+ "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";
    return "Podaj pojemność obiektu\r\n"
      + "Domyślnie pojemność jest podana w litrach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metryczne: L   mL   cc   cubic m\r\n"
      + "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";

  case SS_PROMPT_LENGTH_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj długość najdłuższej osi pojemnika albo wpisz 'none' aby przyjąć wartości\r\n"
	+ "z rodzica.\r\n"
	+ "Domyślnie długość jest podana w centymetrach albo możesz podać dodatkowo jednostki.\r\n"
	+ "Metryczne: m   mm   cm   dm     Standard: in   ft   yd\r\n";
    return "Podaj długość najdłuższej osi pojemnika.\r\n"
      + "Domyślnie długość jest podana w centymetrach albo możesz podać dodatkowo jednostki.\r\n"
      + "Metryczne: m   mm   cm   dm     Standard: in   ft   yd\r\n";

  case SS_PROMPT_DAMAGE:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź ilość zadawanych obrażeń przez obiekt albo wpisz "
	+ " 'none' aby\n przyjąć wartości z archetypu.\n";
    return "Wprowadź ilość zadawanych obrażeń przez obiekt.\n";
  case SS_PROMPT_WLOCATION:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź lokację (lub lokacje, oddzielone spacjami) na które można założyć dany obiekt, "
	+ " albo wpisz 'none' \n aby przyjąć wartości z archetypu.\n"
	+ "Dostępne lokacje to: tułów ręce nogi głowa dłonie\n";
    return "Wprowadź lokację (lub lokacje) na które można założyć dany obiekt.\n"
      + "Dostępne lokacje to: tułów ręce nogi głowa dłonie\n";
  case SS_PROMPT_ARMOR:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź wartość zbroi przedmiotu, czyli ile obrażeń potrafi "
	+ " odjąć podczas ataku \n albo wpisz 'none' aby przyjąć wartości "
	+ " z archetypu.\n";
    return "Wprowadź wartość zbroi przedmiotu, czyli ile obrażeń potrafi "
      + " odjąć podczas ataku. \n";
  case SS_PROMPT_PRICE:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź cenę za przedmiot w sklepie (kupno/sprzedaż) albo wpisz"
	+ " 'none' aby przyjąć \n wartości z archetypu.\n";
    return "Wprowadź cenę za przedmiot w sklepie (kupno/sprzedaż).\n";
  case SS_PROMPT_HP:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź ilość punktów życia przedmiotu albo wpisz 'none' aby przyjąć \n"
	+ "wartości z archetypu.\n";
    return "Wprowadź ilość punktów życia przedmiotu.\n";
  case SS_PROMPT_COMBAT_RATING:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź poziom bojowy obiektu albo wpisz 'none' aby przyjąć wartości \n"
	+ "z archetypu.\n";
    return "Wprowadź poziom bojowy obiektu.\n";
  case SS_PROMPT_BODY_LOCATIONS:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Wprowadź lokacje ciała oddzielone spacjami albo wpisz 'none' aby przyjąć wartość z archetypu.\n"
	+ "Aby pominąć ten krok, wciśnij enter. Przykład: głowa, korpus, przednia łapa, tylnia łapa\n";
    return "Wprowadź lokacje ciała oddzielone spacjami. Aby pominąć ten krok, wciśnij enter.\n"
      + "Przykład: głowa, korpus, przednia łapa, tylnia łapa\n";
  case SS_PROMPT_SKILL:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Podaj nazwę umiejętności potrzebną do używania tego obiektu albo wpisz 'none' aby przyjąć\n"
	+ "wartość z archetypu. Aby pominąć ten krok, wciśnij enter. \n";
    return "Podaj nazwę umiejętności potrzebną do używania tego obiektu. Aby pominąć ten krok, wciśnij \n"
      + "enter.\n";
  default:
    return "<NIEZNANY STAN>\r\n";
  }
}


/* This is called when the state is switched to.  The pushp parameter
   describes whether a push or a pop switched to this state -- if
   pushp is true, the state was just allocated and started.  If it's
   false, a state got pushed and we resumed control afterward. */
void switch_to(int pushp) {
  if(pushp
     && (substate == SS_PROMPT_OBJ_NUMBER
	 || substate == SS_PROMPT_OBJ_TYPE
	 || substate == SS_PROMPT_OBJ_DETAIL_OF)) {
    /* Just allocated */
    send_string("Tworzenie nowego przedmiotu. Wpisz 'wyjdz'"
		+ " (za wyjątkiem wieloliniowych tekstów) aby przerwać.\r\n");
    send_string(blurb_for_substate(substate));
    send_string(" > ");
  } else if (substate == SS_PROMPT_LOOK_DESC
	     || substate == SS_PROMPT_EXAMINE_DESC) {
    /* Do nothing */
  } else if (substate == SS_PROMPT_NOUNS
	     || substate == SS_PROMPT_WEIGHT_CAPACITY
	     || substate == SS_PROMPT_WLOCATION
	     || substate == SS_PROMPT_DAMAGE
	     || substate == SS_PROMPT_ARMOR) {
    /* This means we just got back from getting a desc */
    send_string(" > ");
  } else {
    /* Somebody else pushed and then popped a state, so we're just
       getting back to ourselves. */
    send_string("(Tworzenie obiektu -- powrót)\r\n");
    send_string(" > ");
  }
}

void switch_from(int popp) {
  if(!popp) {
    if(substate != SS_PROMPT_LOOK_DESC
       && substate != SS_PROMPT_EXAMINE_DESC
       && substate != SS_PROMPT_CONTAINER
       && substate != SS_PROMPT_OPEN
       && substate != SS_PROMPT_OPENABLE
       && substate != SS_PROMPT_WEAPON
       && substate != SS_PROMPT_WEARABLE) {
      send_string("(Tworzenie obiektu -- przerwanie)\r\n");
    }
  }
}

/* Some other state has passed us data, probably when it was
   popped. */
void pass_data(mixed data) {
  switch(substate) {
  case SS_PROMPT_LOOK_DESC:
    prompt_look_desc_data(data);
    break;
  case SS_PROMPT_EXAMINE_DESC:
    prompt_examine_desc_data(data);
    break;
  case SS_PROMPT_CONTAINER:
    prompt_container_data(data);
    break;
  case SS_PROMPT_OPEN:
    prompt_open_data(data);
    break;
  case SS_PROMPT_OPENABLE:
    prompt_openable_data(data);
    break;
  case SS_PROMPT_WEAPON:
    prompt_weapon_data(data);
    break;
  case SS_PROMPT_WEARABLE:
    prompt_wearable_data(data);
    break;
  default:
    send_string("Warning: User State was passed unrecognized data!\r\n");
    break;
  }
}

static int prompt_obj_type_input(string input) {
  if(input)
    input = STRINGD->trim_whitespace(STRINGD->to_lower(input));

  /* TODO:  we should probably use the binder for this */
  if(!input
     || (input != "r" && input != "p" && input != "d"
	 && input != "room"
	 && input != "port" && input != "portable"
	 && input != "det" && input != "detail")) {
    send_string("That's not a valid object type.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_OBJ_TYPE));

    return RET_NORMAL;
  }

  if(input[0] == "r"[0]) {
    obj_type = OT_ROOM;
  } else if(input[0] == "d"[0]) {
    obj_type = OT_DETAIL;
  } else {
    obj_type = OT_PORTABLE;
  }

  if(obj_type == OT_DETAIL) {
    substate = SS_PROMPT_OBJ_DETAIL_OF;
  } else {
    substate = SS_PROMPT_OBJ_NUMBER;
  }

  send_string(blurb_for_substate(substate));

  /* The editor is going to print its own prompt, so don't bother
     with ours. */
  return RET_NORMAL;
}

static int prompt_obj_detail_of_input(string input) {
  int base_num;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("\r\nMusisz podać obiekt bazowy."
		+ " Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  if(sscanf(input, "%*d %*s") == 2
     || sscanf(input, "%*s %*d") == 2
     || sscanf(input, "%d", base_num) != 1) {
    send_string("\r\nMusisz podać pojedynczy numer obiektu.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  if(base_num < 1) {
    send_string("\r\nNumer obiektu musi być większy niż zero.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  obj_detail_of = MAPD->get_room_by_num(base_num);
  if(!obj_detail_of) {
    send_string("\r\nTo nie wygląda na room bądź portable #"
		+ base_num + ".\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  send_string("\r\nObiekt bazowy zaakceptowany.\r\n");
  substate = SS_PROMPT_OBJ_NUMBER;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_obj_number_input(string input) {
  string segown;
  object location;
  int    zonenum;

  if(!input || STRINGD->is_whitespace(input)) {
    /* Autoassign */
    obj_number = -1;

    send_string("Numer obiektu będzie przypisany automatycznie.\r\n");
  } else {
    if(sscanf(input, "%*s %*d") == 2
       || sscanf(input, "%*d %*s") == 2
       || sscanf(input, "%d", obj_number) != 1) {
      send_string("Proszę wprowadzić *tylko* numer.\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));
      return RET_NORMAL;
    }
    /* Object number was parsed. */
    if(obj_number < 1) {
      send_string("Nie wygląda na poprawny numer obiektu.\r\n");
      send_string("Numer obiektu musi być większy od zera.\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }
    if(MAPD->get_room_by_num(obj_number)) {
      send_string("Jest już obiekt o numerze #" + obj_number + ".\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }
    segown = OBJNUMD->get_segment_owner(obj_number / 100);
    if(obj_number >= 0 && segown && segown != MAPD) {
      user->message("Obiekt #" + obj_number
		    + " w segmencie posiadanym przez "
		    + segown + "!\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }

    /* Okay, object number looks good -- continue. */
  }

  if(obj_type == OT_DETAIL) {
    location = obj_detail_of;
  } else {
    location = get_user()->get_location();
    if(location && obj_type == OT_ROOM) {
      /* The new room should be put into the same place as the room
	 the user is currently standing in.  Makes a good default. */
      location = location->get_location();
    }
  }

  /* Rooms, portables and details are now all cloned from the same
     base. */
  new_obj = clone_object(SIMPLE_ROOM);

  if(!new_obj) {
    send_string("Przykro mi, osiągnąłeś limit obiektów albo pamięci!\r\n");

    return RET_POP_STATE;
  }

  zonenum = -1;
  if(obj_number < 0) {
    /* Get zone based on object type and location */
    if(obj_type == OT_DETAIL) {
      zonenum = ZONED->get_zone_for_room(obj_detail_of);
    } else if(get_user()->get_location()) {
      zonenum = ZONED->get_zone_for_room(get_user()->get_location());
    } else {
      zonenum = 0;
    }

    if(zonenum < 0) {
      LOGD->write_syslog("Odd, zone is less than 0 in @make_room...",
			 LOG_WARN);
      zonenum = 0;
    }
  }
  MAPD->add_room_to_zone(new_obj, obj_number, zonenum);

  zonenum = ZONED->get_zone_for_room(new_obj);

  if(obj_type == OT_DETAIL
     && !obj_detail_of) {
    send_string("Ktoś skasował obiekt bazowy kiedy tworzyłeś ten\r\n Nie stworzono detalu."
		+ " Wychodzenie z OLC!\r\n");
    destruct_object(new_obj);
    return RET_POP_STATE;
  }

  if(obj_detail_of) {
    obj_detail_of->add_detail(new_obj);
  } else if(location) {
    location->add_to_container(new_obj);
  }

  send_string("Dodano obiekt #" + new_obj->get_number()
	      + " do strefy #" + zonenum
	      + " (" + ZONED->get_name_for_zone(zonenum) + ")" + ".\r\n");
  if(obj_detail_of) {
    send_string("To jest detal obiektu ");
  } else {
    send_string("Jego lokacja to ");
  }
  if(location) {
    string tmp;

    if(location->get_brief()) {
      tmp = location->get_brief()->to_string(get_user());
    } else {
      tmp = "(nieopisany)";
    }

    send_string("#" + location->get_number() + "(" + tmp + ")\r\n\r\n");
  } else {
    send_string("nigdzie\r\n\r\n");
  }

  /* Okay, now keep entering data... */
  substate = SS_PROMPT_OBJ_PARENT;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_obj_parent_input(string input) {
  int     parnum, ctr;
  object *obj_parents;
  object  obj_parent;
  string *parent_strings;

  if(!input || STRINGD->is_whitespace(input)) {
    /* No parent -- that works. */

    substate = SS_PROMPT_BRIEF_DESC;
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  parent_strings = explode(input, " ");
  for(ctr = 0; ctr < sizeof(parent_strings); ctr++) {
    if(parent_strings[ctr] == "#")
      continue;

    if((!sscanf(parent_strings[ctr], "#%d", parnum)
	&& !sscanf(parent_strings[ctr], "%d", parnum))
       || (parnum < 0)) {
      send_string("Każdy rodzić musi być wartością większą od zera\r\n poprzedzoną znakiem #.\r\n");
      send_string("'" + parent_strings[ctr] + "' nie jest.\r\n");
      send_string(blurb_for_substate(substate));
      return RET_NORMAL;
    }

    if(!(obj_parent = MAPD->get_room_by_num(parnum))) {
      send_string("Nie ma obiektu #" + parnum + ".\r\n");
      send_string(blurb_for_substate(substate));
      return RET_NORMAL;
    }

    obj_parents += ({ obj_parent });
  }

  if(obj_parents && sizeof(obj_parents)) {
    new_obj->set_archetypes(obj_parents);
  } else {
    send_string("Wewnętrzny błąd. Dziwne. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  substate = SS_PROMPT_BRIEF_DESC;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_brief_desc_input(string input) {
  object PHRASE phr;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Były tylko spacje. Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_BRIEF_DESC));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);
  phr = new_obj->get_brief();
  phr->from_unq(input);

  substate = SS_PROMPT_LOOK_DESC;

  send_string(blurb_for_substate(substate));
  push_new_state(US_ENTER_DATA);

  return RET_NORMAL;
}

static void prompt_look_desc_data(mixed data) {
  object PHRASE phr;

  if(typeof(data) != T_STRING) {
    send_string("Dziwne dane wprowadzone do stanu! Przerywamy.\r\n");
    pop_state();
    return;
  }

  if(!data || STRINGD->is_whitespace(data)) {
    send_string("Wygląda jak same spacje. Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_LOOK_DESC));

    push_new_state(US_ENTER_DATA);

    return;
  }

  data = STRINGD->trim_whitespace(data);
  phr = new_obj->get_look();
  phr->from_unq(data);

  substate = SS_PROMPT_EXAMINE_DESC;
  send_string("\r\nOpis 'patrz' (look) zaakceptowany.\r\n");

  send_string(blurb_for_substate(SS_PROMPT_EXAMINE_DESC));

  push_new_state(US_ENTER_DATA);
}

static void prompt_examine_desc_data(mixed data) {
  string examine_desc;

  if(typeof(data) != T_STRING) {
    send_string("Dziwne dane wprowadzone do stanu! Przerywamy.\r\n");
    pop_state();
    return;
  }

  if(!data || STRINGD->is_whitespace(data)) {
    send_string("Opis zbadaj (examine) domyślnie taki sam jak opis 'patrz'(look).\r\n");
    examine_desc = nil;
  } else {
    examine_desc = STRINGD->trim_whitespace(data);
  }

  if(examine_desc && !STRINGD->is_whitespace(examine_desc)) {
    new_obj->set_examine(NEW_PHRASE(examine_desc));
  }
  substate = SS_PROMPT_NOUNS;

  send_string("\r\nW porządku, teraz weźmiemy listę rzeczowników i przymiotników, którymi możesz opisać\r\n"
	      + "ten obiekt. Dla przypomnienia pokażemy Tobie również krótki opis przedmiotu, który wprowadziłeś.\r\n");
  send_string(blurb_for_substate(SS_PROMPT_NOUNS));

  /* Don't return anything, this is a void function */
}

/* Used when processing nouns and adjectives */
private mixed process_words(string input) {
  object  phr, location;
  string* words;
  int     ctr;

  words = explode(input, " ");

  for(ctr = 0; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  phr = PHRASED->new_simple_english_phrase(implode(words[0..], ","));

  return phr;
}


static int prompt_nouns_input(string input) {
  string nouns;

  if(obj_type == OT_PORTABLE
     && (!input || STRINGD->is_whitespace(input))) {
    send_string("Nie. Chcesz mieć przynajmmniej jeden przymiotnik. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_NOUNS));
    return RET_NORMAL;
  }

  nouns = STRINGD->trim_whitespace(input);
  new_obj->add_noun(process_words(nouns));

  substate = SS_PROMPT_ADJECTIVES;

  send_string("Dobrze. Teraz to samo dla przymiotników.\r\n");
  send_string(blurb_for_substate(SS_PROMPT_ADJECTIVES));

  return RET_NORMAL;
}

static int prompt_adjectives_input(string input) {
  object PHRASE phr;
  string adjectives;

  adjectives = STRINGD->trim_whitespace(input);
  if(adjectives && adjectives != "") {
    new_obj->add_adjective(process_words(adjectives));
  }

  if(obj_type == OT_ROOM) {
    new_obj->set_container(1);
    new_obj->set_open(1);

    new_obj->set_weight(2000.0);    /* 2 metric tons */
    new_obj->set_volume(1000000.0); /* Equiv of 10m cubic room */
    new_obj->set_length(1000.0);    /* 10m -- too big to pick up */
    new_obj->set_weight_capacity(2000000.0);   /* 2000 metric tons */
    new_obj->set_volume_capacity(27000000.0);  /* Equiv of 30m cubic room */
    new_obj->set_length_capacity(1500.0);      /* 15m */

    send_string("\r\nUkończono pokój #" + new_obj->get_number() + ".\r\n");
    return RET_POP_STATE;
  }

  if(obj_type == OT_PORTABLE) {
    substate = SS_PROMPT_WEIGHT;

    send_string("Dobrze. Teraz ustalimy wagę obiektu.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  /* If it's not a portable or a room, it's a detail.  In that case,
     don't bother with the weight, volume and length for this object.
     It's part of another.  It *will* have capacities later, though,
     if it's a container. */

  substate = SS_PROMPT_CONTAINER;

  push_new_state(US_ENTER_YN, "Czy obiekt jest pojemnikiem? ");

  return RET_NORMAL;
}


static int prompt_weight_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "kg"             : 1.0,
	       "kilograms"      : 1.0,
	       "kilogram"       : 1.0,
	       "g"              : 0.001,
	       "grams"          : 0.001,
	       "gram"           : 0.001,
	       "mg"             : 0.000001,
	       "milligrams"     : 0.000001,
	       "milligram"      : 0.000001,
	       "pounds"         : 0.45,
	       "pound"          : 0.45,
	       "lb"             : 0.45,
	       "ounces"         : 0.028,
	       "ounce"          : 0.028,
	       "oz"             : 0.028,
	       "tons"           : 990.0,
	       "ton"            : 990.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostki masy bądź wagi. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '4.7 oz' lub '3 tons' lub '0.5 mg'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać numer większy od zera. Sprónuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_weight(value);

  send_string("Waga zaakceptowana.\r\n");
  substate = SS_PROMPT_VOLUME;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_volume_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy to ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "liter"             : 1.0,
               "liters"            : 1.0,
               "l"                 : 1.0,
               "L"                 : 1.0,
               "milliliter"        : 0.001,
               "milliliters"       : 0.001,
               "ml"                : 0.001,
               "mL"                : 0.001,
               "cubic centimeters" : 0.001,
               "cubic centimeter"  : 0.001,
	       "cc"                : 0.001,
	       "cubic cm"          : 0.001,
	       "cu cm"             : 0.001,
	       "cubic meters"      : 1000.0,
	       "cubic meter"       : 1000.0,
	       "cubic m"           : 1000.0,
	       "cu m"              : 1000.0,

	       "ounces"            : 0.0296,
	       "ounce"             : 0.0296,
	       "oz"                : 0.0296,
	       "pints"             : 0.473,
	       "pint"              : 0.473,
	       "pt"                : 0.473,
	       "quarts"            : 0.946,
	       "quart"             : 0.946,
	       "qt"                : 0.946,
	       "gallons"           : 3.784,
	       "gallon"            : 3.784,
	       "gal"               : 3.784,
	       "cubic foot"        : 28.3,
	       "cubic feet"        : 28.3,
	       "cubic ft"          : 28.3,
	       "cu ft"             : 28.3,
	       "cubic yards"       : 765.0,
	       "cubic yard"        : 765.0,
	       "cubic yd"          : 765.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostki objętości. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '1.2 liters' lub '250 cc' lub '35 cu dm'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać numer większy od zera. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_volume(value);

  send_string("Objętość zaakceptowana.\r\n");
  substate = SS_PROMPT_LENGTH;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_length_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "centimeters"    : 1.0,
	       "centimeter"     : 1.0,
	       "cm"             : 1.0,
	       "millimeters"    : 0.1,
	       "millimeter"     : 0.1,
	       "mm"             : 0.1,
	       "decimeters"     : 10.0,
	       "decimeter"      : 10.0,
	       "dm"             : 10.0,
	       "meters"         : 100.0,
	       "meter"          : 100.0,
	       "m"              : 100.0,

	       "inches"         : 2.54,
	       "inch"           : 2.54,
	       "in"             : 2.54,
	       "feet"           : 30.5,
	       "foot"           : 30.5,
	       "ft"             : 30.5,
	       "yards"          : 91.4,
	       "yard"           : 91.4,
	       "yd"             : 91.4,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostek długości. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '4.7 oz' lub '3 tons' lub '0.5 mg'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać numer większy niż zero. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_length(value);

  send_string("Zaakceptowano długość.\r\n");
  substate = SS_PROMPT_CONTAINER;

  push_new_state(US_ENTER_YN, "Czy obiekt jest pojemnikiem? ");

  return RET_NORMAL;
}


static void prompt_container_data(mixed data) {
  if(typeof(data) != T_INT) {
    send_string("Wewnętrzny błąd -- przekazano zły typ!\r\n");
    pop_state();
    return;
  }

  if(!data) {
    /* Not a container, so neither open nor openable. */
    if(obj_type == OT_PORTABLE)
      {
	substate = SS_PROMPT_WEAPON;
	push_new_state(US_ENTER_YN, "Obiekt jest bronią? ");
      }
    else
      {
	send_string("Ukończono detal #" + new_obj->get_number() + ".\r\n");
	pop_state();
      }
    return;
  }

  new_obj->set_container(1);

  substate = SS_PROMPT_OPEN;

  push_new_state(US_ENTER_YN, "Czy pojemnik jest otwarty? ");
}

static void prompt_open_data(mixed data) {
  if(typeof(data) != T_INT) {
    send_string("Wewnętrzny błąd -- przekazano zły typ!\r\n");
    pop_state();
    return;
  }

  if(data) {
    /* Container is open */
    new_obj->set_open(1);
  }

  substate = SS_PROMPT_OPENABLE;

  push_new_state(US_ENTER_YN,
		 "Czy pojemnik można swobodnie zamykać i otwierać? ");
}

static void prompt_openable_data(mixed data) {
  if(typeof(data) != T_INT) {
    send_string("Wewnętrzny błąd -- przekazano zły typ!\r\n");
    pop_state();
    return;
  }

  if(data) {
    new_obj->set_openable(1);
  }

  substate = SS_PROMPT_WEIGHT_CAPACITY;
  send_string(blurb_for_substate(substate));
}


static int prompt_weight_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "kg"             : 1.0,
	       "kilograms"      : 1.0,
	       "kilogram"       : 1.0,
	       "g"              : 0.001,
	       "grams"          : 0.001,
	       "gram"           : 0.001,
	       "mg"             : 0.000001,
	       "milligrams"     : 0.000001,
	       "milligram"      : 0.000001,
	       "pounds"         : 0.45,
	       "pound"          : 0.45,
	       "lb"             : 0.45,
	       "ounces"         : 0.028,
	       "ounce"          : 0.028,
	       "oz"             : 0.028,
	       "tons"           : 990.0,
	       "ton"            : 990.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostki masy bądź wagi. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '4.7 oz' lub '3 tons' lub '0.5 mg'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać numer większy niż zero. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_weight_capacity(value);

  send_string("Zaakceptowano udźwig.\r\n");
  substate = SS_PROMPT_VOLUME_CAPACITY;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_volume_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "liter"             : 1.0,
               "liters"            : 1.0,
               "l"                 : 1.0,
               "L"                 : 1.0,
               "milliliter"        : 0.001,
               "milliliters"       : 0.001,
               "ml"                : 0.001,
               "mL"                : 0.001,
               "cubic centimeters" : 0.001,
               "cubic centimeter"  : 0.001,
	       "cc"                : 0.001,
	       "cubic cm"          : 0.001,
	       "cu cm"             : 0.001,
	       "cubic meters"      : 1000.0,
	       "cubic meter"       : 1000.0,
	       "cubic m"           : 1000.0,
	       "cu m"              : 1000.0,

	       "ounces"            : 0.0296,
	       "ounce"             : 0.0296,
	       "oz"                : 0.0296,
	       "pints"             : 0.473,
	       "pint"              : 0.473,
	       "pt"                : 0.473,
	       "quarts"            : 0.946,
	       "quart"             : 0.946,
	       "qt"                : 0.946,
	       "gallons"           : 3.784,
	       "gallon"            : 3.784,
	       "gal"               : 3.784,
	       "cubic foot"        : 28.3,
	       "cubic feet"        : 28.3,
	       "cubic ft"          : 28.3,
	       "cu ft"             : 28.3,
	       "cubic yards"       : 765.0,
	       "cubic yard"        : 765.0,
	       "cubic yd"          : 765.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostek objętości. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '1.2 liters' lub '250 cc' lub '35 cu dm'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać wartość większą niż zero. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_volume_capacity(value);

  send_string("Zaakceptowano pojemność.\r\n");
  substate = SS_PROMPT_LENGTH_CAPACITY;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_length_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Spróbujmy ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "centimeters"    : 1.0,
	       "centimeter"     : 1.0,
	       "cm"             : 1.0,
	       "millimeters"    : 0.1,
	       "millimeter"     : 0.1,
	       "mm"             : 0.1,
	       "decimeters"     : 10.0,
	       "decimeter"      : 10.0,
	       "dm"             : 10.0,
	       "meters"         : 100.0,
	       "meter"          : 100.0,
	       "m"              : 100.0,

	       "inches"         : 2.54,
	       "inch"           : 2.54,
	       "in"             : 2.54,
	       "feet"           : 30.5,
	       "foot"           : 30.5,
	       "ft"             : 30.5,
	       "yards"          : 91.4,
	       "yard"           : 91.4,
	       "yd"             : 91.4,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("Nie rozpoznaję jednostki '" + unitstr
		  + "' jako jednostek długości. Spróbuj ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Wprowadź wartość, opcjonalnie z jednostkami. Coś jak:\r\n"
		+ "  '4.7 oz' lub '3 tons' lub '0.5 mg'. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("Chcesz podać wartość większą od zera. Spróbuj ponownie.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_length_capacity(value);

  send_string("Zaakceptowano maksymalną długość przedmiotów w pojemniku.\r\n\r\n");
  substate = SS_PROMPT_CONTAINER;

  if(obj_type == OT_PORTABLE)
    {
      substate = SS_PROMPT_WEAPON;
      push_new_state(US_ENTER_YN, "Obiekt jest bronią? ");
      return RET_NORMAL;
    }
  else
    {
      send_string("Ukończono detal #" + new_obj->get_number() + ".\r\n");
    }

  return RET_POP_STATE;
}

static void prompt_weapon_data(mixed data)
{
  if(typeof(data) != T_INT)
    {
      send_string("Wewnętrzny błąd -- przekazano zły typ!\r\n");
      pop_state();
      return;
    }

  if(data)
    {
      new_obj->set_weapon(1);
    }

  substate = SS_PROMPT_DAMAGE;
  send_string(blurb_for_substate(substate));
}

static int prompt_damage_input(string input)
{
  int     value;

  value = 0;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%d", value) == 1)
    {
      if (value < 0)
	{
	  send_string("Obrażenia powinny być dodatnie bądź zero. Spróbuj ponownie.\n");
	  send_string(blurb_for_substate(substate));

	  return RET_NORMAL;
	}
    }
  else if(!STRINGD->stricmp(input, "none"))
    {
      value = -1;
    }
  else
    {
      send_string("Broń powinna zadawać jakieś obrażenia. Spróbuj ponownie.\n");
      send_string(blurb_for_substate(substate));
	  
      return RET_NORMAL;
    }

  new_obj->set_damage(value);

  send_string("Zaakceptowano obrażenia.\n\n");
  substate = SS_PROMPT_WEARABLE;
  push_new_state(US_ENTER_YN, "Obiekt można zakładać na ciało? ");

  return RET_NORMAL;
}

static void prompt_wearable_data(mixed data)
{
  if(typeof(data) != T_INT)
    {
      send_string("Wewnętrzny błąd -- przekazano zły typ!\r\n");
      pop_state();
      return;
    }

  if(data)
    {
      new_obj->set_wearable(1);
    }
  else
    {
      substate = SS_PROMPT_ARMOR;
      send_string(blurb_for_substate(substate));
      return;
    }

  substate = SS_PROMPT_WLOCATION;
  send_string(blurb_for_substate(substate));
}

static int prompt_wlocation_input(string input)
{
  string* inputlocations;
  int* locations;
  int i;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  inputlocations = explode(input, " ");
  locations = ({ });
  for (i = 0; i < sizeof(inputlocations); i ++)
    {
      switch (inputlocations[i])
	{
	case "głowa":
	  locations += ({0});
	  break;
	case "tułów":
	  locations += ({1});
	  break;
	case "ręce":
	  locations += ({2});
	  break;
	case "dłonie":
	  locations += ({3});
	  break;
	case "nogi":
	  locations += ({4});
	  break;
	case "none":
	  locations = nil;
	  i = sizeof(inputlocations);
	  break;
	default:
	  break;
	}
    }

  new_obj->set_wearlocations(locations);

  send_string("Zaakceptowano lokacje do noszenia.\n\n");
  substate = SS_PROMPT_ARMOR;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_armor_input(string input)
{
  int     value;

  value = 0;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%d", value) == 1)
    {
      if (value < 0)
	{
	  send_string("Zbroja nie powinna zwiększać obrażeń. Spróbuj ponownie.\n");
	  send_string(blurb_for_substate(substate));

	  return RET_NORMAL;
	}
    }
  else if(!STRINGD->stricmp(input, "none"))
    {
      value = -1;
    }
  else
    {
      send_string("Podaj wartość zbroi. Spróbuj ponownie.\n");
      send_string(blurb_for_substate(substate));
	  
      return RET_NORMAL;
    }

  new_obj->set_armor(value);

  send_string("Zaakceptowano zbroję obiektu.\n\n");
  substate = SS_PROMPT_PRICE;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_price_input(string input)
{
  int     value;

  value = 0;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%d", value) == 1)
    {
      if (value < 0)
	{
	  send_string("Cena nie powinna być ujemna. Spróbuj ponownie.\n");
	  send_string(blurb_for_substate(substate));

	  return RET_NORMAL;
	}
    }
  else if(!STRINGD->stricmp(input, "none"))
    {
      value = -1;
    }
  else
    {
      send_string("Podaj cenę obiektu. Spróbuj ponownie.\n");
      send_string(blurb_for_substate(substate));
	  
      return RET_NORMAL;
    }

  new_obj->set_price(value);

  send_string("Zaakceptowano cenę obiektu.\n\n");
  substate = SS_PROMPT_HP;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_hp_input(string input)
{
  int     value;

  value = 0;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%d", value) == 1)
    {
      if (value < 0)
	{
	  send_string("Ilość punktów życia nie powinna być ujemna. Spróbuj ponownie.\n");
	  send_string(blurb_for_substate(substate));

	  return RET_NORMAL;
	}
    }
  else if(!STRINGD->stricmp(input, "none"))
    {
      value = -1;
    }
  else
    {
      send_string("Podaj punkty życia obiektu. Spróbuj ponownie.\n");
      send_string(blurb_for_substate(substate));
	  
      return RET_NORMAL;
    }

  new_obj->set_hp(value);

  send_string("Zaakceptowano punkty życia obiektu.\n\n");
  substate = SS_PROMPT_COMBAT_RATING;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_combat_rating_input(string input)
{
  int     value;

  value = 0;

  if(!input || STRINGD->is_whitespace(input))
    {
      send_string("Spróbujmy ponownie.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%d", value) == 1)
    {
      if (value < 0)
	{
	  send_string("Poziom bojowy nie powinnien być ujemna. Spróbuj ponownie.\n");
	  send_string(blurb_for_substate(substate));

	  return RET_NORMAL;
	}
    }
  else if(!STRINGD->stricmp(input, "none"))
    {
      value = -1;
    }
  else
    {
      send_string("Podaj poziom bojowy obiektu. Spróbuj ponownie.\n");
      send_string(blurb_for_substate(substate));
	  
      return RET_NORMAL;
    }

  new_obj->set_combat_rating(value);

  send_string("Zaakceptowano poziom bojowy obiektu.\n\n");
  substate = SS_PROMPT_BODY_LOCATIONS;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_body_locations_input(string input)
{
  string* locations;

  if(!input || STRINGD->is_whitespace(input))
    {
      locations = ({ });
    }
  else
    {
      locations = explode(input, " ");
    }
  if (sizeof(locations) && locations[0] == "none")
    {
      locations = ({ });
    }

  new_obj->set_body_locations(locations);

  send_string("Zaakceptowano lokacje ciała.\n\n");
  substate = SS_PROMPT_SKILL;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_skill_input(string input)
{
  if (input = "none")
    {
      input = "";
    }
  new_obj->set_skill(input);

  send_string("Zaakceptowano umiejętność.\n\n");
  send_string("Zakończono prace nad przenośnym obiektem #" + new_obj->get_number() + ".\n");

  return RET_POP_STATE;
}
