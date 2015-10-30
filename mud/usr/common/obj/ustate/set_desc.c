#include <phantasmal/lpc_names.h>

#include <type.h>

inherit USER_STATE;

private int    init;

private object user;

static void create(varargs int clone)
{
  ::create();
  if(clone)
    {
      init = 0;
    }
}

void set_up_func(varargs object reporter)
{
  user = reporter;
  init = 1;
}

int from_user(string output)
{
  error("From_user called in set_desc func!");
}

void pass_data(mixed data)
{
    object phr;
    if(typeof(data) == T_NIL) {
        ::pass_data(nil);
        pop_state();
        return;
    }

    if(!typeof(data) == T_STRING)
        error("Incorrect data type in set_desc user state!");

    data = STRINGD->trim_whitespace(data);
    if (data != "") {
        phr = user->get_body()->get_look();
        phr->from_unq(data);
        send_string("Ustawiono nowy opis postaci.\n");
    }
    else
        send_string("Przerwano wprowadzanie nowego opisu postaci.\n");
    pop_state();
}
