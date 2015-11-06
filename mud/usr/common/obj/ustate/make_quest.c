#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>


inherit USER_STATE;

private string quest;
private int substate;
private int stages;

/* substates */
#define SS_PROMPT_NAME            1
#define SS_PROMPT_STAGES_AMOUNT   2
#define SS_PROMPT_STAGE_TEXT      3
#define SS_PROMPT_STAGE_CONDITION 4
#define SS_PROMPT_REWARD          5
#define SS_PROMPT_FINISH          6

/* Input function return values */
#define RET_NORMAL                  1
#define RET_POP_STATE               2

/* Prototypes */
static int prompt_input(string input);
private string blurp_for_substate(int substate);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone)
{
    ::create();
    if (clone) {
        substate = SS_PROMPT_VERB;
        stages = 1;
        social = "~quest{\n";
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
        case SS_PROMPT_STAGES_AMOUNT:
        case SS_PROMPT_STAGE_TEXT:
        case SS_PROMPT_STAGE_CONDITION:
        case SS_PROMPT_REWARD:
            ret = prompt_input(input);
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
private string blurp_for_substate(int substate)
{
    string tmp;

    switch (substate) {
        case SS_PROMPT_NAME:
            return "Wpisz nazwę przygody lub 'wyjdź' aby wyjść.\n";
        case SS_PROMPT_STAGES_AMOUNT:
            return "";
        case SS_PROMPT_STAGE_TEXT:
            return "";
        case SS_PROMPT_STAGE_CONDITION:
            return "";
        case SS_PROMPT_REWARD:
            return "";
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
        send_string(blurp_for_substate(substate));
        send_string("Możesz również wpisać 'wyjdz' aby przerwać pracę nad przygodą.\n");
        send_string(" > ");
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
}

static int prompt_input(string input)
{
    if (input)
        input = STRINGD->trim_whitespace(input);
}
