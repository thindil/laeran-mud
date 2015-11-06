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

    number = -1;
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
