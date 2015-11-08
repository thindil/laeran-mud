#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

inherit unq DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);

/* Data from QuestD file*/
string** quests;
string*** stages;

static void create(varargs int clone) 
{
    if (clone) 
        error("Nie można sklonować tego obiektu!");

    quests = ({ });
    stages = ({ });
    unq::create(clone);
    upgraded();
}

void upgraded(varargs int clone)
{
    if (!SYSTEM() && !COMMON() && !GAME())
        return;
    set_dtd_file(QUESTD_DTD);

    unq::upgraded();
}

string to_unq_text(int number)
{
    string questtmp;
    int i;

    questtmp = "~quest{\n"
           + "  ~name{" + quests[number][0] + "}\n"
           + "  ~reward{" + quests[number][1] + "}\n";
    for (i = 0; i < sizeof(stages[number]); i++) {
        questtmp += "  ~stage{\n"
                + "    ~text{" + stages[number][i][0] + "}\n";
        if (stages[number][i][1] != "")
            questtmp += "    ~condition{" + stages[number][i][1] + "}\n";
        questtmp += "  }\n";
    }
    questtmp += "}\n";

    return questtmp;
}

void from_unq_text(string unq_text) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    unq::from_unq_text(unq_text);
}

void from_unq(mixed *unq) 
{
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    unq::from_unq(unq);
}

void from_dtd_unq(mixed* unq) 
{
    mixed *quest_unq, *stage_unq, *stage;
    int number;
    
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    if (!sizeof(quests))
        number = -1;
    else
        number = sizeof(quests) - 1;
    while(sizeof(unq) > 0) {
        if (unq[0] == "quest") {
            quest_unq = unq[1];
            number++;

            while(sizeof(quest_unq) > 0) {
                switch (quest_unq[0][0]) {
                    case "name":
                        quests += ({ ({ quest_unq[0][1] }) });
                        break;
                    case "reward":
                        quests[number] += ({ quest_unq[0][1] });
                        break;
                    case "stage":
                        stage_unq = quest_unq[0][1];
                        stage = ({ stage_unq[0][1] });
                        if (sizeof(stage_unq) == 2) 
                            stage += ({ stage_unq[1][1] });
                        else
                            stage += ({ "" });
                        if ((number + 1) <= sizeof(stages))
                            stages[number] += ({ stage });
                        else
                            stages += ({ ({ stage }) });
                        break;
                    default:
                        error("Nieznany tag '" + STRINGD->mixed_sprint(quest_unq[0])
                                + "' w pliku z przygodami!");
                        break;
                }
                quest_unq = quest_unq[1..];
            }
        }
        unq = unq[2..];
    }
}

int num_quests(void)
{
    if (!sizeof(quests))
        return 0;
    return sizeof(quests);
}

void start_quest(float number, object mobile)
{
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    TAGD->set_tag_value(mobile, "Quest", number);
    mobile->get_user()->message(stages[(int)number][0][0] + "\n");
}

void progress_quest(object mobile)
{
    int qnumber, snumber;
    float number;
    string *reward;
    object item, new_object;
    string result;

    if(!SYSTEM() && !COMMON() && !GAME())
        return;
    
    number = TAGD->get_tag_value(mobile, "Quest");
    qnumber = (int)modf(number)[1];
    snumber = (int)(modf(number)[0] * 100.0) + 1;
    if ((snumber + 1) < sizeof(stages[qnumber])) {
        TAGD->set_tag_value(mobile, "Quest", number + 0.01);
        mobile->get_user()->message(stages[qnumber][snumber][0] + "\n");
    } else {
        TAGD->set_tag_value(mobile, "Quest", nil);
        mobile->get_user()->message(stages[qnumber][snumber][0] + "\n");
        reward = explode(quests[qnumber][1], ":");
        switch (reward[0]) {
            case "skill":
                mobile->get_user()->gain_exp(reward[1], (int)reward[2]);
                break;
            case "item":
                item = MAPD->get_room_by_num(reward[1]);
                new_object = clone_object(SIMPLE_ROOM);
                MAPD->add_room_to_zone(new_object, -1, ZONED->get_zone_for_room(mobile->get_location()));
                new_object->add_archetype(item);
                new_object->set_brief(nil);
                new_object->set_look(nil);
                mobile->get_location()->add_to_container(new_object);
                if (item->is_container())
                    new_object->set_container(1);
                if (item->is_open())
                    new_object->set_open(1);
                if (item->is_openable())
                    new_object->set_openable(1);
                if (item->is_weapon())
                    new_object->set_weapon(1);
                if (item->is_wearable())
                    new_object->set_wearable(1);
                result = mobile->place(new_object, mobile->get_body());
                if (result) 
                    mobile->get_user()->message(result + "\nTwoja nagroda upada na ziemię.\n");
                break;
            default:
                error("Nieznana nagroda za przygodę.");
        }
    }
}

string* get_condition(float number)
{
    int qnumber, snumber;

    if(!SYSTEM() && !COMMON() && !GAME())
        return nil;

    qnumber = (int)modf(number)[1];
    snumber = (int)(modf(number)[0] * 100.0) + 1;

    return explode(stages[qnumber][snumber][1], ":");
}

string* get_quests(void)
{
    string *qnames;
    int i;

    qnames = ({ });
    for (i = 0; i < sizeof(quests); i++)
        qnames += ({ quests[i][0] });

    return qnames;
}
