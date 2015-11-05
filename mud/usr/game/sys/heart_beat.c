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
      TIMED->set_heart_beat(TIMED_HALF_MINUTE, "heart_beat_func");
      TIMED->set_heart_beat(TIMED_ONE_HOUR, "heart_beat_dump");
      TIMED->set_heart_beat(TIMED_ONE_DAY, "heart_beat_save");
    }
}

/* delete dropped objects, add packages to postman */
void heart_beat_clear(void)
{
    int *rooms, *mobiles;
    int i, packages;
    object obj;
    mixed tag;

    if(previous_program() != TIMED)
        return;
    rooms = ({ });
    for(i = 0; i < ZONED->num_zones(); i++)
        rooms += MAPD->rooms_in_zone(i);
    for (i = 0; i < sizeof(rooms); i++) {
        obj = MAPD->get_room_by_num(rooms[i]);
        tag = TAGD->get_tag_value(obj, "DropTime");
        if (tag && time() - tag >= 300) {
            if (obj->get_location())
                obj->get_location()->remove_from_container(obj);
            destruct_object(obj);
        }
    }
    mobiles = MOBILED->all_mobiles();
    for (i = 0; i < sizeof(mobiles); i++) {
        obj = MOBILED->get_mobile_by_num(mobiles[i]);
        if (obj->get_type() == "postman") {
            packages = obj->get_packages();
            packages += 5;
            if (packages > 100)
                packages = 100;
            obj->set_packages(packages);
        }
    }
}

void heart_beat_func(void)
{
    int *mobiles, *rooms;
    int i, zone, roll, j;
    object body, parentbody, room, mobile, item;
    mixed *inventory;

    if (previous_program() != TIMED)
        return;
    mobiles = MOBILED->all_mobiles();
    for (i = 0; i < sizeof(mobiles); i++) {
        mobile = MOBILED->get_mobile_by_num(mobiles[i]);
        /* spawn mobiles bodies */
        if (!mobile->get_body() && mobile->get_parentbody()) {
            body = clone_object(SIMPLE_ROOM);
            parentbody = MAPD->get_room_by_num(mobile->get_parentbody());
            body->add_archetype(parentbody);
            body->set_brief(nil);
            body->set_look(nil);
            if (parentbody->is_container())
                body->set_container(1);
            if (parentbody->is_open())
                body->set_open(1);
            if (parentbody->is_openable())
                body->set_openable(1);
            if (mobile->get_spawnroom() > 0){
                room = MAPD->get_room_by_num(mobile->get_spawnroom());
                zone = ZONED->get_zone_for_room(room);
            } else {
                zone = ZONED->get_zone_for_room(parentbody->get_location());
                rooms = MAPD->rooms_in_zone(zone);
                roll = mobile->get_parentbody();
                while (roll == mobile->get_parentbody()) {
                    roll = random(sizeof(rooms));
                    if (MAPD->get_room_by_num(rooms[roll])->num_exits() < 1)
                        roll = mobile->get_parentbody();
                }
                room = MAPD->get_room_by_num(rooms[roll]);
            }
            MAPD->add_room_to_zone(body, -1, zone);
            room->add_to_container(body);
            inventory = parentbody->objects_in_container();
            for (j = 0; j < sizeof(inventory); j++) {
                item = clone_object(SIMPLE_ROOM);
                item->add_archetype(inventory[j]);
                item->set_brief(nil);
                item->set_look(nil);
                if (inventory[j]->is_container())
                    item->set_container(1);
                if (inventory[j]->is_open())
                    item->set_open(1);
                if (inventory[j]->is_openable())
                    item->set_openable(1);
                if (inventory[j]->is_wearable())
                    item->set_wearable(1);
                MAPD->add_room_to_zone(item, -1, zone);
                body->add_to_container(item);
            }
            mobile->assign_body(body);
        }
        /* heal wounded mobiles (not players) */
        if (mobile->get_body() && TAGD->get_tag_value(mobile->get_body(), "Hp") 
                && !TAGD->get_tag_value(mobile->get_body(), "Combat")
                && mobile->get_type() != "user") {
            TAGD->set_tag_value(mobile->get_body(), "Hp", nil);
            if (mobile->get_user()) {
                mobile->get_user()->message("Jesteś już kompletnie zdrowy.\n");
                mobile->get_user()->set_health(mobile->get_body()->get_hp());
            }
        }
        /* Move wander type mobiles */
        if (mobile->get_type() == "wander" && !TAGD->get_tag_value(mobile->get_body(), "Combat"))
            mobile->random_move();
    }
}

void heart_beat_save(void)
{
  find_object(INITD)->save_mud_data(nil, ROOM_DIR, MOB_DIR, ZONE_DIR, SOCIAL_DIR, nil);
}

void heart_beat_dump(void)
{
    dump_state();
}
