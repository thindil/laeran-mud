#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>
#include <phantasmal/tagd.h>

#include <type.h>

static mapping mobile_tags;
static mapping object_tags;

static void create(void) {
  mobile_tags = ([ ]);
  object_tags = ([ ]);
}

void upgraded(varargs int clone) {
  if(!COMMON() && !SYSTEM())
    return;
}

private void check_value_type(int type, mixed value) {
  if(typeof(value) == type || typeof(value) == T_NIL)
    return;
  
  error("Pomyłka w typach:  typ " + typeof(value)
	+ " nie pasuje do tagu o typie " + type + "!");
}

void new_mobile_tag(string name, int type,
		    varargs string get_function, string set_function) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może tworzyć nowe tagi dla mobów!");

  if(mobile_tags[name])
    error("Tag dla mobków '" + name + "' już istnieje!");

  if(type <= T_NIL || type > T_MAPPING)
	error(type + " nie jest prawidłowym typem dla tagu!");

  mobile_tags[name] = ({ type, get_function, set_function });
}

void new_object_tag(string name, int type,
		    varargs string get_function, string set_function,
		    int inherit_type) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może tworzyć nowe tagi dla obiektów!");

  if(object_tags[name])
    error("Tag dla obiektów '" + name + "' już istnieje!");

  if(type <= T_NIL || type > T_MAPPING)
	error(type + " nie jest prawidłowym typem dla tagu!");

  if(inherit_type < TAG_INHERIT_NONE || inherit_type > TAG_INHERIT_MAX)
    error("Nie rozpoznaję numeru TAG_INHERIT " + inherit_type + " jako prawidłowego numeru!");

  if(inherit_type == TAG_INHERIT_MERGE && type == T_OBJECT) {
	error("Nie mogę atomatycznie połączyć dwóch lub więcej obiektów. Zmień typ dziedziczenia albo obiektu!");
  }

  object_tags[name] = ({ type, get_function, set_function, inherit_type });
}

mixed mobile_get_tag_value(object mobile, string name) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może pobierać wartości tagów mobili!");

  if(function_object("set_number", mobile) != MOBILE)
    error("Można wywoływać mobile_get_tag_value tylko na mobkach!");

  tag_arr = mobile_tags[name];
  if(!tag_arr)
    error("Nie ma takiego tagu dla mobili jak '" + name + "'!");

  return call_other(mobile, (tag_arr[1] ? tag_arr[1] : "get_tag"), name);
}

void mobile_set_tag_value(object mobile, string name, mixed value) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może ustawiać tagi dla mobili!");

  if(function_object("set_number", mobile) != MOBILE)
    error("Można wywoływać call mobile_set_tag_value tylko na mobkach!");

  tag_arr = mobile_tags[name];
  if(!tag_arr)
    error("Nie ma takiego tagu dla mobili jak '" + name + "'!");

  check_value_type(tag_arr[0], value);

  call_other(mobile, (tag_arr[2] ? tag_arr[2] : "set_tag"), name, value);
}

mixed object_get_tag_value(object obj, string name) {
  mixed  *tag_arr;
  mixed   tag_val, tmp;
  object *parents;
  int     ctr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może pobierać wartości tagów dla obiektów!");

  if(function_object("set_number", obj) != OBJECT)
    error("Można wywoływać object_get_tag_value tylko na obiektach!");

  tag_arr = object_tags[name];
  if(!tag_arr)
    error("Nie ma takiego tagu obiektów jak '" + name + "'!");

  tag_val = call_other(obj, (tag_arr[1] ? tag_arr[1] : "get_tag"), name);
  if(tag_val) return tag_val;
  
  parents = obj->get_archetypes();
  if(!parents || !sizeof(parents)) {
	  return nil;
  }

  switch(tag_arr[3]) {
	case TAG_INHERIT_NONE:
	  return nil;
	case TAG_INHERIT_FIRST:
	  return object_get_tag_value(parents[0], name);
	case TAG_INHERIT_MERGE:
	  tag_val = (tag_arr[0] == T_INT ? 0
	             : (tag_arr[3] == T_FLOAT ? 0.0
			: (tag_arr[3] == T_STRING ? ""
			   : nil)));
	  for(ctr = 0; ctr < sizeof(parents); ctr++) {
		switch(tag_arr[0]) {
		  case T_INT:
		  case T_FLOAT:
		  case T_STRING:
		    tag_val += tmp;
		    break;
		  case T_ARRAY:
		  case T_MAPPING:
		  	tag_val = tag_val | tmp;
		  	break;
		}
	  }
  }
  return tag_val;
}

