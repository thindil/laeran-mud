#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

inherit MOBILE;

/* Member variables */
private int registered;

/* prototypes */
void upgraded(varargs int clone);

static void create(varargs int clone) {
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
    if (clone && !registered) {
        TIMED->set_heart_beat(TIMED_HALF_MINUTE, "heart_beat");
        registered = 1;
    }
}

string get_type(void) 
{
    return "wander";
}

void heart_beat(void)
{
    int    num_ex, ctr, zone, roll;
    object exit, dest, new_body, parent_body, room, item;
    int *rooms;
    mixed *inventory;

    if (body && TAGD->get_tag_value(body, "Combat"))
        return;

    /* Spawn mobile */
    if (!body && parentbody) {
        new_body = clone_object(SIMPLE_ROOM);
        parent_body = MAPD->get_room_by_num(parentbody);
        new_body->add_archetype(parent_body);
        new_body->set_brief(nil);
        new_body->set_look(nil);
        if (parent_body->is_container())
            new_body->set_container(1);
        if (parent_body->is_open())
            new_body->set_open(1);
        if (parent_body->is_openable())
            new_body->set_openable(1);
        if (spawnroom > 0){
            room = MAPD->get_room_by_num(spawnroom);
            zone = ZONED->get_zone_for_room(room);
        } else {
            zone = ZONED->get_zone_for_room(parent_body->get_location());
            rooms = MAPD->rooms_in_zone(zone);
            roll = parentbody;
            while (roll == parentbody) {
                roll = random(sizeof(rooms));
                if (MAPD->get_room_by_num(rooms[roll])->num_exits() < 1)
                    roll = parentbody;
            }
            room = MAPD->get_room_by_num(rooms[roll]);
        }
        MAPD->add_room_to_zone(new_body, -1, zone);
        room->add_to_container(new_body);
        inventory = parent_body->objects_in_container();
        for (ctr = 0; ctr < sizeof(inventory); ctr++) {
            item = clone_object(SIMPLE_ROOM);
            item->add_archetype(inventory[ctr]);
            item->set_brief(nil);
            item->set_look(nil);
            if (inventory[ctr]->is_container())
                item->set_container(1);
            if (inventory[ctr]->is_open())
                item->set_open(1);
            if (inventory[ctr]->is_openable())
                item->set_openable(1);
            if (inventory[ctr]->is_wearable())
                item->set_wearable(1);
            MAPD->add_room_to_zone(item, -1, zone);
            new_body->add_to_container(item);
        }
        assign_body(new_body);
        return;
    }

    /* heal wounded mobile */
    if (body && TAGD->get_tag_value(body, "Hp")) {
        TAGD->set_tag_value(body, "Hp", nil);
        return;
    }

    /* move mobile */
    num_ex = location->num_exits();
    if(num_ex > 0) {
        int dir;

        ctr = random(100);
        if (ctr >= 60)
            return;
        ctr = random(num_ex);
        exit = location->get_exit_num(ctr);
        if(!exit)
            error("Błąd wewnętrzny! Nie mogę pobrać wyjścia!");

        dir = exit->get_direction();
        dest = exit->get_destination();

        if (ZONED->get_zone_for_room(dest) != ZONED->get_zone_for_room(location))
            return;

        move(dir);
    }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
