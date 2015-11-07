#include <kernel/kernel.h>

#include <phantasmal/map.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

inherit MOBILE;

/* user associated with this mobile */
object user;

static void create(varargs int clone) {
  ::create(clone);
}

/* overridden functions from mobile.c */
string get_type(void) {
  return "user";
}

object get_user(void) {
  return user;
}

void set_user(object new_user) {
  if (!SYSTEM() && !COMMON() && !GAME()) {
    error("Only authorized objects can change the user!");
  }

  user = new_user;
}

void notify_moved(object obj) {
  if (user) {
    user->notify_moved(obj);
  }

  ::notify_moved(obj);
}

/* 
 * Hook function overrides
 */

void hook_say(object body, string message) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" mówi: " + message + "\r\n");
  }
}

void hook_emote(object body, string message) {
  if(user) {
    user->send_phrase(body->get_brief());
    user->message(" " + message + "\r\n");
  }
}

/* Args -- body, target, verb */
void hook_social(object body, object target, string verb) {
    string ret;
    string *condition;
    float quest;

    if(user) {
        if (TAGD->get_tag_value(this_object(), "Quest") && verb == "pozdrow") {
            quest = TAGD->get_tag_value(this_object(), "Quest");
            condition = QUESTD->get_condition(quest);
            if (condition[0] == "npc" && condition[1] == target->get_mobile()->get_number())
                QUESTD->progress_quest(this_object());
        } else {
            ret = SOULD->get_social_string(user, body, target, verb);
            if(!ret) 
                user->message("Nie można pobrać tekstu socjalnego!\n");
            else
                user->message(ret + "\n");
        }
    }
}

void hook_whisper(object body, string message) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" szepcze do Ciebie: " + message + "\n");
  }
}

void hook_whisper_other(object body, object target) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" szepcze coś do ");
    user->send_phrase(target->get_brief());
    user->message("\n");
  }
}

void hook_leave(object body, int dir) 
{
    string reason;    

    if (user) {
        if (dir == DIR_TELEPORT) {
            user->message(body->get_brief()->to_string(user) + " znika w kłębie dymu.\n");
            if (TAGD->get_tag_value(user->get_body(), "Follow") 
                    && TAGD->get_tag_value(user->get_body(), "Follow") == body->get_number())
                user->stop_follow(user);
        } else {
            user->message(body->get_brief()->to_string(user) + " odchodzi na " 
                    + EXITD->get_name_for_dir(dir)->to_string(user) + "\n");
            if (TAGD->get_tag_value(user->get_body(), "Follow") 
                    && TAGD->get_tag_value(user->get_body(), "Follow") == body->get_number()) {
                reason = move(dir);
                if (reason) {
                    user->message(reason + "\n");
                    user->stop_follow(user);
                } else
                    user->message(user->get_body()->get_location()->get_brief()->to_string(user) + "\n");
            }
        }
    }
}

void hook_enter(object body, int dir) {
  if (user) {
    if (dir == DIR_TELEPORT) {
      user->send_phrase(body->get_brief());
      user->message(" pojawia się znikąd.\n");
    } else {
      user->send_phrase(body->get_brief()) ;
      user->message(" nadchodzi z ");
      user->send_phrase(EXITD->get_name_for_dir(dir));
      user->message("\n");
    }
  }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* User mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
