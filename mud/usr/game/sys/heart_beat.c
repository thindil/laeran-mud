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
    }
}

/* delete dropped objects */
void heart_beat_clear(void)
{
  int* rooms;
  int i;
  object obj;
  mixed tag;
  
  if(previous_program() == TIMED)
    {
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
}
