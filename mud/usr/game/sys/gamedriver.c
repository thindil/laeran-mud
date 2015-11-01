#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

string welcome_message;
string shutdown_message;
string suspended_message;
string sitebanned_message;
string *banned_ip;

static void create(void)
{
	string file_tmp;

	file_tmp = read_file("/usr/game/text/welcome.msg");
	if (!file_tmp)
		error("Can't read /usr/game/text/welcome.msg!");
	welcome_message = file_tmp;

	file_tmp = read_file("/usr/game/text/shutdown.msg");
	if (!file_tmp)
		error("Can't read /usr/game/text/shutdown.msg!");
	shutdown_message = file_tmp;

	file_tmp = read_file("/usr/game/text/suspended.msg");
	if (!file_tmp)
		error("Can't read /usr/game/text/suspended.msg!");
	suspended_message = file_tmp;

	file_tmp = read_file("/usr/game/text/sitebanned.msg");
	if (!file_tmp)
		error("Can't read /usr/game/text/sitebanned.msg!");
	sitebanned_message = file_tmp;

    file_tmp = read_file("/usr/game/text/bans.txt");
    if (!file_tmp)
        banned_ip = ({ });
    else
        banned_ip = explode(file_tmp, ", ");
}

#define GAME_USER "/usr/game/obj/user"
object new_user_connection(string first_line) {
  if(!find_object(GAME_USER))
    compile_object(GAME_USER);

  return clone_object(GAME_USER);
}

string get_welcome_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return welcome_message;
}

string get_shutdown_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return shutdown_message;
}

string get_suspended_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return suspended_message;
}

string get_sitebanned_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return sitebanned_message;
}

string* get_banned_ip(void)
{
    return banned_ip;
}

void set_banned_ip(string *banned)
{
    banned_ip = banned;
}

void add_banned_ip(string value)
{
    banned_ip += ({ value });
}

/* check if ip is banned, return 1 for true */
int site_is_banned(string ip)
{
    int i, j;

    for (i = 0; i < sizeof(banned_ip); i++)
        if (ip == banned_ip[i])
            return 1;

    return 0;
}
