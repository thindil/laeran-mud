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

#define GAME_USERCOMMANDSLIB "/usr/game/lib/usercommands"

inherit phan PHANTASMAL_USER;
inherit usrcmds GAME_USERCOMMANDSLIB;

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
        void death(void);
        void set_health(int hp);
        void set_condition(int fatigue);
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
mapping aliases;

/*
 * NAME:	create()
 * DESCRIPTION:	initialize user object
 */
static void create(int clone)
{
  phan::create(clone);
  usrcmds::create(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    phan::upgraded(clone);
    usrcmds::upgraded(clone);

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
             "alias"     : "cmd_aliases",

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
             "zamien"    : "cmd_transform",
             "napraw"    : "cmd_repair",

		     "at"        : "cmd_attack",
		     "atak"      : "cmd_attack",
		     "atakuj"    : "cmd_attack"

    ]);

  }
}

static void destructed(varargs int clone)
{
    usrcmds::destructed(clone);
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
    float  weight, volume;

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
    if (aliases == nil)
        aliases = ([ ]);

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

        /* Recount player current weight and volume */
        inv = body->objects_in_container();
        weight = 0.0;
        volume = 0.0;
        for (i = 0; i < sizeof(inv); i++) {
            weight += inv[i]->get_weight();
            volume += inv[i]->get_volume();
        }
        body->set_current_volume(volume);
        body->set_current_weight(weight);
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

        /* Aliases */
        if (map_sizeof(aliases) && sizeof(map_indices(aliases) & ({ cmd }))) {
            command = explode(aliases[cmd], " ");
            cmd = command[0];
            str = implode(command[1..], " ") + " " + str;
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

string* get_skills_names(void)
{
    return map_indices(skills);
}

int get_gender(void)
{
    return gender;
}

string get_health(void)
{
    return health;
}

string get_condition(void)
{
    return condition;
}

object get_location(void)
{
    return location;
}

void begin_combat(object enemy)
{
    combat = clone_object(COMBAT);
    combat->start_combat(body, enemy);
}

void end_combat(void)
{
    combat->stop_combat();
    destruct_object(combat);
    message("Uciekasz z walki.\n");
}

string *get_commands(void)
{
    return map_indices(commands_map);
}

int get_locale(void)
{
    return locale;
}

string get_email(void)
{
    return email;
}

void set_email(string new_email)
{
    email = new_email;
}

void delete_cmd(string name)
{
    commands_map[name] = nil;
}

mapping get_aliases(void)
{
    return aliases;
}

void set_alias(string name, string value)
{
    aliases[name] = value;
}

object get_mobile(void)
{
    return mobile;
}
