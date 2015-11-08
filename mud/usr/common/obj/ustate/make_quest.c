#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

#include <gameconfig.h>


inherit USER_STATE;

private string quest;
private int substate;
private int stages;
private int stage;

/* substates */
#define SS_PROMPT_NAME            1
#define SS_PROMPT_REWARD          2
#define SS_PROMPT_STAGES_AMOUNT   3
#define SS_PROMPT_STAGE_TEXT      4
#define SS_PROMPT_STAGE_CONDITION 5
#define SS_PROMPT_FINISH          6

/* Input function return values */
#define RET_NORMAL                  1
#define RET_POP_STATE               2

/* Prototypes */
static int  prompt_name_input(string input);
static int  prompt_stages_amount_input(string input);
static void prompt_stage_text_data(mixed data);
static int  prompt_stage_condition_input(string input);
static int  prompt_reward_input(string input);
private string blurb_for_substate(int substate);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone)
{
    ::create();
    if (clone) {
        substate = SS_PROMPT_NAME;
        stages = 2;
        stage = 1;
        quest = "~quest{\n";
    }
}

/* Setting up state */
void set_up_func(varargs string verb)
{
}

/* Handles input from user. Depends on current substate. */
int from_user(string input)
{
    int ret;
    string quitcheck;

    if (input) {
        quitcheck = STRINGD->trim_whitespace(input);
        if (!STRINGD->stricmp(quitcheck, "wyjdz")) {
            send_string("(Wychodzenie z OLC - Przerwano!)\n");
            pop_state();
            return MODE_ECHO;
        }
    }

    switch (substate) {
        case SS_PROMPT_NAME:
            ret = prompt_name_input(input);
            break;
        case SS_PROMPT_STAGES_AMOUNT:
            ret = prompt_stages_amount_input(input);
            break;
        case SS_PROMPT_STAGE_TEXT:
            send_string("Wewnętrzny błąd OLC, przerwanie!\n");
            LOGD->write_syslog("Osiągnieto from_user() w stanie " + substate
                    + " podczas robienia @make_quest.", LOG_ERR);
            pop_state();
            return MODE_ECHO;
        case SS_PROMPT_STAGE_CONDITION:
            ret = prompt_stage_condition_input(input);
            break;
        case SS_PROMPT_REWARD:
            ret = prompt_reward_input(input);
            break;
        default:
            send_string("Nieznany stan! Przerwanie OLC!\n");
            pop_state();
            return MODE_ECHO;
    }

    switch (ret) {
        case RET_NORMAL:
            send_string(" > ");
            break;
        case RET_POP_STATE:
            pop_state();
            break;
        default:
            send_string("Zwrócono nieznaną wartość! Przerwanie OLC!\n");
            break;
    }

    return MODE_ECHO;
}

/* Something was sent to user by mud, ignore it. */
void to_user(string output) {
}

/* Send text based on current substate. */
private string blurb_for_substate(int substate)
{
    string tmp;

    switch (substate) {
        case SS_PROMPT_NAME:
            return "Wpisz nazwę przygody lub 'wyjdź' aby wyjść.\n";
        case SS_PROMPT_REWARD:
            return "Podaj co jest nagrodą za przygodę. Możliwe wartości: 'skill:nazwa umiejętności:doświadczenie'\n"
                + "punkty doświadczenia w wybranej umiejętności. 'item:numer przedmiotu' przedmiot o wybranym\n"
                + "numerze (stanie się on rodzicem nagrody).\n";
        case SS_PROMPT_STAGES_AMOUNT:
            return "Podaj ilość etapów jakie posiada przygoda. Musi to być wartość pomiędzy 2 a 100\n"
                + "(jeżeli chcesz spędzić resztę życia na uzupełnianiu tylu informacji).\n";
        case SS_PROMPT_STAGE_TEXT:
            return "Etap nr: " + stage + "\nWprowadź tekst jaki zobaczy gracz kiedy przejdzie do tego etapu "
                + "przygody.\n";
        case SS_PROMPT_STAGE_CONDITION:
            return "Etap nr: " + stage + "\nWprowadź warunek jaki jest potrzebny do spełnienia aby wejść do "
                + "tego etapu przygody.\nMożliwe wartości: 'item:numer przedmiotu' trzeba posiadać przedmiot, "
                + "którego archetypem\njest przedmiot o podanym numerze. 'npc:numer mobka' trzeba pozdrowić "
                + "mobka o wybranym\nnumerze. 'room:numer pokoju' trzeba wejść do pokoju o wybranym numerze.\n";
        default:
            return "<NIEZNANY STAN>\n";
    }
}

/* switching to state/substate if pushp is true,
 * we just entered this state. */
void switch_to(int pushp)
{
    if (pushp && (substate == SS_PROMPT_NAME 
                || substate == SS_PROMPT_STAGES_AMOUNT
                || substate == SS_PROMPT_STAGE_TEXT 
                || substate == SS_PROMPT_STAGE_CONDITION
                || substate == SS_PROMPT_REWARD )) {
        send_string(blurb_for_substate(substate));
        send_string("Możesz również wpisać 'wyjdz' aby przerwać pracę nad przygodą.\n");
        send_string(" > ");
    } else if (substate == SS_PROMPT_STAGE_TEXT
            || substate == SS_PROMPT_STAGE_CONDITION) {
        /* do nothing */
    } else {
        send_string("(Tworzenie przygody -- powrót)\n");
        send_string(" > ");
    }
}

