#include <phantasmal/lpc_names.h>

#include <type.h>

inherit USER_STATE;

private int    init;

private string func_name;
private object obj_to_set;
private int    locale;

static void create(varargs int clone) {
  ::create();
  if(clone) {
    init = 0;
  }
}

void set_up_func(varargs object obj, string desctype, int desc_loc) {
  obj_to_set = obj;
  func_name = desctype;
  locale = desc_loc;
  init = 1;
}

int from_user(string output) {
  error("From_user wywołane w funkcji set_obj_desc!");
}

void pass_data(mixed data) {
  object phr;

  if(typeof(data) == T_NIL) {
    ::pass_data(nil);   /* Request user obj to print prompt */
    pop_state();
    return;
  }

  if(!typeof(data) == T_STRING) {
    error("Nieprawidłowy typ danych w stanie użytkownika set_obj_desc!");
  }

  data = STRINGD->trim_whitespace(data);
  phr = call_other(obj_to_set, "get_" + func_name);

  if(!phr || (func_name == "examine" && phr == obj_to_set->get_look())) {
    phr = PHRASED->new_simple_english_phrase("CHANGE ME!");
    call_other(obj_to_set, "set_" + func_name, phr);
  }
  phr->from_unq(data);

  send_string("Ustawiono opis " + func_name + ".\n");

  pop_state();
}
