#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

inherit MOBILE;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone)
{
    ::create(clone);

    upgraded(clone);
}


void upgraded(varargs int clone)
{
}

string get_type(void) 
{
    return "alchemy";
}

/* Initiate communication */
void hook_social(object body, object target, string verb)
{
    if (target && get_number() == target->get_mobile()->get_number() && verb == "pozdrow") {
        if (TAGD->get_tag_value(this_object(), "Quest") != nil) { 
            if (!QUESTD->was_on_quest(TAGD->get_tag_value(this_object(), "Quest"), body->get_mobile())) 
                whisper(body, "Witaj, jak rozumiem, jesteś początkującym adeptem alchemii? Może szukasz wiedzy?\n"
                        + "Jeżeli tak, szepnij do mnie 'szukam wiedzy' a podpowiem Ci jak możesz nauczyć się\n"
                        + "podstaw alchemicznej transformacji.\n");
        } else
            whisper(body, "Witaj, obawiam się, że na razie nie mam nic do zaoferowania Tobie.\n");
    }
}

/* Start alchemy quests */
void hook_whisper(object body, string message)
{
    switch (message) {
        case "szukam wiedzy":
            if (TAGD->get_tag_value(this_object(), "Quest") == nil) {
                whisper(body, "Niestety, na razie nie mogę Tobie pomóc, proszę wróć za jakiś czas\n");
                return;
            }
            if (QUESTD->was_on_quest(TAGD->get_tag_value(this_object(), "Quest"), body->get_mobile())) {
                whisper(body, "Niestety, nauczyłem Ciebie już wszystkiego co mogłem.\n");
                return;
            }
            QUESTD->start_quest(TAGD->get_tag_value(this_object(), "Quest"), body->get_mobile());
            break;
        default:
            break;
    }
}

void from_dtd_unq(mixed* unq) 
{
    /* Set the body, location and number fields */
    unq = mobile_from_dtd_unq(unq);
}
