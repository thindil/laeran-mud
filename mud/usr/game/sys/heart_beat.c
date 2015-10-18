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
      /*TIMED->set_heart_beat(TIMED_HALF_MINUTE, "heart_beat_spawn");*/
    }
}

/* delete dropped objects */
void heart_beat_clear(void)
{
  int* rooms;
  int i;
  object obj;
  mixed tag;
  
  if(previous_program() != TIMED)
    {
      return;
    }
  rooms = ({ });
  for(i = 0; i < ZONED->num_zones(); i++)
    {
      rooms += MAPD->rooms_in_zone(i);
    }
  for (i = 0; i < sizeof(rooms); i++)
    {
      obj = MAPD->get_room_by_num(rooms[i]);
      tag = TAGD->get_tag_value(obj, "DropTime");
      if (tag && time() - tag >= 300)
	{
	  if (obj->get_location())
	    {
	      obj->get_location()->remove_from_container(obj);
	    }
	  destruct_object(obj);
	}
    }
}

/* spawn mobiles bodies */
void heart_beat_spawn(void)
{
  int *mobiles, *rooms;
  int i, zone, roll;
  object body, parentbody, room;
  
  if (previous_program() != TIMED)
    {
      return;
    }
  mobiles = MOBILED->all_mobiles();
  for (i = 0; i < sizeof(mobiles); i++)
    {
      if (!mobiles[i]->get_body() && mobiles[i]->get_parentbody())
	{
	  body = clone_object(SIMPLE_ROOM);
	  parentbody = MAPD->get_room_by_num(mobiles[i]->get_parentbody());
	  body->add_archetype(parentbody);
	  body->set_brief(nil);
	  body->set_look(nil);
	  if (parentbody->is_container())
	    {
	      body->set_container(1);
	    }
	  if (parentbody->is_open())
	    {
	      body->set_open(1);
	    }
	  if (parentbody->is_openable())
	    {
	      body->set_openable(1);
	    }
	  if (mobiles[i]->get_spawnroom())
	    {
	      room = MAPD->get_room_by_num(mobiles[i]->get_spawnroom());
	      zone = ZONED->get_zone_for_room(room);
	    }
	  else
	    {
	      zone = ZONED->get_zone_for_room(mobiles[i]->get_parentbody());
	      rooms = MAPD->rooms_in_zone(zone);
	      roll = -1;
	      while (roll == mobiles[i]->get_parentbody())
		{
		  roll = random(sizeof(rooms));
		  if (rooms[roll] == mobiles[i]->get_parentbody())
		    {
		      roll = mobiles[i]->get_parentbody();
		    }
		}
	    }
	  MAPD->add_room_to_zone(body, -1, zone);
	  room->add_to_container(body);
	}
    }
}
