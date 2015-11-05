#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>
#include <phantasmal/phrase.h>

inherit MOBILE;

static int packages;
static string *recipients;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone)
{
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
    if (clone) {
        packages = 5;
        recipients = ({ });
    }
}

string get_type(void) 
{
    return "postman";
}

void set_recipients(string* new_recipients)
{
    recipients = new_recipients;
}

void set_packages(int new_packages)
{
    packages = new_packages;
}

string* get_recipients(void)
{
    return recipients;
}

int get_packages(void)
{
    return packages;
}

/* Initiate communication */
void hook_social(object body, object target, string verb)
{
    if (target && get_number() == target->get_mobile()->get_number() && verb == "pozdrow") {
        whisper(body, "Witaj, może chciałbyś zarobić nieco pieniędzy? Mamy tutaj nieco paczek\n"
                + "do rozniesienia a za mało rąk do pracy. Każdą paczkę trzeba donieść do\n"
                + "jakiejś osoby w mieście. Płacimy 2 miedziaki za każdą doręczoną paczkę.\n"
                + "Jeżeli jesteś zainteresowany, szepnij do mnie 'wezme paczke'\n");
    }
}

/* Postman iteraction */
void hook_whisper(object body, string message)
{
  string* parts;
  object new_object, recipient;
  mixed* objs;
  string items, desc;
  int number;

  switch (message)
    {
    case "wezme paczke":
        if (!packages) {
            whisper(body, "Niestety chwilowo nie mamy żadnej paczki przygotowanej do wysyłki.\n"
                    + "Proszę, wróć za parę minut.\n");
            return;
        }
        if (!sizeof(recipients)) {
            whisper(body, "Niestety na razie nie mamy odbiorców na nasze paczki, proszę wróć później.\n");
            return;
        }
        if (body->get_mobile()->get_user()->have_command("daj")) {
            whisper(body, "Jak widzę już dostałeś jedną paczkę, zanieś ją najpierw zanim będziesz\n"
                    + "mógł otrzymać następną.\n");
            return;
        }
        objs = get_body()->objects_in_container();
        new_object = clone_object(SIMPLE_ROOM); 
        MAPD->add_room_to_zone(new_object, -1, ZONED->get_zone_for_room(body->get_location()));
        new_object->add_archetype(objs[0]);
        new_object->set_brief(nil);
        body->get_location()->add_to_container(new_object);
        items = body->get_mobile()->place(new_object, body);
        if (items) {
            body->get_mobile()->get_user()->message(items + "\n");
            new_object->get_location()->remove_from_container(new_object);
            destruct_object(new_object);
        } else {
            packages--;
            body->get_mobile()->get_user()->add_command("daj", "cmd_give");
            number = (int)recipients[random(sizeof(recipients))];
            recipient = MAPD->get_room_by_num(number);
            desc = "Duża, w miarę ciężka sześcienna skrzynia owinięta papierem. Na górze widnieje napis: 'Dostarczyć\n"
                + "do " + recipient->get_brief()->to_string(body->get_mobile()->get_user()) + " w "
                + recipient->get_location()->get_brief()->to_string(body->get_mobile()->get_user()) + ".\n";
            new_object->set_look(PHRASED->new_simple_english_phrase(desc));
            TAGD->set_tag_value(new_object, "Recipient", number);
            whisper(body, "W porządku, oto Twoja paczka, zanieś ją teraz do " 
                    + recipient->get_brief()->to_string(body->get_mobile()->get_user()) + " w "
                    + recipient->get_location()->get_brief()->to_string(body->get_mobile()->get_user()) + ".\n"
                    + "Jeżeli się zgubisz, możesz zawsze sprawdzić adres odbiorcy na paczce. Kiedy dotrzesz na\n"
                    + "miejsce, 'daj paczka' odbiorcy a on już przekaże Tobie wynagrodzenie za dostawę.\n");
        }
      break;
    default:
      whisper(body, "Witaj, może chciałbyś zarobić nieco pieniędzy? Mamy tutaj nieco paczek\n"
                + "do rozniesienia a za mało rąk do pracy. Każdą paczkę trzeba donieść do\n"
                + "jakiejś osoby w mieście. Płacimy 2 miedziaki za każdą doręczoną paczkę.\n"
                + "Jeżeli jesteś zainteresowany, szepnij do mnie 'wezme paczke'\n");
      break;
    }
}

static mixed* postman_from_dtd_unq(mixed* unq) 
{
    mixed *ret, *ctr;

    ret = ({ });
    ctr = unq;

    while(sizeof(ctr[0]) > 0) {
        switch (ctr[0][0]) {
            case "packages":
                packages = (int)ctr[0][1];
                break;
            case "recipients":
                recipients = explode(ctr[0][1], ", ");
                break;
            default:
                ret += ({ ctr[0] });
                break;
        }
        ctr[0] = ctr[0][2..];
    }

    return ret;
}

void from_dtd_unq(mixed* unq) 
{
    /* Set the body, location and number fields */
    unq = mobile_from_dtd_unq(unq);
    /* Now parse the data section */
    unq = postman_from_dtd_unq(unq);
}

string mobile_unq_fields(void) 
{
    string ret;

    ret = "    ~packages{" + packages + "}\n";
    if (sizeof(recipients)) 
        ret += "    ~recipients{" + implode(recipients, ", ") + "}\n";

    return ret;
}