void object_set_tag_value(object obj, string name, mixed value) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może ustawiać wartości tagów dla obiektów!");

  if(function_object("set_number", obj) != OBJECT)
    error("Można wywoływać object_set_tag_value tylko na obiektach!");

  tag_arr = object_tags[name];
  if(!tag_arr)
    error("Nie ma takiego tagu dla obiektów jak '" + name + "'!");

  check_value_type(tag_arr[0], value);

  call_other(obj, (tag_arr[2] ? tag_arr[2] : "set_tag"), name, value);
}

mixed get_tag_value(object tagged_object, string name) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać get_tag_value!");

  switch(function_object("set_number", tagged_object)) {
  case OBJECT:
    return object_get_tag_value(tagged_object, name);
  case MOBILE:
    return mobile_get_tag_value(tagged_object, name);
  default:
    error("Nie rozpoznany typ tagu w get_tag_value!");
  }
}

void set_tag_value(object tagged_object, string name, mixed value) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać set_tag_value!");

  switch(function_object("set_number", tagged_object)) {
  case OBJECT:
    object_set_tag_value(tagged_object, name, value);
    return;
  case MOBILE:
    mobile_set_tag_value(tagged_object, name, value);
    return;
  default:
    error("Nie rozpoznany typ tagu w set_tag_value!");
  }
}

string* mobile_tag_names(void) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać mobile_tag_names!");

  return map_indices(mobile_tags);
}

string* object_tag_names(void) { 
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać object_tag_names!");

  return map_indices(object_tags);
}

mixed* mobile_all_tags(object mobile) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać mobile_all_tags!");

  return mobile->get_all_tags();
}

mixed* object_all_tags(object obj) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać object_all_tags!");

  return obj->get_all_tags();
}

int mobile_tag_type(string tag_name) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać mobile_tag_type!");

  if(mobile_tags[tag_name])
    return mobile_tags[tag_name][0];

  return -1;
}

int object_tag_type(string tag_name) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać object_tag_type!");

  if(object_tags[tag_name])
    return object_tags[tag_name][0];

  return -1;
}

string to_unq_text(int number)
{
    string tagtmp, name;
    string *indices;

    if (!SYSTEM() && !COMMON())
        return nil;

    indices = map_indices(mobile_tags);
    if (number < sizeof(indices)) {
        name = indices[number];
        tagtmp = "~mobile_tag{\n"
            + "  ~name{" + name + "}\n"
            + "  ~type{" + mobile_tags[name][0] + "}\n";
        if (mobile_tags[name][1])
            tagtmp += "  ~getter{" + mobile_tags[name][1] + "}\n";
        if (mobile_tags[name][2])
            tagtmp += "  ~setter{" + mobile_tags[name][2] + "}\n";
        tagtmp += "}\n";
        return tagtmp;
    } else
        number -= sizeof(indices);
    
    indices = map_indices(object_tags);
    if (number < sizeof(indices)) {
        name = indices[number];
        tagtmp = "~object_tag{\n"
            + "  ~name{" + name + "}\n"
            + "  ~type{" + object_tags[name][0] + "}\n";
        if (object_tags[name][1])
            tagtmp += "  ~getter{" + object_tags[name][1] + "}\n";
        if (object_tags[name][2])
            tagtmp += "  ~setter{" + object_tags[name][2] + "}\n";
        tagtmp += "}\n";
        return tagtmp;
    }

    return nil;
}

int num_tags(void)
{
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać num_tags!");

  return sizeof(map_indices(object_tags)) + sizeof(map_indices(mobile_tags));
}

void clear_tags(void)
{
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może wywoływać clear_tags!");

  object_tags = ([ ]);
  mobile_tags = ([ ]);
}
