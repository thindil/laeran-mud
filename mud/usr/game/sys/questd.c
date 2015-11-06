#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

inherit unq DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);

/* Data from QuestD file*/
string* quests;
mixed*  stages;

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
           + "  ~number{" + number + "}\n"
           + "  ~name{" + quests[number] + "}\n";
    for (i = 0; i < sizeof(stages[number]); i++) {
        questtmp += "  ~stage{\n"
                + "    ~snumber{" + stages[number][0] + "}\n"
                + "    ~text{" + stages[number][1] + "}\n"
                + "    ~condition{" + stages[number][2] + "}\n"
                + "  }\n";
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
    mixed *quest_unq, *stage;
    int number;
    
    if(!SYSTEM() && !COMMON() && !GAME())
        return;

    stage = allocate(3);
    while(sizeof(unq) > 0) {
        if (unq[0] == "quest") {
            quest_unq = unq[1];

            while(sizeof(quest_unq) > 0) {
                switch (quest_unq[0][0]) {
                    case "number":
                        number = (int)quest_unq[0][1];
                    case "name":
                        quests += ({ quest_unq[0][1] });
                        stages += ({ });
                        break;
                    case "stage":
                        if (stage[0]) {
                            stages[number] += ({ stage });
                            error(STRINGD->mixed_sprint(stage));
                        }
                        stage = allocate(3);
                        break;
                    case "snumber":
                        stage[0] = quest_unq[0][1];
                        break;
                    case "text":
                        stage[1] = quest_unq[0][1];
                        break;
                    case "condition":
                        stage[2] = quest_unq[0][1];
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

    error("\n Quests: " + STRINGD->mixed_sprint(quests) + "\nStages: " + STRINGD->mixed_sprint(stages));
}
