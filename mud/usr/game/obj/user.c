/* $Header: /cvsroot/phantasmal/mudlib/usr/game/obj/user.c,v 1.17 2005/08/02 21:20:01 angelbob Exp $ */

#include <kernel/kernel.h>
#include <kernel/user.h>
#include <kernel/rsrc.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/channel.h>
#include <phantasmal/map.h>
#include <phantasmal/search_locations.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

#include <gameconfig.h>

inherit PHANTASMAL_USER;

/* Duplicated in PHANTASMAL_USER */
#define STATE_NORMAL            0
#define STATE_LOGIN             1
#define STATE_OLDPASSWD         2
#define STATE_NEWPASSWD1        3
#define STATE_NEWPASSWD2        4

static mapping commands_map;
static object combat;
static int state_password;
static int heartbeat_handle;

/* Prototypes */
        void upgraded(varargs int clone);
static  void cmd_social(object user, string cmd, string str);
        void death(void);
        void set_health(int hp);
        void set_condition(int fatigue);
private string lalign(string num, int width);
        void set_password(string new_pass);
        void stop_follow(object user);
        void heartbeat(void);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

/* This is the mobile we'll use for the user. */
#define USER_MOBILE "/usr/common/obj/user_mobile"

int current_room;
mapping stats;
mapping skills;
string health;
string condition;
string* quests;

/*
 * NAME:	create()
 * DESCRIPTION:	initialize user object
 */
static void create(int clone)
{
  ::create(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    ::upgraded(clone);

    state_password = STATE_NORMAL;
    heartbeat_handle = -1;
    commands_map = ([
		     "mow"       : "cmd_say",
		     "emo"       : "cmd_emote",

		     "pn"         : "cmd_movement",
		     "pd"         : "cmd_movement",
		     "w"          : "cmd_movement",
		     "z"          : "cmd_movement",
		     "pdw"        : "cmd_movement",
		     "pdz"        : "cmd_movement",
		     "pnw"        : "cmd_movement",
		     "pnz"        : "cmd_movement",
		     "g"          : "cmd_movement",
		     "d"          : "cmd_movement",

		     "polnoc"           : "cmd_movement",
		     "poludnie"         : "cmd_movement",
		     "wschod"           : "cmd_movement",
		     "zachod"           : "cmd_movement",
		     "polnocnyzachod"   : "cmd_movement",
		     "poludniowyzachod" : "cmd_movement",
		     "polnocnywschod"   : "cmd_movement",
		     "poludniowywschod" : "cmd_movement",
		     "gora"             : "cmd_movement",
		     "dol"              : "cmd_movement",
		     "dosrodka"         : "cmd_movement",
		     "nazewnatrz"       : "cmd_movement",
             "podazaj"          : "cmd_follow",

		     "pomoc"     : "cmd_help",
		     "kto"       : "cmd_users",
		     "omnie"     : "cmd_whoami",
		     "bug"       : "cmd_report",
		     "literowka" : "cmd_report",
		     "idea"      : "cmd_report",
		     "powiedz"   : "cmd_tell",
		     "szepnij"   : "cmd_whisper",
		     "pp"        : "cmd_ooc",
		     "socjalne"  : "cmd_socials",
		     "komendy"   : "cmd_commands",
             "ustaw"     : "cmd_settings",
             "przygody"  : "cmd_quests",

		     "kanal"     : "cmd_channels",
		     "kanaly"    : "cmd_channels",

		     "p"         : "cmd_look",
		     "patrz"     : "cmd_look",
		     "zb"        : "cmd_look",
		     "zba"       : "cmd_look",
		     "zbadaj"    : "cmd_look",

		     "wez"       : "cmd_get",
		     "bierz"     : "cmd_get",
		     "upusc"     : "cmd_drop",
		     "i"         : "cmd_inventory",
		     "inw"       : "cmd_inventory",
		     "inwentarz" : "cmd_inventory",
		     "wsadz"     : "cmd_put",
		     "umiesc"    : "cmd_put",
		     "wyjmij"    : "cmd_remove",
		     "otworz"    : "cmd_open",
		     "zamknij"   : "cmd_close",
		     "zaloz"     : "cmd_wear",
		     "zdejmij"   : "cmd_takeoff",

		     "at"        : "cmd_attack",
		     "atak"      : "cmd_attack",
		     "atakuj"    : "cmd_attack"

    ]);

  }
}

/****************/


/*
 * Returns true if the filename isn't allowed
 */
int filename_is_forbidden(string filename) 
{
    if(previous_program() != PHANTASMAL_USER)
        error("Zły program wywołuje filename_is_forbidden!");

    return 0;
}

/*
 * Returns true if the long name isn't allowed
 */
int name_is_forbidden(string name) 
{
    if(previous_program() != PHANTASMAL_USER)
        error("Zły program wywołuje name_is_forbidden!");

    return 0;
}


/*
 * NAME:	player_login()
 * DESCRIPTION:	Create the player body, set the account info and so on...
 */
