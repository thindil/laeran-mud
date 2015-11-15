#include <kernel/kernel.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/timed.h>

#include <gameconfig.h>
#include <config.h>

static void create(void) {

}

void set_up_heart_beat(void)
{
  if(previous_program() == GAME_INITD)
    {
      TIMED->set_heart_beat(TIMED_TEN_MINUTES, "heart_beat_clear");
      TIMED->set_heart_beat(TIMED_ONE_HOUR, "heart_beat_dump");
      TIMED->set_heart_beat(TIMED_ONE_DAY, "heart_beat_save");
    }
}

/* delete dropped objects, add packages to postman, change weather for zones */
void heart_beat_clear(void)
{
    int *rooms;
    int i, roll, j;
    object obj;
    mixed tag;
    string weather;
    string *weathers;
    object *users;

    if(previous_program() != TIMED)
        return;
    rooms = ({ });
    weathers = ({ "clear", "overcast", "rain" });
    users = users();
    for(i = 0; i < ZONED->num_zones(); i++) {
        rooms += MAPD->rooms_in_zone(i);
        weather = ZONED->get_attribute(i, "weather");
        if (weather != "none" && random(100) > 50) {
            roll = random(sizeof(weathers));
            if (weather != weathers[roll]) {
                weather = weathers[roll];
                ZONED->set_attribute(i, "weather", weather);
                for (j = 0; j < sizeof(users); j++) {
                    if (ZONED->get_zone_for_room(users[j]->get_location()) != i)
                        continue;
                    if (users[j]->get_location()->get_room_type() == 1
                            || users[j]->get_location()->get_room_type() == 2) {
                        switch (weather) {
                            case "clear":
                                users[j]->message("Przejaśnia się.\n");
                                break;
                            case "overcast":
                                users[j]->message("Zbierają się chmury.\n");
                                break;
                            case "rain":
                                users[j]->message("Zaczyna padać.\n");
                                break;
                            default:
                                break;
                        }
                    }
                }   
            }
        }
    }

    for (i = 0; i < sizeof(rooms); i++) {
        obj = MAPD->get_room_by_num(rooms[i]);
        tag = TAGD->get_tag_value(obj, "DropTime");
        if (tag && time() - tag >= 300) {
            if (obj->get_location())
                obj->get_location()->remove_from_container(obj);
            destruct_object(obj);
        }
    }
}

void heart_beat_save(void)
{
    find_object(INITD)->save_mud_data(nil, ROOM_DIR, MOB_DIR, ZONE_DIR, SOCIAL_DIR, QUEST_DIR, nil);
}

void heart_beat_dump(void)
{
    dump_state();
}
