#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

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
  return "townhall";
}

/* Initiate communication */
void hook_social(object body, object target, string verb)
{
    if (target && get_number() == target->get_mobile()->get_number() && verb == "pozdrow") {
        whisper(body, "Witaj, widzę, że jesteś nieco zagubiony. Oto kilka wskazówek, które mogę Ci\n"
                + "dać. Jeżeli potrzebujesz pieniędzy, płacimy tutaj w ratuszu za zabicie\n"
                + "określonych istot. Aby otrzymać wynagrodzenie za daną istotę, musisz przynieść\n"
                + "dowód do mnie. Jeżeli chcesz się dowiedzieć dokładnie jakie dowody, szepnij\n"
                + "do mnie 'lista' aby zobaczyć ich listę wraz z ofertą ile płacimy za nie.\n"
                + "Jeżeli masz przy sobie to czego potrzebujemy, szepnij do mnie 'mam <nazwa \n"
                + "przedmiotu>. Wtedy wymienimy Twoje przedmioty na gotówkę. Jeżeli potrzebujesz\n"
                + "jakiś porad, szepnij do mnie 'porady'.\n");
    }
}

/* Bounty iteraction */
void hook_whisper(object body, string message)
{
    string* parts;
    mixed* objs;
    int i, price, amount, gain;
    string items;

    parts = explode(message, " ");
    objs = get_body()->objects_in_container();
    items = "Lista poszukiwanych przez nas dowodów: \n";
    price = amount = 0;
    switch (parts[0])
    {
        case "lista":
            for (i = 0; i < sizeof(objs); i++)
                items += "- " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + ", oferujemy: " + (string)objs[i]->get_price() + " miedziaków\n";
            body->get_mobile()->get_user()->message_scroll(items);
            break;
        case "mam":
            if (sizeof(parts) < 2 || parts[1] == "") {
                whisper(body, "Jaki przedmiot konkretnie masz nam do zaoferowania?");
                return;
            }
            for (i = 0; i < sizeof(objs); i++) {
                if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " ")) {
                    price = objs[i]->get_price();
                    break;
                }
            }
            if (price == 0) {
                whisper(body, "Ale my tego nie potrzebujemy.");
                return;
            }
            objs = body->objects_in_container();
            gain = 0;
            for (i = 0; i < sizeof(objs); i++) {
                if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " ")) {
                    items = body->get_mobile()->place(objs[i], body->get_location());
                    if (!items) {
                        body->set_price(body->get_price() + price);
                        gain += price;
                        objs[i]->get_location()->remove_from_container(objs[i]);
                        destruct_object(objs[i]);
                        amount ++;
                    }
                    else
                        body->get_mobile()->get_user()->message(items + "\n");
                }
            }
            if (amount)
                body->get_mobile()->get_user()->message("Sprzedałeś " + ((amount > 1)? (string)amount + "x " : "") + implode(parts[1..], " ") + " i zarobiłeś na tym " + (string)gain + " miedziaków.\n");
            else
                whisper(body, "Widzę, że nie masz takiego przedmiotu do zaoferowania.");
            break;
        case "porady":
            whisper(body, "Proszę bardzo: sklepy, w których znajdziesz ekwipunek znajdują się na wschód\n"
                    + "od ratusza. Dokładnie na zachód od ratusza (wejście od strony placu) jest bar\n"
                    + "gdzie można spotkać innych poszukiwaczy przygód. Jeżeli pójdziesz na zachód od\n"
                    + "ratusza ulicą, trafisz na szpital. Idąc na południe trafisz do pracowni alchemika.\n");
            break;
        default:
            whisper(body, "Witaj, widzę, że jesteś nieco zagubiony. Oto kilka wskazówek, które mogę Ci\n"
                    + "dać. Jeżeli potrzebujesz pieniędzy, płacimy tutaj w ratuszu za zabicie\n"
                    + "określonych istot. Aby otrzymać wynagrodzenie za daną istotę, musisz przynieść\n"
                    + "dowód do mnie. Jeżeli chcesz się dowiedzieć dokładnie jakie dowody, szepnij\n"
                    + "do mnie 'lista' aby zobaczyć ich listę wraz z ofertą ile płacimy za nie.\n"
                    + "przedmiotu>. Wtedy wymienimy Twoje przedmioty na gotówkę. Jeżeli potrzebujesz\n"
                    + "jakiś porad, szepnij do mnie 'porady'.\n");
            break;
    }
}

void from_dtd_unq(mixed* unq) 
{
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);
}
