#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

/* Prototypes */
void upgraded(varargs int clone);

/* Data for world */
int hour, minutes;

static void create(varargs int clone)
{
    if (clone)
        error("Nie można sklonować tego obiektu!");

    hour = 0;
    minutes = 0;
    upgraded();
}

void upgraded(varargs int clone)
{
    if (!SYSTEM() && !COMMON() && !GAME())
        return;
}

void save_world_data(void)
{
    save_object(WORLD_DATA);
}

void load_world_data(void)
{
    restore_object(WORLD_DATA);
}

void set_time(int add_hour, int add_minutes)
{
    string msg;
    int i;
    object* users;

    hour += add_hour;
    minutes += add_minutes;
    if (minutes >= 60) {
        hour++;
        minutes -= 60;
    }
    if (hour >= 24)
        hour = 0;

    msg = "";
    if (hour == 6 && !minutes)
        msg = "Wstaje ranek.\n";
    else if (hour == 12 && !minutes)
        msg = "Wybija południe.\n";
    else if (hour == 18 && !minutes)
        msg = "Nadciąga wieczór.\n";
    else if (hour == 22 && !minutes)
        msg = "Zapada noc.\n";
    else if (!hour && !minutes)
        msg = "Wybiła północ.\n";

    if (msg) {
        users = users();
        for (i = 0; i < sizeof(users); i++) {
            if (users[i]->get_location()->get_room_type() == 1
                    || users[i]->get_location()->get_room_type() == 2) 
                        users[i]->message(msg);
        }   
    }
}

string get_hour(void)
{
    if (!hour)
        return "Północ. ";
    else if (hour > 0 && hour < 12)
        return "Godzina " + (string)hour + ". ";
    else if (hour == 12)
        return "Południe. ";
    else
        return "Godzina" + (string)(hour - 12) + " po południu. ";
}
