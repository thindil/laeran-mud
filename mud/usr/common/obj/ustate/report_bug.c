#include <phantasmal/lpc_names.h>

#include <type.h>

inherit USER_STATE;

private int    init;

private string command;
private object user;

static void create(varargs int clone)
{
  ::create();
  if(clone)
    {
      init = 0;
    }
}

void set_up_func(varargs string cmd, object reporter)
{
  command = cmd;
  user = reporter;
  init = 1;
}

int from_user(string output)
{
  error("From_user called in cmd_report func!");
}

void pass_data(mixed data)
{
  if(typeof(data) == T_NIL)
    {
      ::pass_data(nil);
      pop_state();
      return;
    }

  if(!typeof(data) == T_STRING)
    {
      error("Incorrect data type in report_bug user state!");
    }

  data = STRINGD->trim_whitespace(data);
  if (data != "")
      call_other(user, "cmd_report", user, command, data);
  else
      send_string("Przerwano wprowadzanie zgłoszenia.\n");
  pop_state();
}
