#include <kernel/user.h>
#include <phantasmal/lpc_names.h>

inherit USER_STATE;

private string prompt;

static void create(varargs int clone) {
  ::create();
  if(clone) {
    prompt = "Tak lub nie? ";
  }
}

void set_prompt(string new_prompt) {
  prompt = new_prompt;
}

void set_up_func(varargs string new_prompt) {
  if(new_prompt) {
    set_prompt(new_prompt);
  }
}

int from_user(string input) {
  if(input == "t" || input == "T"
     || !STRINGD->stricmp(input, "tak")) {
    pass_data(1);

    pop_state();
    return MODE_ECHO;
  }
  if(input == "n" || input == "N"
     || !STRINGD->stricmp(input, "nie")) {
    pass_data(0);

    pop_state();
    return MODE_ECHO;
  }

  send_string("To zdecydowanie nie było 'tak' lub 'nie'.  Spróbuj ponownie.\r\n");
  send_string(prompt);

  return MODE_ECHO;
}

void switch_to(int pushp) {
  send_string(prompt);
}
