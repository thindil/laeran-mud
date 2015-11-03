#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>


inherit USER_STATE;

private string social;
private int substate;

/* substates */
#define SS_PROMPT_VERB            1
#define SS_PROMPT_SELF_ONLY       2
#define SS_PROMPT_SELF_TARGET     3
#define SS_PROMPT_TARGET          4
#define SS_PROMPT_OTHER_ONLY      5
#define SS_PROMPT_OTHER_TARGET    6
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
        substate = SS_PROMPT_VERB;
        social = "~social{\n";
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
        case SS_PROMPT_VERB:
        case SS_PROMPT_SELF_ONLY:
        case SS_PROMPT_SELF_TARGET:
        case SS_PROMPT_TARGET:
        case SS_PROMPT_OTHER_ONLY:
        case SS_PROMPT_OTHER_TARGET:
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
        case SS_PROMPT_VERB:
            return "Wpisz nazwę komendy socjalnej lub 'wyjdź' aby wyjść.\n"
                + "Przykład: warkot\n";
        case SS_PROMPT_SELF_ONLY:
            return "Wpisz tekst jaki zobaczu gracz kiedy użyje komendy socjalnej bez\n"
                + "wybranego celu.\n"
                + "Przykład: Warczysz.\n";
        case SS_PROMPT_SELF_TARGET:
            return "Wpisz tekst jaki zobaczy gracz kiedy użyje komendy socjalnej z\n"
                + "wybranym celem. Jako imię celu podaj ~target{numer}, gdzie 'numer'"
                + "oznacza numer przypadku odmiany imienia.\n"
                + "Przykład: Warczysz na ~target{3}.\n";
        case SS_PROMPT_TARGET:
            return "Wpisz tekst jaki zobaczy osoba będąca celem komendy socjalnej\n"
                + "Jako imię osoby uruchamiającej komendę podaj ~actor{}.\n"
                + "Przykład: ~actor{} warczy na Ciebie.\n";
        case SS_PROMPT_OTHER_ONLY:
            return "Wpisz tekst jaki zobaczą inne osoby będące w tym samym pomieszczeniu\n"
                + "kiedy użyta zostanie komenda socjalna bez wybranego celu. Jako imię\n"
                + "osoby uruchamiającej komendę podaj ~actor{}.\n"
                + "Przykład: ~actor{} warczy.\n";
        case SS_PROMPT_OTHER_TARGET:
            return "Wpisz tekst jaki zobaczą inne osoby będące w tym samym pomieszczeniu\n"
                + "kiedy zostanie używa komenda socjalna z wybranym celem. Jako imię\n"
                + "osoby uruchamiającej komendę podaj ~actor{}, jako cel ~target{numer}"
                + "gdzie 'numer' oznacza numer przypadku odmiany imienia.\n"
                + "Przykład: ~actor{} warczy na ~target{3}.\n";
        default:
            return "<NIEZNANY STAN>\n";
    }
}

/* switching to state/substate if pushp is true,
 * we just entered this state. */
void switch_to(int pushp)
{
    if (pushp && (substate == SS_PROMPT_VERB 
                || substate == SS_PROMPT_SELF_ONLY
                || substate == SS_PROMPT_SELF_TARGET 
                || substate == SS_PROMPT_TARGET
                || substate == SS_PROMPT_OTHER_ONLY 
                || substate == SS_PROMPT_OTHER_TARGET)) {
        send_string(blurp_for_substate(substate));
        send_string("Możesz również wpisać 'wyjdz' aby przerwać pracę nad komendą.\n");
        send_string(" > ");
    } else {
        send_string("(Tworzenie komendy socjalnej -- powrót)\n");
        send_string(" > ");
    }
}

void switch_from(int popp)
{
    if (popp && (substate == SS_PROMPT_VERB 
                || substate == SS_PROMPT_SELF_ONLY
                || substate == SS_PROMPT_SELF_TARGET 
                || substate == SS_PROMPT_TARGET
                || substate == SS_PROMPT_OTHER_ONLY 
                || substate == SS_PROMPT_OTHER_TARGET))
        send_string("(Tworzenie komendy socjalnej -- przerwanie)\n");
}

void pass_data(mixed data)
{
}

static int prompt_input(string input)
{
    if (input)
        input = STRINGD->trim_whitespace(input);
    if (input && substate == SS_PROMPT_VERB) {
        input = STRINGD->to_lower(input);
        if (SOULD->is_social_verb(input)) {
            send_string("\nTaka komenda socjalna już istnieje. Spróbuj ponownie.\n");
            send_string(blurp_for_substate(substate));
            return RET_NORMAL;
        }
    }

    if (!input) {
        if (substate == SS_PROMPT_VERB) 
            send_string("\nMusisz podać nazwę komendy. Spróbujmy ponownie.\n");
        else
            send_string("\nMusisz podać jakiś tekst. Spróbujmy ponownie.\n");
        send_string(blurp_for_substate(substate));
        return RET_NORMAL;
    }

    switch (substate) {
        case SS_PROMPT_VERB:
            social += "  ~verb{" + input + "}\n";
            send_string("\nZaakceptowano nazwę komendy.\n");
            substate = SS_PROMPT_SELF_ONLY;
            break;
        case SS_PROMPT_SELF_ONLY:
            social += "  ~self-only{" + input + "}\n";
            send_string("\nZaakceptowano tekst dla siebie bez celu.\n");
            substate = SS_PROMPT_SELF_TARGET;
            break;
        case SS_PROMPT_SELF_TARGET:
            social += "  ~self-target{" + input + "}\n";
            send_string("\nZaakceptowano tekst dla siebie z celem.\n");
            substate = SS_PROMPT_TARGET;
            break;
        case SS_PROMPT_TARGET:
            social += "  ~target{" + input + "}\n";
            send_string("\nZaakceptowano tekst dla celu.\n");
            substate = SS_PROMPT_OTHER_ONLY;
            break;
        case SS_PROMPT_OTHER_ONLY:
            social += "  ~other-only{" + input + "}\n";
            send_string("\nZaakceptowano tekst dla innych bez celu.\n");
            substate = SS_PROMPT_OTHER_TARGET;
            break;
        case SS_PROMPT_OTHER_TARGET:
            social += "  ~other-target{" + input + "}\n";
            send_string("\nZaakceptowano tekst dla inny z celem.\n");
            substate = SS_PROMPT_FINISH;
            break;
        default:
            break;
    }

    if (substate != SS_PROMPT_FINISH) {
        send_string(blurp_for_substate(substate));
        return RET_NORMAL;
    } else {
        SOULD->from_unq_text(social + "}\n");
        send_string("Zakończono prace nad nową komendą socjalną.\n");
        return RET_POP_STATE;
    }
}