void player_login(int first_time)
{
    int    start_room_num, start_zone, rnd, i;
    object start_room, other_user;
    mixed* inv;

    if(previous_program() != PHANTASMAL_USER)
        error("Zły program wywołuje player_login!");

    hostname = query_ip_number(query_conn());
    body = nil;

    /* Set up location, body, etc */
    start_room_num = START_ROOM;
    start_room = MAPD->get_room_by_num(start_room_num);

    /* If start room can't be found, set the start room to the void */
    if (start_room == nil) {
        LOGD->write_syslog("Nie mogę znaleźć pokoju startowego! Zaczynamy w pustce...");
        start_room_num = 0;
        start_room = MAPD->get_room_by_num(start_room_num);
        start_zone = 0;
        if(start_room == nil) {
            /* Panic!  No void! */
            error("Błąd wewnętrzny: nie ma Pustki!");
        }
    } else {
        start_zone = ZONED->get_zone_for_room(start_room);
        if(start_zone < 0) {
            /* What's with this start room? */
            error("Błąd wewnętrzny: nie ma jakiejkolwiek strefy dla pokoju startowego!");
        }
    }

    if(body_num > 0) 
        body = MAPD->get_room_by_num(body_num);

    if(body && body->get_mobile()
            && body->get_mobile()->get_user()) {
        other_user = body->get_mobile()->get_user();
    }
    if(other_user && other_user->get_name() != name) {
        LOGD->write_syslog("Jest już ustawiony użytkownik dla tego mobka!", LOG_ERROR);
        message("UWAGA: Zła konfiguracja plików z ciałem oraz mobkiem! Wewnętrzny błąd!\n");

        other_user->message("Ktoś zalogował się na Twoje konto!\n");
        other_user->message("Zamykamy Twoje połączenie...\n");
        destruct_object(other_user);
    }

    /* Lets do character stats */
    if (stats == nil)
        stats = ([ "siła": ({10, 0}), "zręczność": ({10, 0}), "inteligencja": ({10, 0}), "kondycja": ({10, 0}) ]);
    if (skills == nil)
        skills = ([ ]);
    if (!health || health == "")
        health = "Zdrowy";
    if (!condition || condition == "")
        condition = "Wypoczęty";
    if (quests == nil)
        quests = ({ });

    if(!body) {
        location = start_room;
        current_room = start_room_num;

        body = clone_object(SIMPLE_ROOM);
        if(!body)
            error("Nie mogę sklonować ciała gracza!");

        body->set_container(1);
        body->set_open(1);
        body->set_openable(0);

        /* Players weigh about 80 kilograms */
        rnd = random(11);
        if (random(11) < 5)
            rnd = rnd * (-1);
        if (gender == 1)
            body->set_weight(70.0 + (float)rnd);
        else
            body->set_weight(55.0 + (float)rnd);
        rnd = random(11);
        if (random(11) < 5)
            rnd = rnd * (-1);
        if (gender == 1)
            rnd = 170 + rnd;
        else
            rnd = 160 + rnd;
        body->set_length((float)rnd);
        rnd = rnd / 10;
        body->set_volume((2.5 * (float)rnd));

        /* Players are able to lift 50 kilograms */
        body->set_weight_capacity(50.0);
        /* Players are able to carry up to 20 liters of stuff --
           that's roughly a large hiking backpack. */
        body->set_volume_capacity(20.0);
        /* Players are able to take 3m long items */
        body->set_length_capacity(300.0);
        body->set_hp(10);

        MAPD->add_room_to_zone(body, -1, start_zone);
        if(!MAPD->get_room_by_num(body->get_number())) {
            LOGD->write_syslog("Błąd przy tworzeniu nowego ciała!", LOG_ERR);
        }
        body_num = body->get_number();

        /* Set descriptions and add noun for new name */
        body->set_brief(NEW_PHRASE(Name));
        body->set_look(NEW_PHRASE(Name + " stoi tutaj."));
        body->set_examine(nil);
        body->add_noun(NEW_PHRASE(STRINGD->to_lower(Name)));

        /* Can't just clone mobile here, it causes problems later */
        mobile = MOBILED->clone_mobile_by_type("user");
        if(!mobile)
            error("Nie mogę sklonować mobka typu 'user'!");
        MOBILED->add_mobile_number(mobile, -1);
        mobile->assign_body(body);
        mobile->set_user(this_object());

        mobile->teleport(location, 1);

        /* We just set a body number, so we need to save the player data
           file again... */
        save_user_to_file();
    } else {
        location = MAPD->get_room_by_num(current_room);
        mobile = body->get_mobile();
        if(!mobile) {
            mobile = clone_object(USER_MOBILE);
            MOBILED->add_mobile_number(mobile, -1);
            mobile->assign_body(body);
        }

        mobile->set_user(this_object());
        mobile->teleport(location, 1);

        /* Move body to start room */
        if(location->get_number() <= LOCKER_ROOM)
            mobile->teleport(start_room, 1);

        /* Recound player current weight and volume */
        inv = body->objects_in_container();
        for (i = 0; i < sizeof(inv); i++) {
            body->remove_from_container(inv[i]);
            body->append_to_container(inv[i]);
        }
    }

    /* Show room to player */
    message("\n");
    show_room_to_player(location);
    heartbeat_handle = call_out("heartbeat", 10);
}


/*
 * NAME:	player_logout()
 * DESCRIPTION:	Deal with player body, update account info and so on...
 */
static void player_logout(void)
{
    if(previous_program() != PHANTASMAL_USER)
        error("Zły program wywołuje player_logout!");

    /* Teleport body to meat locker */
    if(body) {      
        object meat_locker;
        object mobile;
        mixed *objs;
        int i;

        /* remove packages from player on exit */
        objs = body->objects_in_container();
        for (i = 0; i < sizeof(objs); i++) {
            if (objs[i]->get_brief()->to_string(this_object()) == "paczka") {
                body->remove_from_container(objs[i]);
                destruct_object(objs[i]);
            }
        }

        if (TAGD->get_tag_value(body, "Combat")) {
            combat->stop_combat();
            destruct_object(combat);
            death();
        }

        if (TAGD->get_tag_value(body, "Follow")) 
            stop_follow(this_object());

        if(LOCKER_ROOM >= 0) {
            meat_locker = MAPD->get_room_by_num(LOCKER_ROOM);
            if(meat_locker) {
                if (location) {
                    current_room = location->get_number();
                    mobile = body->get_mobile();
                    mobile->teleport(meat_locker, 1);
                }
            } else
                LOGD->write_syslog("Nie mogę znaleźć pokoju #" + LOCKER_ROOM + " jako przechowalnia ciał!", LOG_ERR);
        }
    }
    save_user_to_file();
    CHANNELD->unsubscribe_user_from_all(this_object());

    if (heartbeat_handle > -1)
        remove_call_out(heartbeat_handle);

    message("\nDo zobaczenia!\n");
}


int process_command(string str)
{
    string cmd;
    int    ctr, size;
    int    force_command;
    mixed* command;

    cmd = str;
    if (strlen(str) != 0 && str[0] == '!') {
        cmd = cmd[1 ..];
        force_command = 1;
    }

    /* Do this unless we're in the editor and didn't start the command
       with an exclamation mark */
    if (!wiztool || !query_editor(wiztool) || force_command) {
        /* check standard commands */
        cmd = STRINGD->trim_whitespace(cmd);
        if(cmd == "") 
            str = nil;

        if (strlen(cmd) != 0) {
            switch (cmd[0]) {
                case '\'':
                    if (strlen(cmd) > 1) {
                        str = cmd[1..];
                    } else {
                        str = "";
                    }
                    cmd = "mow";
                    break;

                case ':':
                    if (strlen(cmd) > 1) {
                        str = cmd[1..];
                    } else {
                        str = "";
                    }
                    cmd = "emo";
                    break;

                default:
                    /* If single word, leave cmd the same.  If multiword, put
                       first word in cmd. */
                if(sscanf(cmd, "%s %s", cmd, str) != 2) {
                    str = "";
                }
                break;
            }
        }
        
        /* Set new password */
        if (cmd == "ustaw" && str == "haslo") {
            message("Podaj stare hasło: ");
            state_password = STATE_OLDPASSWD;
            return MODE_NOECHO;
        }
        if (state_password == STATE_OLDPASSWD) {
            if (hash_string("crypt", cmd + str, password) != password) {
                message("Nieprawidłowe stare hasło, przerywam ustawianie nowego hasła.\n");
                state_password = STATE_NORMAL;
                return MODE_ECHO;
            }
            else {
                message("Podaj nowe hasło: ");
                state_password = STATE_NEWPASSWD1;
                return MODE_NOECHO;
            }
        }
        if (state_password == STATE_NEWPASSWD1) {
            if (!str) 
                message("Puste nowe hasło. Przerywam zmianę hasła.\n");
            else {
                set_password(cmd + str);
                message("Ustawiono nowe hasło.\n");
            }
            state_password = STATE_NORMAL;
            return MODE_ECHO;
        }

        /* Log out from game */
        if(cmd == "wyloguj")
        {
            return MODE_DISCONNECT;
        }

        if(SOULD->is_social_verb(cmd)) {
            cmd_social(this_object(), cmd, str);
            str = nil;
            return -1;
        }

        if(commands_map[cmd]) {
            string err;

            if (TAGD->get_tag_value(body->get_mobile(), "Logged"))
                LOGD->write_syslog("Komenda: " + cmd + " opcje: |" + str + "|");
            err = (call_other(this_object(),                /* Call on self */
                        commands_map[cmd],            /* The function */
                        this_object(),                /* This user */
                        cmd,                          /* The command */
                        str == "" ? nil : str)        /* str or nil */
                  );
            if(err) {
                LOGD->write_syslog("Błąd w komendzie '" + cmd + "/"
                        + (str ? str : "(nil)") + "'. Tekst błędu: "
                        + err);

                message("Twoja komenda się wykraczyła w kodzie.\nKrzycz na Thindila.\n");

                /* Return normal status, print a prompt and continue. */
                return -1;
            }
            str = nil;
        }
    }

    if (str) {
        if (wiztool) {
            string err;

            err = catch(wiztool->command(cmd, str));
            if(err) {
                LOGD->write_syslog("Błąd komendzie '" + cmd + "/"
                        + (str ? str : "(nil)") + "'. Tekst błędu: "
                        + err);

                message("Twoja komenda się wykraczyła w kodzie.\nKrzycz na Thindila.\n");

                /* Return normal status, print a prompt and continue. */
                return -1;
            }
        } else
            message("Nie ma takiej komendy: " + cmd + " " + str + "\n");
    }

    /* All is well, just print a prompt and wait for next command */
    return -1;
}

