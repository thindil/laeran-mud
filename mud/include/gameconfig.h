/*
 * Gameconfig.h
 *
 * This file tells Phantasmal things about your configuration.  You can
 * also keep your own stuff in here
 *
 */

/*
 * You have to define GAME_INITD so that Phantasmal knows what to call
 * when you start up.  Other configuration is done in this object's
 * create() function, which will call ConfigD to set everything else
 * that Phantasmal needs to know.
 */
#define GAME_INITD             "/usr/game/initd"

#define GAME_DRIVER            "/usr/game/sys/gamedriver"
#define HEART_BEAT             "/usr/game/sys/heart_beat"
#define COMBAT                 "/usr/game/obj/combat"

#define START_ROOM             100
#define LOCKER_ROOM            1
#define HOSPITAL_ROOM          417
