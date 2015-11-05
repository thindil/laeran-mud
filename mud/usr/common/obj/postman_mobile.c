#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

inherit MOBILE;

static int packages;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone)
{
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
    if (clone)
        packages = 5;
}

string get_type(void) 
{
    return "postman";
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

  parts = explode(message, " ");
  switch (parts[0])
    {
    case "wezme paczke":
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

    while(sizeof(ctr) > 0) {
        if(!STRINGD->stricmp(ctr[0][0], "packages")) 
            packages = ctr[0][1];
        else 
            ret += ({ ctr[0] });
        ctr = ctr[1..];
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
    return "    ~packages{" + packages + "}\n";
}