int get_stat_val(string name)
{
    return stats[name][0];
}

int get_stat_exp(string name)
{
    return stats[name][1];
}

int get_skill_val(string name)
{
    if (!skills[name])
        return 1;
    return skills[name][0];
}

int get_skill_exp(string name)
{
    if (!skills[name])
        return 0;
    return skills[name][1];
}

int have_skill(string name)
{
    if (sizeof(map_indices(skills) & ({ name })))
        return 1;
    else
        return 0;
}

void death()
{
    object PHRASE phr;
    object new_body;
    string conj_name;
    string *nouns;

    message("GINIESZ.\n");

    current_room = HOSPITAL_ROOM;
    if (!MAPD->get_room_by_num(current_room)) 
        current_room = START_ROOM;
    new_body = clone_object(SIMPLE_ROOM);
    new_body->set_brief(body->get_brief());
    new_body->set_look(body->get_look());
    new_body->set_container(1);
    new_body->set_open(1);
    new_body->set_openable(0);
    new_body->set_weight(body->get_weight());
    new_body->set_length(body->get_length());
    new_body->set_volume(body->get_volume());
    new_body->set_weight_capacity(body->get_weight_capacity());
    new_body->set_volume_capacity(20.0);
    new_body->set_length_capacity(300.0);
    new_body->set_hp(10);
    phr = PHRASED->new_simple_english_phrase(implode(body->get_nouns(locale), ", ")); 
    new_body->add_noun(phr);
    new_body->set_price(body->get_price());
    MAPD->add_room_to_zone(new_body, -1, ZONED->get_zone_for_room(location));
    mobile->assign_body(new_body);
    body_num = new_body->get_number();

    nouns = body->get_nouns(locale);
    if (sizeof(nouns) < 7)
        conj_name = Name;
    else
        conj_name = nouns[3];
    phr = PHRASED->new_simple_english_phrase("zwłoki " + conj_name);
    body->set_brief(phr);
    phr = PHRASED->new_simple_english_phrase("zwłoki " + conj_name + " leżą tutaj.");
    body->set_look(phr);
    body->clear_nouns();
    phr = PHRASED->new_simple_english_phrase("zwłoki, " + conj_name + ", zwloki");
    body->add_noun(phr);
    TAGD->set_tag_value(body, "DropTime", time() + 3600);
    body = new_body;
    location = MAPD->get_room_by_num(current_room);
    mobile->teleport(location, 1);
    set_health(10);
    set_condition(stats["kondycja"][0] * 10);
    show_room_to_player(location);
}

void gain_exp(string skill, int value)
{
    int exp, needexp, level;

    /* gain experience in stats */
    if (sizeof(map_indices(stats) & ({ skill })))
    {
        exp = stats[skill][1] + value;
        needexp = stats[skill][0] * 1000;
        if (exp > needexp)
        {
            exp -= needexp;
            stats[skill][0] ++;
            if (skill == "siła" && stats[skill][0] < 51)
            {
                body->set_weight_capacity(50.0 + (float)stats[skill][0]);
            }
        }
        stats[skill][1] = exp;
        if (stats[skill][0] > 50)
        {
            stats[skill][0] = 50;
            stats[skill][1] = 0;
        }
        return;
    }

    if (have_skill(skill)) {
        exp = skills[skill][1] + value;
        needexp = skills[skill][0] * 100;
        level = skills[skill][0];
    } else {
        exp = value;
        needexp = 100;
        level = 1;
    }
    if (exp > needexp) {
        exp -= needexp;
        level ++;
    }
    if (level > 100) {
        level = 100;
        exp = 0;
    }
    skills[skill] = ({ level, exp });
}

/* Set string value of health for prompt */
void set_health(int hp)
{
  float percent;
  
  percent = (float)((float)hp / (float)body->get_hp());
  if (percent == 1.0)
    {
      health = "Zdrowy";
    }
  else if (percent < 1.0 && percent >= 0.8)
    {
      health = "Poturbowany";
    }
  else if (percent < 0.8 && percent >= 0.6)
    {
      health = "Lekko ranny";
    }
  else if (percent < 0.6 && percent >= 0.4)
    {
      health = "Ranny";
    }
  else if (percent < 0.4 && percent >= 0.2)
    {
      health = "Ciężko ranny";
    }
  else
    {
      health = "Na skraju śmierci";
    }
}

/* Set string value of condition for prompt */
void set_condition(int fatigue)
{
    float percent;

    percent = (float)((float)fatigue / (float)(stats["kondycja"][0] * 10));
    if (percent == 1.0)
        condition = "Wypoczęty";
    else if (percent < 1.0 && percent >= 0.8)
        condition = "Nieco zmęczony";
    else if (percent < 0.8 && percent >= 0.6)
        condition = "Zmęczony";
    else if (percent < 0.6 && percent >= 0.4)
        condition = "Bardzo zmęczony";
    else if (percent < 0.4 && percent >= 0.2)
        condition = "Wykończony";
    else
        condition = "Padnięty";
}

/* Show prompt to user */
void print_prompt(void)
{
  string str;

  str = (wiztool) ? query_editor(wiztool) : nil;
  if (str)
    {
      message(":");
    }
  else
    {
      message("[" + health + "] [" + condition + "]\n");
    }
}

void heartbeat(void)
{
    int cur_val;

    if (TAGD->get_tag_value(body, "Combat")) {
        call_out("heartbeat", 10);
        return;
    }

    if (TAGD->get_tag_value(body, "Hp")) {
        cur_val = TAGD->get_tag_value(body, "Hp");
        cur_val += 3;
        if (cur_val >= body->get_hp()) {
            cur_val = body->get_hp();
            TAGD->set_tag_value(body, "Hp", nil);
            message("Jesteś już kompletnie zdrowy.\n");
        }
        else
            TAGD->set_tag_value(body, "Hp", cur_val);
        set_health(cur_val);
    }
    
    if (TAGD->get_tag_value(body, "Fatigue")) {
        cur_val = TAGD->get_tag_value(body, "Fatigue");
        cur_val -= ((stats["kondycja"][0] * 10) / 3);
        if (cur_val <= 0) {
            cur_val = 0;
            TAGD->set_tag_value(body, "Fatigue", nil);
            message("Jesteś już kompletnie wypoczęty.\n");
        }
        else
            TAGD->set_tag_value(body, "Fatigue", cur_val);
        set_condition((stats["kondycja"][0] * 10) -  cur_val);
    }

    call_out("heartbeat", 10);
}

