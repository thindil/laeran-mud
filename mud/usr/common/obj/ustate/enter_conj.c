#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>


inherit USER_STATE;

private string nouns;
private int substate;
private object user_body;

/* substates */
#define SS_PROMPT_DOPELNIACZ      1 
#define SS_PROMPT_CELOWNIK        2
#define SS_PROMPT_BIERNIK         3
#define SS_PROMPT_NARZEDNIK       4
#define SS_PROMPT_MIEJSCOWNIK     5
#define SS_PROMPT_WOLACZ          6
#define SS_PROMPT_FINISH          7

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
        substate = SS_PROMPT_DOPELNIACZ;
        nouns = "";
        user_body = nil;
    }
}

/* Setting up state */
void set_up_func(varargs object user)
{
    nouns = user->get_name();
    user_body = user->get_body();
}

/* Handles input from user. Depends on current substate. */
int from_user(string input)
{
    int ret;
    string quitcheck;

    if (input) {
        quitcheck = STRINGD->trim_whitespace(input);
        if (!STRINGD->stricmp(quitcheck, "wyjdz")) {
            send_string("(Ustawianie odmiany - Przerwano!)\n");
            pop_state();
            return MODE_ECHO;
        }
    }

    switch (substate) {
        case SS_PROMPT_DOPELNIACZ:
        case SS_PROMPT_CELOWNIK:
        case SS_PROMPT_BIERNIK:
        case SS_PROMPT_NARZEDNIK:
        case SS_PROMPT_MIEJSCOWNIK:
        case SS_PROMPT_WOLACZ:
            ret = prompt_input(input);
            break;
        default:
            send_string("Nieznany stan! Przerwanie ustawiania odmiany!\n");
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
            send_string("Zwrócono nieznaną wartość! Przerwanie ustawiania odmiany!\n");
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
        case SS_PROMPT_DOPELNIACZ:
            return "Podaj dopełniacz dla Twojego imienia (Kogo nie ma?):\n";
        case SS_PROMPT_CELOWNIK:
            return "Podaj celownik dla Twojego imienia (Komu się przyglądam?):\n";
        case SS_PROMPT_BIERNIK:
            return "Podaj biernik dla Twojego imienia (Kogo widzę?):\n";
        case SS_PROMPT_NARZEDNIK:
            return "Podaj narzędnik dla Twojego imienia (Z kim idę?):\n";
        case SS_PROMPT_MIEJSCOWNIK:
            return "Podaj miejscownik dla Twojego imienia (O kim mówię?):\n";
        case SS_PROMPT_WOLACZ:
            return "Podaj wołacz dla Twojego imienia (wołając Ciebie):\n";
        default:
            return "<NIEZNANY STAN>\n";
    }
}

/* switching to state/substate if pushp is true,
 * we just entered this state. */
void switch_to(int pushp)
{
    if (pushp && (substate == SS_PROMPT_DOPELNIACZ 
                || substate == SS_PROMPT_CELOWNIK
                || substate == SS_PROMPT_BIERNIK 
                || substate == SS_PROMPT_NARZEDNIK
                || substate == SS_PROMPT_MIEJSCOWNIK 
                || substate == SS_PROMPT_WOLACZ)) {
        send_string(blurp_for_substate(substate));
        send_string("Możesz również wpisać 'wyjdz' aby przerwać pracę nad odmianą imienia.\n");
        send_string(" > ");
    } else {
        send_string("(Ustawianie odmiany imienia -- powrót)\n");
        send_string(" > ");
    }
}

void switch_from(int popp)
{
    if (popp && (substate == SS_PROMPT_DOPELNIACZ 
                || substate == SS_PROMPT_CELOWNIK
                || substate == SS_PROMPT_BIERNIK 
                || substate == SS_PROMPT_NARZEDNIK
                || substate == SS_PROMPT_MIEJSCOWNIK 
                || substate == SS_PROMPT_WOLACZ))
        send_string("(Ustawianie odmiany imienia -- przerwanie)\n");
}

void pass_data(mixed data)
{
}

static int prompt_input(string input)
{
    if (input)
        input = STRINGD->trim_whitespace(input);

    if (!input) {
        send_string("\nMusisz podać jakiś tekst. Spróbujmy ponownie.\n");
        send_string(blurp_for_substate(substate));
        return RET_NORMAL;
    }

    switch (substate) {
        case SS_PROMPT_DOPELNIACZ:
            send_string("\nDopełniacz ustawiony.\n");
            substate = SS_PROMPT_CELOWNIK;
            break;
        case SS_PROMPT_CELOWNIK:
            send_string("\nCelownik ustawiony.\n");
            substate = SS_PROMPT_BIERNIK;
            break;
        case SS_PROMPT_BIERNIK:
            send_string("\nBiernik ustawiony.\n");
            substate = SS_PROMPT_NARZEDNIK;
            break;
        case SS_PROMPT_NARZEDNIK:
            send_string("\nNarzednik ustawiony.\n");
            substate = SS_PROMPT_MIEJSCOWNIK;
            break;
        case SS_PROMPT_MIEJSCOWNIK:
            send_string("\nMiejscownik ustawiony.\n");
            substate = SS_PROMPT_WOLACZ;
            break;
        case SS_PROMPT_WOLACZ:
            send_string("\nWołacz ustawiony.\n");
            substate = SS_PROMPT_FINISH;
            break;
        default:
            break;
    }

    if (substate != SS_PROMPT_FINISH) {
        nouns += "," + input;
        send_string(blurp_for_substate(substate));
        return RET_NORMAL;
    } else {
        user_body->clear_nouns();
        user_body->add_noun(PHRASED->new_simple_english_phrase(nouns));
        send_string("Ustawiłeś odmianę swojego imienia.\n");
        return RET_POP_STATE;
    }
}