void switch_from(int popp)
{
    if (popp && (substate == SS_PROMPT_NAME 
                || substate == SS_PROMPT_STAGES_AMOUNT
                || substate == SS_PROMPT_STAGE_TEXT 
                || substate == SS_PROMPT_STAGE_CONDITION
                || substate == SS_PROMPT_REWARD ))
        send_string("(Tworzenie przygody -- przerwanie)\n");
}

void pass_data(mixed data)
{
    switch(substate) {
        case SS_PROMPT_STAGE_TEXT:
            prompt_stage_text_data(data);
            break;
        default:
            send_string("Ostrzeżenie: Nieznane dane z nieznanego stanu!\n");
            break;
    }
}

static int prompt_name_input(string input)
{
    if (input)
        input = STRINGD->trim_whitespace(input);
    if (!input || STRINGD->is_whitespace(input)) {
        send_string("Musisz podać nazwę przygody. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }

    quest += "  ~name{" + input + "}\n";
    send_string("\nZaakceptowano nazwę przygody.\n");
    substate = SS_PROMPT_REWARD;
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
}

static int prompt_reward_input(string input)
{
    string *parts;

    if (input)
        input = STRINGD->trim_whitespace(input);
    if (!input || STRINGD->is_whitespace(input)) {
        send_string("Musisz podać nagrodę za przygodę. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }
    
    parts = explode(input, ":");
    switch (parts[0]) {
        case "item":
            if(sizeof(parts) < 2) {
                send_string("Musi być item:numer przedmiotu. Spróbujmy ponownie.\n");
                send_string(blurb_for_substate(substate));
                return RET_NORMAL;
            }
            break;
        case "skill":
            if(sizeof(parts) < 3) {
                send_string("Musi być skill:nazwa umiejętności:doświadczenie. Spróbujmy ponownie.\n");
                send_string(blurb_for_substate(substate));
                return RET_NORMAL;
            }
            break;
        default:
            send_string("Musi być odpowiednia opcja, item lub skill. Spróbujmy ponownie.\n");
            send_string(blurb_for_substate(substate));
            return RET_NORMAL;
    }

    quest += "  ~reward{" + input + "}\n";
    send_string("\nZaakceptowano nagrodę za przygodę.\n");
    substate = SS_PROMPT_STAGES_AMOUNT;
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
}

static int prompt_stages_amount_input(string input)
{
    int value;

    if (input)
        input = STRINGD->trim_whitespace(input);
    if (!input || STRINGD->is_whitespace(input) || sscanf(input, "%d", value) != 1) {
        send_string("Musisz podać liczbę etapów przygody. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }

    if (value < 2 || value > 100) {
        send_string("Liczba etapów musi być wartością od 2 do 100. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }

    stages = value;
    send_string("\nZaakceptowano liczbę etapów.\n");
    substate = SS_PROMPT_STAGE_TEXT;
    send_string(blurb_for_substate(substate));
    push_new_state(US_ENTER_DATA);
    return RET_NORMAL;
}

static void prompt_stage_text_data(mixed data)
{
    if (typeof(data) != T_STRING) {
        send_string("Dziwne dane wprowadzone do stanu! Przerywamy.\n");
        pop_state();
        return;
    }

    if (!data || STRINGD->is_whitespace(data)) {
        send_string("Musisz podać jakiś tekst. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(SS_PROMPT_STAGE_TEXT));
        push_new_state(US_ENTER_DATA);
        return;
    }
    data = STRINGD->trim_whitespace(data);
    quest += "  ~stage{\n"
        + "    ~text{" + data + "}\n";
    send_string("\nZaakceptowano tekst etapu przygody.\n");
    if (stage > 1)
        substate = SS_PROMPT_STAGE_CONDITION;
    else
        stage++;
    send_string(blurb_for_substate(substate));
    if (stage == 2 && substate == SS_PROMPT_STAGE_TEXT) {
        quest += "  }\n";
        push_new_state(US_ENTER_DATA);
    }
}

static int prompt_stage_condition_input(string input)
{
    string *parts;

    if (input)
        input = STRINGD->trim_whitespace(input);
    if (!input || STRINGD->is_whitespace(input)) {
        send_string("Musisz podać warunek aby przejść na ten etap przygody. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }
    
    parts = explode(input, ":");
    if(sizeof(parts) < 2) {
        send_string("Musi być item:numer przedmiotu, npc:numer mobka lub room:numer pokoju. Spróbujmy ponownie.\n");
        send_string(blurb_for_substate(substate));
        return RET_NORMAL;
    }
    quest += "    ~condition{" + input + "}\n"
        + "  }\n";
    send_string("\nZaakceptowano warunek etapu przygody.\n");
    stage++;
    if (stage <= stages) {
        substate = SS_PROMPT_STAGE_TEXT;
        send_string(blurb_for_substate(substate));
        push_new_state(US_ENTER_DATA);
        return RET_NORMAL;
    } else {
        substate = SS_PROMPT_FINISH;
        QUESTD->from_unq_text(quest + "}\n");
        send_string("Zakończono prace nad nową przygodą.\n");
        return RET_POP_STATE;
    }
}