void set_password(string new_pass)
{
    password = hash_string("crypt", new_pass);
}

void add_command(string cmd, string func)
{
    commands_map[cmd] = func; 
}

int have_command(string cmd)
{
    if (commands_map[cmd])
        return 1;
    else
        return 0;
}

void add_quest(string name)
{
    quests += ({ name });
}

string* get_quests(void)
{
    return quests;
}
/************** User-level commands *************************/

static void cmd_ooc(object user, string cmd, string str) {
  if (!str || str == "") {
    message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  CHANNELD->chat_to_channel(CHANNEL_OOC, str);
}

static void cmd_say(object user, string cmd, string str) {
  if(!str || str == "") {
    message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->say(str);
}

static void cmd_emote(object user, string cmd, string str) {
  if(!str || str == "") {
    message("Użycie: " + cmd + " <tekst>\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->emote(str);
}

static void cmd_tell(object self, string cmd, string str) {
  object user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2 ||
      !(user=::find_user(username))) {
    message("Użycie: " + cmd + " <imię> <tekst>\n");
  } else {
    user->message(Name + " mówi Tobie: " + str + "\n");
  }
}

/* Whisper to someone */
static void cmd_whisper(object self, string cmd, string str)
{
  object *user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2)
    {
      message("Użycie: " + cmd + " <imię> <tekst>\n");
      return;
    }
  user = find_first_objects(username, LOC_CURRENT_ROOM);
  if (!user)
    {
      message("Nie ma kogoś o imieniu " + username + " w okolicy.\n");
    }
  else
    {
      mobile->whisper(user[0], str);
    }
}

/* Show info about character */
static void cmd_whoami(object user, string cmd, string str)
{
    string charinfo, skilltext, tmptext;
    mapping statsinfo, skillsinfo;
    int i, j;
    string *stattext, *statnames, *skillname, *skillsnames;

    charinfo = "Nazywasz się " + Name + ". ";
    if (gender == 1)
        charinfo += "Jesteś mężczyzną. ";
    else
        charinfo += "Jesteś kobietą. ";
    charinfo += "Masz " + body->get_length() + " cm wzrostu. "
        + "Ważysz " + body->get_weight() + " kg.\n"
        + "Jesteś " + STRINGD->to_lower(health) + " i " + STRINGD->to_lower(condition) + ".\n"
        + "Niesiesz " + body->get_current_weight() + " kg ekwipunku na " + body->get_weight_capacity() + " kg możliwych.\n\n"
        + "===== CECHY =====";
    statsinfo = ([ "siła": ({ "Słaby", "Przeciętny", "Dobrze zbudowany", "Siłacz" }),
            "zręczność": ({ "Niezdarny", "Przeciętny", "Zwinny", "Akrobata" }),
            "inteligencja": ({ "Niezbyt rozgarnięty", "Przeciętny", "Inteligentny", "Mistrz Umysłu" }),
            "kondycja": ({ "Chorowity", "Przeciętny", "Wysportowany", "Niezniszczalny" }) ]);
    stattext = ({ "Siła: ", "Zręczność: ", "Inteligencja: ", "Kondycja: " });
    statnames = ({ "siła", "zręczność", "inteligencja", "kondycja" });
    for (i = 0; i < sizeof(statnames); i++) {
        if (!(i % 2))
            charinfo += "\n";
        else
            charinfo += "      ";
        tmptext = stattext[i];
        if (stats[statnames[i]][0] >= 10 && stats[statnames[i]][0] < 20)
            tmptext += statsinfo[statnames[i]][0];
        else if (stats[statnames[i]][0] >= 20 && stats[statnames[i]][0] < 30)
            tmptext += statsinfo[statnames[i]][1];
        else if (stats[statnames[i]][0] >= 30 && stats[statnames[i]][0] < 40)
            tmptext += statsinfo[statnames[i]][2];
        else
            tmptext += statsinfo[statnames[i]][3];
        charinfo += lalign(tmptext, 40);
    }
    charinfo += "\n\n===== UMIEJĘTNOŚCI =====";
    skillsinfo = ([ ]);
    skillsnames = map_indices(skills);
    for (i = 0; i < sizeof(skillsnames); i++) {
        skillname = explode(skillsnames[i], "/");
        if (skills[skillsnames[i]][0] >= 1 && skills[skillsnames[i]][0] < 20)
            skilltext = "nowicjusz";
        else if (skills[skillsnames[i]][0] >= 20 && skills[skillsnames[i]][0] < 40)
            skilltext = "amator";
        else if (skills[skillsnames[i]][0] >= 40 && skills[skillsnames[i]][0] < 60)
            skilltext = "adept";
        else if (skills[skillsnames[i]][0] >= 60 && skills[skillsnames[i]][0] < 80)
            skilltext = "zaawansowany";
        else
            skilltext = "mistrz";
        if (!skillsinfo[skillname[0]])
            skillsinfo[skillname[0]] = ({ });
        skillsinfo[skillname[0]] += ({ skillname[1], skilltext });
    }
    skillsnames = map_indices(skillsinfo);
    for (i = 0; i < sizeof(skillsnames); i++) {
        charinfo += "\n                     ==== " + skillsnames[i] + " ====";
        for (j = 0; j < sizeof(skillsinfo[skillsnames[i]]); j += 2) {
            if (!(j % 4))
                charinfo += "\n";
            charinfo += lalign(skillsinfo[skillsnames[i]][j] + ": " + skillsinfo[skillsnames[i]][j + 1], 40);
        }
    }
    charinfo += "\n";
    message_scroll(charinfo);
}

static void cmd_look(object user, string cmd, string str) {
  object* tmp, *objs;
  int     ctr;

  str = STRINGD->trim_whitespace(str);

  if(!location) {
    user->message("Jesteś w pustce!\n");
    return;
  }

  if(!str || str == "") {
    show_room_to_player(location);
    return;
  }

  if (cmd[0] != 'z') {
    /* trim an initial "at" off the front of the command if the verb
       was "look" and not "examine". */
    sscanf(str, "na %s", str);
  }

  if(sscanf(str, "w %s", str) || sscanf(str, "do srodka %s", str)
     || sscanf(str, "do %s", str) || sscanf(str, "pomiedzy %s", str)) {
    /* Look inside container */
    str = STRINGD->trim_whitespace(str);
    tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
    if(!tmp) {
      user->message("Nie znalazłeś jakiegokolwiek '" + str + "'.\n");
      return;
    }
    if(sizeof(tmp) > 1) {
      user->message("Widzisz więcej niż jeden '" + str +"'. Musisz wybrać który.\n");
    }

    if(!tmp[0]->is_container()) {
      user->message("To nie jest pojemnik.\n");
      return;
    }

    if(!tmp[0]->is_open()) {
      user->message("Jest zamknięte.\n");
      return;
    }

    if ((!tmp[0]->get_mobile()) || (user->is_admin()))
      {
	objs = tmp[0]->objects_in_container();
      }
    else
      {
	objs = nil;
      }
    if(objs && sizeof(objs)) {
      for(ctr = 0; ctr < sizeof(objs); ctr++) {
        user->message("- ");
        user->send_phrase(objs[ctr]->get_brief());
        user->message("\n");
      }
    user->message("-----\n");
    } else {
      user->message("Nie ma nic w ");
      user->send_phrase(tmp[0]->get_brief());
      user->message(".\n");
    }
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    user->message("Nie widzisz żadnego '" + str + "'.\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    user->message("Więcej niż jeden taki jest tutaj. "
		  + "Sprawdziłeś pierwszy.\n\n");
  }

  if(cmd[0] == 'z' && tmp[0]->get_examine()) {
    user->send_phrase(tmp[0]->get_examine());
  } else {
    user->send_phrase(tmp[0]->get_look());
  }
  user->message("\n");
}

/*
 * NAME:	lalign()
 * DESCRIPTION:	return a string as a left-aligned string
 */
private string lalign(string num, int width)
{
    string str;
    int i, diff;

    str = num + "                         ";
    if (width > strlen(str)) {
        diff = width - strlen(str);
        for (i = 0; i <= diff; i++)
            str += " ";
    }
    return str[0..width];
}

static void cmd_inventory(object user, string cmd, string str) {
  int    ctr, size;
  mixed* objs;
  string *inv, *weared;
  string msg;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  objs = body->objects_in_container();
  if(!objs || !sizeof(objs)) {
    user->message("Nic nie nosisz przy sobie.\n");
    return;
  }
  inv = weared = ({ });
  for(ctr = 0; ctr < sizeof(objs); ctr++) {
      if (objs[ctr]->is_dressed())
          weared += ({ lalign("- " + objs[ctr]->get_brief()->to_string(user), 25) });
      else
          inv += ({ lalign("- " + objs[ctr]->get_brief()->to_string(user), 25) });
  }
  if (sizeof(weared) > sizeof(inv))
      size = sizeof(weared);
  else
      size = sizeof(inv);
  msg = lalign("=Inwentarz=", 25) + lalign("=Założone=", 25) + "\n";
  for (ctr = 0; ctr < size; ctr ++) {
      if (ctr < sizeof(inv))
          msg += inv[ctr];
      else
          msg += lalign("            ", 25);
      if (ctr < sizeof(weared))
          msg += weared[ctr] + "\n";
      else
          msg += "\n";
  }
  msg += "\nPosiadasz " + body->get_price() + " miedziaków.\n"
    + "Niesiesz " + body->get_current_weight() + " kg ekwipunku na " + body->get_weight_capacity() + " kg możliwych.\n\n";
  message_scroll(msg);
}

static void cmd_put(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr, portindex, contindex;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <obiekt1> w <obiekt2>\n");
    return;
  }

  if(sscanf(str, "%s do %s", obj1, obj2) != 2
     && sscanf(str, "%s w %s", obj1, obj2) != 2) {
    user->message("Użycie: " + cmd + " <obiekt1> w <obiekt2>\n");
    return;
  }

  if (sscanf(obj1, "%d %s", portindex, obj1) != 2)
    {
      portindex = 0;
    }
  portindex --;
  if (portindex < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  if (sscanf(obj2, "%d %s", contindex, obj2) != 2)
    {
      contindex = 0;
    }
  contindex --;
  if (contindex < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  portlist = find_first_objects(obj1, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj1 + "' w okolicy.\n");
    return;
  }
  if (portindex >= sizeof(portlist))
    {
      message("Nie ma aż tyle '" + obj1 + "' w okolicy.\n");
      return;
    }

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if (!is_admin())
    {
      ctr = 0;
      while (ctr < sizeof(contlist))
	{
	  if (contlist[0]->get_mobile())
	    {
	      contlist = contlist[0..(ctr - 1)] + contlist[(ctr + 1)..];
	    }
	  ctr++;
	}
    }
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć żadnego '" + obj2 + "' w okolicy.\n");
    return;
  }
  if (contindex >= sizeof(contlist))
    {
      message("Nie ma aż tyle '" + obj2 + "' w okolicy.\n");
      return;
    }

  if(sizeof(portlist) > 1)
    {
      if (portindex == -1)
	{
	  message("Jest więcej niż jeden obiekt pasujący do '" + obj1 + "'.\n");
	  portindex = 0;
	}
      message("Wybrałeś " + portlist[portindex]->get_brief()->to_string(user) + ".\n");
  }
  if (portindex == -1)
    {
      portindex = 0;
    }

  if(sizeof(contlist) > 1)
    {
      if (contindex == -1)
	{
	  message("Jest więcej niż jeden obiekt pasujący do '" + obj2 + "'.\n");
	  contindex = 0;
	}
      message("Wybrałeś " + contlist[contindex]->get_brief()->to_string(user) + ".\n");
  }
  if (contindex == -1)
    {
      contindex = 0;
    }

  port = portlist[portindex];
  cont = contlist[contindex];

  if (port->is_dressed())
    {
      port->set_dressed(0);
      message("Zdejmujesz " + port->get_brief()->to_string(user) + ".\n");
    }

  if (!(err = mobile->place(port, cont))) {
    user->message("Wkładasz ");
    user->send_phrase(port->get_brief());
    user->message(" do ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }

}

static void cmd_remove(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr, portindex, contindex;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Użycie: " + cmd + " <obiekt1> z <obiekt2>\n");
    return;
  }

  if(sscanf(str, "%s ze srodka %s", obj1, obj2) != 2
     && sscanf(str, "%s z wnetrza %s", obj1, obj2) != 2
     && sscanf(str, "%s ze %s", obj1, obj2) != 2
     && sscanf(str, "%s z %s", obj1, obj2) != 2) {
    user->message("Użycie: " + cmd + " <obiekt1> z <obiekt2>\n");
    return;
  }

  if (sscanf(obj1, "%d %s", portindex, obj1) != 2)
    {
      portindex = 0;
    }
  portindex --;
  if (portindex < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  if (sscanf(obj2, "%d %s", contindex, obj2) != 2)
    {
      contindex = 0;
    }
  contindex --;
  if (contindex < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj2 + "' w okolicy.\n");
    return;
  }
  if (contindex >= sizeof(contlist))
    {
      message("Nie ma aż tyle '" + obj2 + "' w okolicy.\n");
      return;
    }

  if(sizeof(contlist) > 1)
    {
      if (contindex == -1)
	{
	  message("Jest więcej niż jeden obiekt pasujący do '" + obj2 + "'.\n");
	  contindex = 0;
	}
      message("Wybrałeś " + contlist[contindex]->get_brief()->to_string(user) + ".\n");
    }
  if (contindex == -1)
    {
      contindex = 0;
    }
  cont = contlist[contindex];

  portlist = cont->find_contained_objects(user, obj1);
  if(!portlist || !sizeof(portlist)) {
    user->message("Nie możesz znaleźć jakiegokolwiek '" + obj1 + "' w ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
    return;
  }
  if (portindex >= sizeof(portlist))
    {
      message("Nie ma aż tyle '" + obj1 + "' w okolicy.\n");
      return;
    }

  if(sizeof(portlist) > 1)
    {
      if (portindex == -1)
	{
	  message("Jest więcej niż jeden obiekt pasujący do '" + obj1 + "'.\n");
	  portindex = 0;
	}
      message("Wybrałeś " + portlist[portindex]->get_brief()->to_string(user) + ".\n");
  }
  if (portindex == -1)
    {
      portindex = 0;
    }
  port = portlist[portindex];

  if (!(err = mobile->place(port, body))) {
    user->message("Wziąłeś ");
    user->send_phrase(port->get_brief());
    user->message(" z ");
    user->send_phrase(cont->get_brief());
    user->message(".\n");
  } else {
    user->message(err + "\n");
  }
}


static void cmd_users(object user, string cmd, string str) {
    int i, sz;
    object* users;
    string name_idx;

    users = users();
    message("Zalogowani: \n");
    str = "";
    for (i = 0, sz = sizeof(users); i < sz; i++) {
        name_idx = "* " + users[i]->query_name();
        if (name_idx) {
            if (users[i]->is_admin())
                name_idx += " (Opiekun)";
            str += lalign(name_idx, 25) + lalign("Bezczynny(a): " + users[i]->get_idle_time() + " sekund", 25) + "\n";
        }
    }
    message(str + "\n");
}

/* List available social commands */
static void cmd_socials(object user, string cmd, string str)
{
    int i, sz;
    string* scommands;

    message("Dostępne komendy socjalne:\n");
    scommands = SOULD->all_socials();
    str = "";
    for (i = 0, sz = sizeof(scommands); i < sz; i++) {
        if (!(i % 5))
            str += "\n";
        str += lalign(scommands[i], 20);
    }
    message_scroll(str + "\n");
}

/* Stop following */
void stop_follow(object user)
{
    object target;

    target = MAPD->get_room_by_num(TAGD->get_tag_value(body, "Follow"));
    TAGD->set_tag_value(body, "Follow", nil);
    if (target->get_mobile()->get_user())
        target->get_mobile()->get_user()->message(Name + " przestał za Tobą podążać.\n");
    message("Przestałeś podążać za " + target->get_brief()->to_string(user) + ".\n");
}

static void cmd_movement(object user, string cmd, string str) {
    int    dir, fatigue, exp;
    string reason;
    float capacity;

    if (TAGD->get_tag_value(body, "Fatigue"))
        fatigue = TAGD->get_tag_value(body, "Fatigue");
    else
        fatigue = 0;

    /* Currently, we ignore modifiers (str) and just move */

    dir = EXITD->direction_by_string(cmd);
    if(dir == -1) {
        user->message("'" + cmd + "' nie wygląda na poprawny kierunek.\n");
        return;
    }

    if (reason = mobile->move(dir)) {
        user->message(reason + "\n");
        return;
    }

    capacity = body->get_current_weight() / body->get_weight_capacity();
    if (capacity <= 0.5) {
        fatigue++;
        exp = 1;
    } else if(capacity > 0.5 && capacity <= 0.75) {
        fatigue += 2;
        exp = 2;
    } else if(capacity > 0.75 && capacity <= 0.95) {
        fatigue += 5;
        exp =5;
    } else {
        fatigue += 10;
        exp = 10;
    }
    if (fatigue > (stats["kondycja"][0] * 10))
        fatigue = stats["kondycja"][0] * 10;
    TAGD->set_tag_value(body, "Fatigue", fatigue);
    gain_exp("kondycja", exp);
    set_condition((stats["kondycja"][0] * 10) - fatigue);

    if (TAGD->get_tag_value(body, "Combat")) {
        combat->stop_combat();
        destruct_object(combat);
        message("Uciekasz z walki.\n");
    }
    if (TAGD->get_tag_value(body, "Follow"))
        stop_follow(user);

    show_room_to_player(location);
}

/* This one is special, and is called specially... */
static void cmd_social(object user, string cmd, string str) 
{
    object* targets;
    int index;

    if(!SOULD->is_social_verb(cmd)) {
        message(cmd + " nie wygląda na poprawną komendę socjalną.\n");
        return;
    }
    
    if (!str || STRINGD->trim_whitespace(str) == "") {
        mobile->social(cmd, nil);
        return;
    }

    if (sscanf(str, "%d %s", index, str) != 2)
        index = 0;
    index --;
    if (index < -1) {
        message("W marzeniach.\n");
        return;
    }
    targets = location->find_contained_objects(user, str);
    if(!targets) {
        message("Nie możesz znaleźć żadnego '" + str + "' w okolicy.\n");
        return;
    }
    if (index >= sizeof(targets)) {
        message("Nie ma aż tyle '" + str + "' w okolicy.\n");
        return;
    }
    if (sizeof(targets) > 1) {
        if (index == -1) {
            message("Więcej niż jedna taka osoba jest w okolicy.\n");
            index = 0;
        }
        message("Wybierasz " + targets[index]->get_brief()->to_string(user) + "\n");

    }
    if (index == -1)
        index = 0;
    mobile->social(cmd, targets[index]);
}

static void cmd_get(object user, string cmd, string str) {
  object* tmp;
  string err;
  int index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " <opis w pomocy>\n");
    return;
  }

  if(sscanf(str, "%*s z wnetrza %*s") == 2
     || sscanf(str, "%*s ze srodka %*s") == 2
     || sscanf(str, "%*s z %*s") == 2
     || sscanf(str, "%*s ze %*s") == 2) {
    cmd_remove(user, cmd, str);
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }

  if(sizeof(tmp) > 1)
    {
      if (index == -1)
	{
	  message("Więcej niż jedna z tych rzeczy leży w okolicy.\n");
	  index = 0;
	}
    message("Wybrałeś "+ tmp[index]->get_brief()->to_string(user) + ".\n");
  }
  if (index == -1)
    {
      index = 0;
    }

  if(tmp[index] == location) {
    message("Nie możesz tego zabrać. Znajdujesz się wewnątrz tego.\n");
    return;
  }

  if(tmp[index]->get_detail_of()) {
    message("Nie możesz tego wziąć. Jest częścią ");
    send_phrase(tmp[index]->get_detail_of()->get_brief());
    message(".\n");
    return;
  }

  if(!(err = mobile->place(tmp[index], body)))
    {
      message("Bierzesz " + tmp[index]->get_brief()->to_string(user) + ".\n");
      TAGD->set_tag_value(tmp[index], "DropTime", nil);
    }
  else
    {
      message(err + "\n");
    }
}

static void cmd_drop(object user, string cmd, string str)
{
  object* tmp;
  string err;
  int index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  tmp = find_first_objects(str, LOC_INVENTORY, LOC_BODY);
  if(!tmp || !sizeof(tmp)) {
    message("Nie niesiesz ze sobą '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      message("Nie masz aż tyle '" + str + "' ze sobą.\n");
      return;
    }

  if(sizeof(tmp) > 1)
    {
      if (index == -1)
	{
	  message("Masz więcej niż jedną taką rzecz.\n");
	  index = 0;
	}
      message("Wybierasz " + tmp[index]->get_brief()->to_string(user) + ".\n");
    }
  if (index == -1)
    {
      index = 0;
    }

  if (tmp[index]->is_dressed())
    {
      tmp[index]->set_dressed(0);
      message("Zdejmujesz " + tmp[index]->get_brief()->to_string(user) + ".\n");
    }

  if (!(err = mobile->place(tmp[index], location)))
    {
      message("Upuszczasz " + tmp[index]->get_brief()->to_string(user) + ".\n");
      TAGD->set_tag_value(tmp[index], "DropTime", time());
    }
  else
    {
      message(err + "\n");
    }
}

static void cmd_open(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr, index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      message("W marzeniach.\n");
      return;
    }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }

  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if(tmp[ctr]->is_openable())
	    break;
	}
      if(ctr >= sizeof(tmp))
	{
	  message("Żadne z wybranych nie może być otwarte.\n");
	  return;
	}
      if (index == -1)
	{
	  message("Więcej niż jedno takie jest w okolicy.\n");
	}
      else
	{
	  ctr = index;
	}
      message("Wybierasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }

  if(!tmp[ctr]->is_openable()) {
    message("Nie możesz tego otworzyć!\n");
    return;
  }

  if(!(err = mobile->open(tmp[ctr]))) {
    message("Otwierasz ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

static void cmd_close(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr, index;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Użycie: " + cmd + " [numer] <obiekt>\n");
    return;
  }

  if (sscanf(str, "%d %s", index, str) != 2)
    {
      index = 0;
    }
  index --;
  if (index < -1)
    {
      message("W marzeniach.\n");
      return;
    }
  
  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
    return;
  }

  if (index >= sizeof(tmp))
    {
      message("Nie ma aż tyle '" + str + "' w okolicy.\n");
      return;
    }
  
  ctr = 0;
  if(sizeof(tmp) > 1)
    {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
	{
	  if(tmp[ctr]->is_openable())
	    break;
	}
      if(ctr >= sizeof(tmp))
	{
	  message("Żadne z tych nie może być zamknięte.\n");
	  return;
	}
      if (index == -1)
	{
	  message("Więcej niż jedno takie jest w okolicy.\n");
	}
      else
	{
	  ctr = index;
	}
      message("Wybierasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  }

  if(!tmp[ctr]->is_openable()) {
    message("Nie możesz tego zamknąć!\n");
    return;
  }

  if(!(err = mobile->close(tmp[ctr]))) {
    message("Zamknąłeś ");
    send_phrase(tmp[0]->get_brief());
    message(".\n");
  } else {
    message(err + "\n");
  }
}

/* Show list of available commands */
static void cmd_commands(object user, string cmd, string str)
{
  string msg;
  int i;
  mixed indices;

  /* Standard commands */
  msg = "Dostępne komendy:";
  indices = map_indices(commands_map) + ({ "wyloguj" });
  for (i = 0; i < sizeof(indices); i++) {
      if (!(i % 5))
          msg += "\n";
      msg += lalign(indices[i], 20);
    }
  msg += "\n\n";
  /* Admin commands */
  if (is_admin()) {
      mixed* admin_commands;
      msg += "Komendy administracyjne:";
      admin_commands = SYSTEM_WIZTOOL->get_command_sets(this_object());
      indices = map_indices(admin_commands[0]);
      for (i = 0; i < sizeof(indices); i++) {
          if (!(i % 5))
              msg += "\n";
    	  msg += lalign(indices[i], 20);
	    }
      msg += "\n";
    }
  message_scroll(msg);
}

/* Wear weapons, armors, etc */
static void cmd_wear(object user, string cmd, string str)
{
  object* tmp;
  int ctr, i;
  mixed* items;
  
  if(str)
    {
      str = STRINGD->trim_whitespace(str);
    }
  if(!str || str == "")
    {
      message("Użycie: " + cmd + " <obiekt>\n");
      return;
    }
  tmp = find_first_objects(str, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp))
    {
      message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
      return;
    }
  ctr = 0;
  if(sizeof(tmp) > 1) {
      for(ctr = 0; ctr < sizeof(tmp); ctr++)
          if (tmp[ctr]->is_wearable() && !tmp[ctr]->is_dressed())
              break;
      if(ctr >= sizeof(tmp))
      {
          message("Żadna z tych rzeczy nie może być założona.\n");
          return;
      }
      message("Więcej niż jedna taka rzecz jest w ekwipunku.\n");
      message("Wybierasz ");
      send_phrase(tmp[ctr]->get_brief());
      message(".\n");
  }
  if(!tmp[ctr]->is_wearable())
    {
      message("Nie możesz tego założyć!\n");
      return;
    }
  if (tmp[ctr]->is_dressed())
    {
      message("Ten obiekt jest już założony!\n");
      return;
    }
  items = body->objects_in_container();
  if (tmp[ctr]->get_wearlocations()[0] == 5) {
      for (i = 0; i < sizeof(items); i++) {
          if (items[i]->is_wearable() && items[i]->get_wearlocations()[0] == 5 && items[i]->is_dressed()) {
              message("Chowasz " + items[i]->get_brief()->to_string(user) + ".\n");
              items[i]->set_dressed(0);
              break;
          }
      }
      message("Bierzesz " + tmp[ctr]->get_brief()->to_string(user) + " do ręki.\n");
  }
  else {
      for (i = 0; i < sizeof(items); i++) {
          if (items[i]->is_wearable() && items[i]->get_wearlocations()[0] != 5 && items[i]->is_dressed()) {
              if (sizeof(tmp[ctr]->get_wearlocations() - items[i]->get_wearlocations()) < sizeof(tmp[ctr]->get_wearlocations())) {
                  message("Zdejmujesz " + items[i]->get_brief()->to_string(user) + ".\n");
                  items[i]->set_dressed(0);
              }
          }
      }
      message("Zakładasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
    }
  tmp[ctr]->set_dressed(1);
}

/* Take off equipment */
static void cmd_takeoff(object user, string cmd, string str)
{
  object* tmp;
  int ctr, i;
  mixed* items;
  
  if(str)
    {
      str = STRINGD->trim_whitespace(str);
    }
  if(!str || str == "")
    {
      message("Użycie: " + cmd + " <obiekt>\n");
      return;
    }
  tmp = find_first_objects(str, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp))
    {
      message("Nie możesz znaleźć jakiegokolwiek '" + str + "'.\n");
      return;
    }
  ctr = 0;
  if(sizeof(tmp) > 1) {
      for(ctr = 0; ctr < sizeof(tmp); ctr++) {
	  if (tmp[ctr]->is_wearable() && tmp[ctr]->is_dressed())
	      break;
      }
      if(ctr >= sizeof(tmp))
      {
          message("Żadna z tych rzeczy nie może być zdjęta.\n");
          return;
      }
  }
  if (!tmp[ctr]->is_dressed())
    {
      message("Ten obiekt nie jest założony!\n");
      return;
    }
  items = body->objects_in_container();
  if (tmp[ctr]->get_wearlocations()[0] == 5)
      message("Chowasz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  else
      message("Zdejmujesz " + tmp[ctr]->get_brief()->to_string(user) + ".\n");
  tmp[ctr]->set_dressed(0);
}

/* Start combat */
static void cmd_attack(object user, string cmd, string str)
{
    object *tmp;
    string target;
    int number;
    string *nouns;

    if(str)
        str = STRINGD->trim_whitespace(str);
    if(!str || str == "") {
        message("Użycie: " + cmd + " <obiekt>\n");
        return;
    }
    if (sscanf(str, "%d %s", number, target) != 2) {
        target = str;
        number = 1;
    }
    tmp = find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (sizeof(tmp) > 1 && !number) {
        message("Jest kilka '" + target + "' w okolicy. Wybierz dokładnie którego chcesz zaatakować.\n");
        return;
    }
    if (number > sizeof(tmp)) {
        message("Nie możesz znaleźć tego '" + target + "'.\n");
        return;
    }
    number -= 1;
    if (!tmp[number]->get_mobile()) {
        message("Możesz atakować tylko istoty. Nie wyżywaj się na sprzęcie.\n");
        return;
    }
    if (!tmp[number]->get_mobile()->get_parentbody()) {
        message("Nie możesz zaatakować tej istoty.\n");
        return;
    }
    if (TAGD->get_tag_value(body, "Combat") || TAGD->get_tag_value(tmp[number], "Combat")) {
        message("Któreś z Was już walczy. Dokończcie najpierw jedną walkę.\n");
        return;
    }
    if (TAGD->get_tag_value(body, "Follow"))
        stop_follow(user);
    nouns = tmp[number]->get_nouns(locale);
    if (sizeof(nouns) < 5)
        message("Atakujesz " + tmp[number]->get_brief()->to_string(user) + "...\n");
    else
        message("Atakujesz " + nouns[3] + "...\n");
    combat = clone_object(COMBAT);
    combat->start_combat(body, tmp[number]);
}

/* Report bug, typo or propose idea */
static void cmd_report(object user, string cmd, string str)
{
    string filename, rtype;

    switch (cmd) {
        case "bug":
            filename = "/usr/game/text/bug_reports.txt";
            rtype = "błędu";
            break;
        case "literowka":
            filename = "/usr/game/text/typo_reports.txt";
            rtype = "literówki";
            break;
        case "idea":
            filename = "/usr/game/text/idea_reports.txt";
            rtype = "pomysłu";
            break;
        default:
            message("Ups, coś poszło nie tak.\n");
            return;
    }
    if (!str || str == "") {
        message("Opis " + rtype + ". Postaraj się podać jak najwięcej szczegółów,\n"
                + "to pomoże nam w pracy.\n");
        push_new_state(US_REPORT_BUG, cmd, user);
        push_new_state(US_ENTER_DATA);
        return;
    }
    else
        write_file(filename, Name + ": " + str + "\n");
    message("Dziękujemy za zgłoszenie " + rtype  + ".\n");
}

/* Player settings - description, email and password change */
static void cmd_settings(object user, string cmd, string str)
{
    string *parts;
    string oldmail;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        message("Użycie: " + cmd + " [opis|mail|haslo|odmiana]\n");
        return;
    }
    parts = explode(str, " ");
    switch (parts[0]) {
        case "opis":
            message("Ustawiasz nowy opis postaci.\n");
            push_new_state(US_SET_DESC, user);
            push_new_state(US_ENTER_DATA);
            break;
        case "mail":
            oldmail = email;
            if (oldmail == "")
                oldmail = "brak adresu";
            if (sizeof(parts) < 2) 
                parts = ({ "mail", "" });
            email = parts[1];
            if (parts[1] == "")
                parts[1] = "brak adresu";
            message("Zmieniono adres mailowy z "+ oldmail + " na " + parts[1] + "\n");
            break;
        case "odmiana":
            push_new_state(US_ENTER_CONJ, user);
            break;
        default:
            message("Nieznana opcja, spróbuj " + cmd + " [opis|mail|haslo|odmiana]\n");
            break;
    }
}

/* Follow other player/mobile */
static void cmd_follow(object user, string cmd, string str)
{
    object *tmp;
    string target;
    int number;
    object leader;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "") {
        message("Użycie: " + cmd + " <cel> lub [przestan]\n");
        return;
    }
    
    if (str == "przestan" && TAGD->get_tag_value(body, "Follow")) {
        stop_follow(user);
        return;
    }
    if (sscanf(str, "%d %s", number, target) != 2) {
        target = str;
        number = 1;
    }
    tmp = find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (sizeof(tmp) > 1 && !number) {
        message("Jest kilka '" + target + "' w okolicy. Wybierz dokładnie za kim chcesz podążać.\n");
        return;
    }
    if (number > sizeof(tmp)) {
        message("Nie możesz znaleźć tego '" + target + "'.\n");
        return;
    }
    number -= 1;
    if (!tmp[number]->get_mobile()) {
        message("Możesz podążać tylko za żywymi istotami. Martwa natura nie porusza się.\n");
        return;
    }
    TAGD->set_tag_value(body, "Follow", tmp[number]->get_number());
    message("Zaczynasz podążać za " + tmp[number]->get_brief()->to_string(user) + "\n");
    if (tmp[number]->get_mobile()->get_user())
        tmp[number]->get_mobile()->get_user()->message(Name + " zaczyna podążać za Tobą.\n");
}

/* Temporary command for packages delivery */
static void cmd_give(object user, string cmd, string str)
{
    string item, target;
    object *tmp, *items;

    if (str)
        str = STRINGD->trim_whitespace(str);
    if (!str || str == "" || sscanf(str, "%s %s", item, target) != 2) {
        message("Użycie: " + cmd + " paczka <cel>\n");
        return;
    }
    
    items = find_first_objects(item, LOC_IMMEDIATE_INVENTORY);
    if (!items || !sizeof(items)) {
        message("Nie masz takiego przedmiotu jak '" + item + "'\n");
        return;
    }
    tmp = find_first_objects(target, LOC_IMMEDIATE_CURRENT_ROOM);
    if(!tmp || !sizeof(tmp)) {
        message("Nie możesz znaleźć jakiegokolwiek '" + target + "'.\n");
        return;
    }
    if (tmp[0]->get_number() != TAGD->get_tag_value(items[0], "Recipient")) {
        message("Nie możesz dać tego przedmiotu akurat tej osobie.\n");
        return;
    }
    body->set_price(body->get_price() + 2);
    items[0]->get_location()->remove_from_container(items[0]);
    destruct_object(items[0]);
    commands_map["daj"] = nil;
    message("Przekazałeś paczkę odbiorcy i dostałeś 2 miedziaki.\n");
}

/* Show current quest info and old quests */
static void cmd_quests(object user, string cmd, string str)
{
    string msg;
    string *questinfo;
    int i;
    
    if (TAGD->get_tag_value(body->get_mobile(), "Quest") == nil)
        msg = "Obecnie nie uczestniczysz w jakiejkolwiek przygodzie.\n\n";
    else {
        questinfo = QUESTD->get_quest_info(TAGD->get_tag_value(body->get_mobile(), "Quest"));
        msg = "Przygoda: " + questinfo[0] + "\n\n" + questinfo[1] + "\n\n";
    }

    if (!sizeof(quests)) {
        msg += "Nie ukończyłeś jeszcze jakiejkolwiek przygody.\n";
        message(msg);
        return;
    }

    msg += "Lista ukończonych przygód:\n";
    for (i = 0; i < sizeof(quests); i++)
        msg += lalign((string)(i + 1), 5) + quests[i] + "\n";

    message_scroll(msg);
}
