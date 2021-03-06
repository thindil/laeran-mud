#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <trace.h>
#include <limits.h>

#define INDENT_LEVEL 4
#define LOG_FILE ("/usr/common/unq_dtd_log.txt")

private mapping dtd;
private int     is_clone;
private mapping builtins;
private string  accum_error;
/* base type of a type -- for inheritance */
private mapping base_type;

/* #define USE_LOG */
/* You can #define USE_LOG to write to a log file.  This doesn't
   use the standard LOGD logging because LOGD requires UNQ DTD
   functionality to successfully start up, so this may all occur
   before you *can* use LOGD.
*/

#ifdef USE_LOG
#define write_log(str) write_file(LOG_FILE, (str) + "\n")
#else 
#define write_log(str)
#endif

static int create(varargs int clone) {
  is_clone = clone;
  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);

  dtd = ([ ]);

  base_type = ([ ]);

  if(!is_clone) {
    /* Note: may eventually allow additions to the builtins array
       for user-defined types -- effectively that's what Phantasmal
       does with 'phrase'. */
    builtins = ([ "string" : 1,
		  "int" : 1,
		  "float" : 1,
		  "unq" : 1,
		  "phrase" : 1 ]);
  }
}

/* Functions for creating UNQ text from DTD template data */
        string serialize_to_dtd(mixed* dtd_unq);
private string serialize_to_dtd_type(string label, mixed unq, int indent);
private string serialize_to_string_with_mods(mixed* type, mixed unq,
					     int indent);
private string serialize_to_dtd_struct(string label, mixed unq, int indent);
private string serialize_to_builtin(string type, mixed unq, int indent);

/* Functions for parsing and verifying DTD UNQ input */
        mixed* parse_to_dtd(mixed* unq);
private mixed* parse_to_dtd_type(string label, mixed unq);
private mixed  parse_to_string_with_mods(mixed* type, mixed unq);
private mixed* parse_to_dtd_struct(string label, mixed unq);
private mixed  parse_to_builtin(string type, mixed unq);

/* Functions for loading a DTD */
private void   new_dtd_element(string label, mixed data);
private mixed* dtd_struct(string* array);
private mixed* dtd_string_with_mods(string str);

/* Random helper funcs */
private int     set_up_fields_mapping(mapping fields, string label);



/* This function should be called only as UNQ_DTD->is_builtin(...),
   never locally from a clone.  It won't work if called on a clone.
   That's mainly because there's only one builtins mapping, and it's
   stored in a central location.  To save the trouble of making this
   happen later, I'm doing it now -- if we start adding user-defined
   types to builtins, we'll need to have only a single instance of it.
*/
mixed is_builtin(string label) {
  if(is_clone)
    error("Wywołuj is_builtin tylko na głównym obiekcie unq_dtd!");

  return builtins[label];
}

#define SPACES_MAX "                "
#define SPACES_MAX_LEN 16
private string indent_spaces(int num) {
  int tmp;

  tmp = num > SPACES_MAX_LEN ? SPACES_MAX_LEN : num;
  if(tmp < 1) return "";

  return SPACES_MAX[0..tmp];
}

/*************************** Functions for Serializing ***************/

/* This function Takes UNQ input that could have been the result of
   this DTD and converts it to an UNQ string suitable for writing
   to a file.  Returns its error in accum_error, which may be
   queried with get_parse_error_stack. */
string serialize_to_dtd(mixed* unq) {
  string ret, data;

  ret = "";
  accum_error = "";

  if(sizeof(unq) % 2) {
    accum_error += "Parzysty rozmiar UNQ przekazany do serialize_to_dtd!\n";
    return nil;
  }

  while(sizeof(unq) > 0) {
    if(sizeof(unq) == 1) {
      accum_error += "Wewnętrzny błąd (dziwny rozmiar danych) w serialize_to_dtd!\n";
      return nil;
    }

    if(!unq[0]) {
      accum_error += "Etykieta (nil) przekazana do serialize_to_dtd!";
      return nil;
    }

    if(dtd[unq[0]]) {
      data = serialize_to_dtd_type(unq[0], unq[1], 0);
      if(!data) {
	accum_error += "Błąd w etykiecie '" + unq[0] + "'.\n";
	return nil;
      }
      ret += data;
    } else {
      accum_error += "Nierozpoznany typ '" + unq[0] + "'.\n";
      return nil;
    }

    /* Cut off leading UNQ tag and contents */
    unq = unq[2..];
  }

  return ret;
}


/* The label should be passed in as arg1, and the unq (excluding the
   label itself) as arg 2. */
private string serialize_to_dtd_type(string label, mixed unq,
				     int indent) {
  string tmp;

  if(dtd[label][0] == "struct") {
    return serialize_to_dtd_struct(label, unq, indent);
  }

  tmp = serialize_to_string_with_mods(dtd[label], unq, indent);
  if(tmp && !UNQ_DTD->is_builtin(label)) {
    tmp = indent_spaces(indent) + "~" + label + "{" + tmp + "}\n";
  }
  if(!tmp) {
    accum_error += "Nie można szeregować " + implode(dtd[label], "")
      + "\n";
    return nil;
  }
  return tmp;
}


/* This serializes the given type, but doesn't wrap it in its appropriate
   label.  The caller will need to do that, if appropriate. */
private string serialize_to_string_with_mods(mixed* type, mixed unq, int indent) {
  string ret;
  int    ctr, is_struct;
  int repmin, repmax;

  if(sizeof(type) != 1 && sizeof(type) != 2)
    error("Niedozwolony typ podany w serialize_to_string_with_mods!");

  if(sizeof(type) == 1) {
    if(UNQ_DTD->is_builtin(type[0]))
      return serialize_to_builtin(type[0], unq, indent);

    if(dtd[type[0]])
      return serialize_to_dtd_type(type[0], unq, indent);

    accum_error += "Nierozpoznany typ '" + type[0]
      + "' w serialize_to_string_with_mods!\n";
    return nil;
  }

  /* Sizeof(type) == 2, so it's a type/mod combo */
  repmin = 0;
  repmax = INT_MAX;
  if(type[1] == "?") {
    repmax = 1;
  } else if (type[1] == "+") {
    repmin = 1;
  } else if (type[1] == "*") {
    /* do nothing, any value is acceptable */
  } else if (sscanf(type[1], "<%d..%d>", repmin, repmax) == 2 || sscanf(type[1], "<..%d>", repmax) == 1 || sscanf(type[1], "<%d..>", repmin) == 1) {
    /* do nothing, all values already entered */
  } else if (sscanf(type[1], "<%d>", repmin) == 1) {
    repmax = repmin;
  } else {
    accum_error += "Nierozpoznany modyfikator typu " + type[1] + "!\n";
    return nil;
  }

  if(typeof(unq) == T_STRING) {
    /* TODO:  type checking here */
    return unq;
  }
  if(typeof(unq) != T_ARRAY) {
    accum_error += "Niemożliwy do szeregowania typ UQ jako UNQ "
      + implode(type, ",") + "!\n";
    return nil;
  }

  /* Multiple entries */
  /* First, check to see that the number of entries is reasonable */
  if (sizeof(unq) < repmin || sizeof(unq) > repmax) {
    accum_error += "Liczba wpisów nie pasuje do modyfikatora +\n";
    return nil;
  }

  /* Set is_struct flag for below... */
  if(UNQ_DTD->is_builtin(type[0])) {
    is_struct = 0;
  } else if(dtd[type[0]][0] == "struct") {
    is_struct = 1;
  } else {
    accum_error += "Type '" + type[0]
      + "' nie wygląda na strukturę bądź wbudowany typ!\n";
    return nil;
  }

  ret = "";
  for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
    string tmp;

    if(is_struct)
      tmp = serialize_to_dtd_struct(type[0], unq[ctr + 1], indent);
    else
      tmp = serialize_to_builtin(type[0], unq[ctr + 1], indent);

    if(tmp == nil)
      return nil;

    ret += tmp;
  }

  return ret;
}


/* The label should be passed in as arg1, and the unq (excluding the
   label itself) as arg 2. */
private string serialize_to_dtd_struct(string label, mixed unq,
				       int indent) {
  string  ret;
  mixed*  type;
  mapping fields;
  int     ctr;

  ret = indent_spaces(indent) + "~" + label + "{\n";
  type = dtd[label];
  if(!type || type[0] != "struct") {
    accum_error += "Nie struktura przekazana do serialize_to_dtd_struct!\n";
    return nil;
  }

  /* Rather than an instance tracker, we're just going to serialize
     the fields in the order given.  That puts some constraints on
     the UNQ that gets passed in, but that's fine for now.  Like
     so much of this code, it'll be expanded later if we need the
     new functionality. */

  fields = ([ ]);
  set_up_fields_mapping(fields, label);

  if(typeof(unq) != T_ARRAY) {
    string tmp;

    if(sizeof(type) != 2
       || !UNQ_DTD->is_builtin(type[1][0])) {
      accum_error += "Nie mogę szeregować struktury '" + label + "' z '"
	+ unq + "' (typ " + typeof(unq) + ").\n";
      return nil;
    }

    tmp = serialize_to_string_with_mods(type[1], unq, indent);
    if(tmp) {
      /* serialize_to_string_with_mods will supply its own curly
	 braces. */
      return indent_spaces(indent) + "~" + label + tmp + "\n";
    }
    return nil;
  }

  /* (typeof(unq) == T_ARRAY) from here on */

  for(ctr = 0; ctr < sizeof(unq); ctr++) {
    string tmp;

    if(!fields[unq[ctr][0]]) {
      accum_error += "Nie rozpoznane pole '" + STRINGD->mixed_sprint(unq[ctr])
	+ "' na indeksie " + ctr + ".\n";
      return nil;
    }

    tmp = serialize_to_dtd_type(unq[ctr][0], unq[ctr][1],
				indent + INDENT_LEVEL);
    if(!tmp) {
      accum_error += "Błąd przy zapisywaniu pola '" + unq[ctr][0] + "' w strukturze.\n";
      return nil;
    }
    /* ret += indent_spaces(indent) + "~" + unq[ctr][0] + tmp + "\n"; */
    ret += tmp;
  }

  ret += + indent_spaces(indent) + "}\n";
  return ret;
}


private string serialize_to_builtin(string type, mixed unq, int indent) {
  if(type == "string") {
    if(typeof(unq) != T_STRING) {
      accum_error += "Typ " + typeof(unq) + " nie jest string!\n";
      return nil;
    }
    return "{" + unq + "}";
  }

  if(type == "int") {
    if(typeof(unq) != T_INT) {
      accum_error += "Typ " + typeof(unq) + " nie jest int!\n";
      return nil;
    }
    return "{" + unq + "}";
  }

  if(type == "float") {
    if(typeof(unq) != T_FLOAT) {
      accum_error += "Typ " + typeof(unq) + " nie jest float!\n";
    }
    return "{" + unq + "}";
  }

  if(type == "phrase") {
    if(typeof(unq) == T_STRING) {
      return "{" + unq + "}";
    }
    return "{" + unq->to_unq_text() + "}";
  }

  if(type == "unq") {
    if(typeof(unq) == T_STRING
       || typeof(unq) == T_INT
       || typeof(unq) == T_FLOAT)
      return "{" + unq + "}";

    error("Nie mogę jeszcze szeregować UNQ, zaimplementuj mnie!");
  }

  accum_error += "Nie rozpoznaję typu " + type + " w szeregowaniu do wbudowanych!\n";
  return nil;
}

/*************************** Functions for Parsing *******************/

/* Takes arbitrary parsed UNQ input and makes it conform to this DTD
   if possible.  Returns errors in accum_error, which may be queried
   with get_parse_error_stack. */
mixed* parse_to_dtd(mixed* unq) {
  int    ctr;
  string label;
  mixed* data, ret;

  accum_error = "";

  ret = ({ });
  unq = UNQ_PARSER->trim_empty_tags(unq);

  write_log("============================================================");
  write_log("Parsowanie do DTD: '" + STRINGD->mixed_sprint(unq) + "'\n-----");

  for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
    label = STRINGD->trim_whitespace(unq[ctr]);
    if(dtd[label]) {
      data = parse_to_dtd_type(label, unq[ctr + 1]);
      if(data == nil) {
	accum_error += "Pomyłka przy etykiecie '" + label + "'\n";
	return nil;
      }
      write_log("Zakończono parsowanie typu '" + label + "'.");
      ret += data;
      continue;
    }

    accum_error += "Nie rozpoznaję etykiety '" + label + "' w DTD.\n";
    return nil;
  }

  return ret;
}

string get_parse_error_stack(void) {
  return accum_error;
}

/* Label is an entry in the DTD, unq is a chunk of input to parse
   assuming it conforms to that label.

   Returns a labelled array.
*/
private mixed* parse_to_dtd_type(string label, mixed unq) {
  mixed* type;
  mixed  tmp;

  if(typeof(unq) == T_ARRAY) {
    unq = UNQ_PARSER->trim_empty_tags(unq);
  }

  write_log("Parsowanie do DTD typu '" + label + "'");

  type = dtd[label];
  if(typeof(type) != T_ARRAY && typeof(type) != T_STRING)
    error("Nieprawidłowy typ w DTD w parse_to_dtd_type!");

  /* If it's a struct */
  if(type[0] == "struct") {
    return parse_to_dtd_struct(label, unq);
  }

  /* Else, not a struct. */

  tmp = parse_to_string_with_mods(type, unq);

  if(tmp == nil) {
    accum_error += "Nie można sparsować " + STRINGD->mixed_sprint(unq)
      + " jako typu " + implode(type,"") + "\n";
    return nil;
  }
  return ({ label, tmp });
}

/* Returns a typed chunk or (with mods) an array of typed chunks
   fitting the builtin or builtin with modifier.  The item returned
   will not be prefixed with the appropriate label.  If desired,
   that may be done by the caller. */
private mixed parse_to_string_with_mods(mixed* type, mixed unq) {
  int repmin, repmax;

  if(sizeof(type) != 1 && sizeof(type) != 2)
    error("Niedozwolony typ podany do parse_to_string_with_mods!");

  if(unq == nil)
    return nil;

  if(sizeof(type) == 1) {
    if(UNQ_DTD->is_builtin(type[0]))
      return parse_to_builtin(type[0], unq);

    if(dtd[type[0]])
      return parse_to_dtd_type(type[0], unq);

    accum_error += "Nierozpoznany typ '" + type[0]
      + "' w parse_to_string_with_mods!\n";
    return nil;
  }

  /* Sizeof(type) == 2, so it's a type/mod combo */
  repmin = 0;
  repmax = INT_MAX;
  if(type[1] == "?") {
    repmax = 1;
  } else if (type[1] == "+") {
    repmin = 1;
  } else if (type[1] == "*") {
    /* do nothing, any value is acceptable */
  } else if (sscanf(type[1], "<%d..%d>", repmin, repmax) == 2
	     || sscanf(type[1], "<..%d>", repmax) == 1
	     || sscanf(type[1], "<%d..>", repmin) == 1) {
    /* do nothing, all values already entered */
  } else if (sscanf(type[1], "<%d>", repmin) == 1) {
    repmax = repmin;
  } else {
    accum_error += "Nierozpoznany modyfikator typu " + type[1] + "!\n";
    return nil;
  }

  if(UNQ_DTD->is_builtin(type[0])) {
    mixed* ret;
    int    ctr;
    mixed  tmp;

    if(typeof(unq) == T_STRING) {
      tmp = parse_to_builtin(type[0], unq);
      if(tmp == nil) return nil;

      /* Only one obj, but since this has modifiers return it as
	 an array-of-one. */
      if(repmin <= 1 && repmax >= 1)
	return ({ tmp });
      else {
	accum_error += "Liczba danych nie pasuje do " + type[1]
	  + " modyfikatora\n";
	return nil;
      }
    }

    if(typeof(unq) != T_ARRAY) {
      /* Not a string and not an array.  Error! */
      error("Bezsensowny typ parsowany w UNQ!");
    }

    /* Okay, multiple entries -- typeof(unq) is T_ARRAY */
    unq = UNQ_PARSER->trim_empty_tags(unq);
    ret = ({ });
    for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
      mixed tmp;

      if(!STRINGD->is_whitespace(unq[ctr])) {
	accum_error += "Oznaczone dane znalezione podczas parsowania "
	  + type[0] + type[1] + "!\n";
	return nil;
      }

      tmp = parse_to_builtin(type[0], unq[ctr + 1]);
      if(tmp == nil) return nil;

      ret += ({ tmp });
    }

    if (sizeof(ret) < repmin || sizeof(ret) > repmax) {
      accum_error += "Liczba danych nie pasuje do modyfikatora " + type[1] + "\n";
      return nil;
    }

    return ret;
  }

  accum_error
    += "Nie ma wsparcia jeszcze dla modyfikatorów dla nie-wbudowanych typów!\n";
  return nil;
}

/* Returns a typed chunk of data which is a string, an int, etc as
   appropriate.  The returned chunk will not be prefixed with a
   label. */
private mixed parse_to_builtin(string type, mixed unq) {
  string err;

  if(!UNQ_DTD->is_builtin(type))
    error("Typ " + type + " nie jest wbudowany w parse_to_builtin!");

  /* Nil won't parse as anything -- save some checking code below */
  if(unq == nil)
    return nil;

  if(type == "string") {
    if(typeof(unq) != T_STRING) {
      accum_error += "Typ " + typeof(unq) + " nie jest string\n";
      return nil;
    }
    return STRINGD->trim_whitespace(unq);
  }

  if(type == "int") {
    int val;

    if(typeof(unq) != T_STRING) {
      accum_error += "Typ " + typeof(unq)
	+ " nie jest string kiedy parsowano int\n";
      return nil;
    }
    unq = STRINGD->trim_whitespace(unq);
    if(!sscanf(unq, "%d", val)) {
      accum_error += unq + " nie jest int.\n";
      return nil;
    }

    return val;
  }

  if(type == "float") {
    float val;

    if(typeof(unq) != T_STRING) {
      accum_error += "Typ " + typeof(unq)
	+ " nie jest string kiedy parsowano float\n";
      return nil;
    }

    if(!sscanf(unq, "%f", val)) {
      accum_error += unq + " nie jest float.\n";
      return nil;
    }

    return val;
  }

  if(type == "phrase") {
    object tmp;

    if(typeof(unq) == T_STRING)
      return PHRASED->new_simple_english_phrase(unq);

    if(typeof(unq) != T_ARRAY)
      error("Nie rozpoznany obiekt UNQ w parse_to_builtin(phrase)!");

    err = catch(tmp = PHRASED->unq_to_phrase(unq));

    if (err != nil) {
      accum_error += err;
      return nil;
    }
     
    return tmp;
  }

  if(type == "unq") {
    if(typeof(unq) == T_STRING) {
      return ({ "", unq }) ;
    }

    if(typeof(unq) != T_ARRAY) {
      accum_error += "Wbudowany obiekt 'unq' nie jest array lub string!\n";
      return nil;
    }

    if(sizeof(unq) % 2) {
      accum_error += "Wbudowana tablica 'unq' nie ma indeksy podzielnego przez 2!\n";
      return nil;
    }

    return unq;
  }

  error("Wbudowana tablica zmodyfikowana bez modyfikacji parse_to_builtin!");
}

/* Parse_to_dtd_struct assumes some input preprocessing -- label is
   whitespace-trimmed and unq's top level is empty-tag-trimmed.  Label
   is also validated to point to an UNQ DTD structure.  UNQ may or may
   not be a valid structure, but has already had the tag corresponding
   to label trimmed from it.

   Returns an array consisting of a label followed by an array
   of labelled fields.
*/
private mixed* parse_to_dtd_struct(string t_label, mixed unq) {
  mixed*  type, *instance_tracker, *ret, *vals;
  mixed   tmp;
  int     ctr;
  mapping fields;
  string  label;
  int     bucket_count;
  int     repmin, repmax;

  write_log("Przekazywanie do struktury dtd '" + t_label + "', arg: '"
	    + STRINGD->mixed_sprint(unq) + "'");

  type = dtd[t_label];
  if(type[0] != "struct")
    error("Nie struktura przekazana do parse_to_dtd_struct!");

  if(typeof(unq) == T_STRING
     || (sizeof(type) == 2
	 && UNQ_DTD->is_builtin(type[1][0]))) {
    /* Hrm.  The struct had better consist of a single built-in field
       in this case. */
    if(sizeof(type) > 2) {
      accum_error += "Łańcuch '" + unq
	+ "' nie może być sparsowany jako wielokrotne pola dla struktury '" + t_label
	+ "'.\n";
      return nil;
    }

    if(!UNQ_DTD->is_builtin(type[1][0])) {
      accum_error += "Łańcuch '" + unq
	+ "' nie może być sparsowany jako nie wbudowane pole '" + type[1][0]
	+ "' struktury DTD '" + t_label + "'\n";
      return nil;
    }

    tmp = parse_to_string_with_mods(type[1], unq);
    if(tmp == nil) return nil;

    return ({ t_label, tmp });
  }

  /* Set up fields array to know what bucket of instance_tracker
     different labelled fields belong in */ 
  fields = ([ ]);
  bucket_count = set_up_fields_mapping(fields, t_label);

  /* Write fields mapping to log file */
  /* Re-use instance tracker variable for this */
  instance_tracker = map_indices(fields);
  write_log("Tablica pól dla struktury " + t_label + ", " + bucket_count
	    + " wiader.");
  for(ctr = 0; ctr < sizeof(instance_tracker); ++ctr) {
    write_log(fields[instance_tracker[ctr]][0] + " - "
	      + instance_tracker[ctr]);
  }
  write_log("-------");

  /* We need to track how many of each tag in the structure are parsed
     so we can check to make sure they match our modifiers or lack
     thereof.  We have one bucket for each type in the struct, and its
     base classes. */
  instance_tracker = allocate(bucket_count);

  for (ctr = 0; ctr < bucket_count; ++ctr) {
    instance_tracker[ctr] = ({ });
  }

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    int   index;
    mixed tmp;
    string match_label;

    match_label = label = STRINGD->trim_whitespace(unq[ctr]);
    if (fields[match_label] == nil) {
      accum_error += "Nierozpoznane pole " + (label ? label : "(nil)")
	+ " w strukturze\n";
      return nil;
    }

    while (match_label != nil
	   && fields[match_label][0] == nil) {
      match_label = base_type[match_label];
    }

    if (match_label == nil) {
      accum_error += "Nierozpoznane pole " + label + " w strukturze\n";
      return nil;
    }
    index = fields[match_label][0];

    tmp = parse_to_dtd_type(label, unq[ctr + 1]);
    if(tmp == nil) {
      accum_error += "Błąd przy parsowaniu etykiety '" + label
	+ "' struktury '" + t_label + "'.\n";
      return nil;
    }
    instance_tracker[index] += ({ tmp });
    if(instance_tracker[index] == nil)
      return nil;

  }

  vals = map_values(fields);

  /* Verify we match the modifiers */
  for(ctr = 0; ctr < sizeof(vals); ctr++) {
    int num;

    repmin = 0;
    repmax = INT_MAX;
    if (vals[ctr][1] == "?") {
      repmax = 1;
    } else if (vals[ctr][1] == "+") {
      repmin = 1;
    } else if (vals[ctr][1] == "*") {
      /* do nothing, any value is acceptable */
    } else if (sscanf(vals[ctr][1], "<%d..%d>", repmin, repmax) == 2
	       || sscanf(vals[ctr][1], "<..%d>", repmax) == 1
	       || sscanf(vals[ctr][1], "<%d..>", repmin) == 1) {
      /* do nothing, all values already entered */
    } else if (sscanf(vals[ctr][1], "<%d>", repmin) == 1) {
      repmax = repmin;
    } else {
      accum_error += "Nierozpoznany modyfikator typu " + vals[ctr][1] + "!\n";
      return nil;
    }

    num = sizeof(instance_tracker[vals[ctr][0]]);
    if (num < repmin || num > repmax) {
      int ctr2;
      mixed *types;

      types = map_indices(fields);

      for (ctr2 = 0; ctr2 < sizeof(types); ctr2++) {
	/* Compare instance-tracker bucket numbers */
	if (fields[types[ctr2]][0] == vals[ctr][0]) {
	  accum_error += "Zły # pól typu '"
	    + types[ctr2] + "' w strukturze '" + t_label + "'.  " + num
	    + " podany, pomiędzy " + repmin + " i "
	    + (repmax == INT_MAX ? "nieskończoność": repmax)
	    + " wymagany.\n";
	  return nil;
	}
      }

      accum_error += "Zły # pól w instacji wiadra #"
	+ STRINGD->mixed_sprint(vals[ctr][0])
	+ "(nieznany) w strukturze '" + t_label + "'.  "
	+ num + " podany, pomiędzy " + repmin + " i "
	+ (repmax == INT_MAX ? "nieskończoność": repmax)
	+ " wymagany.\n";
	  return nil;
    }
  }

  /* Reassemble instances into single array to return */
  ret = ({ });
  for(ctr = 0; ctr < sizeof(instance_tracker); ctr++) {
    ret += instance_tracker[ctr];
  }
  return ({ t_label, ret });
}


/*** To load in the DTD: ***/

void load(string new_dtd) {
  int    ctr, ctr2;
  string str;
  mixed* new_unq;
  string import;

  if(!is_clone)
    error("Nie mogę używać niesklonowanego UNQ DTD! Zatrzymanie!");

  new_unq = UNQ_PARSER->basic_unq_parse(new_dtd);

  if(!new_unq)
    error("Nie mogę parsować danych UNQ podanych do unq_dtd:load!");

  new_unq = UNQ_PARSER->trim_empty_tags(new_unq);

  for (ctr2 = 0; ctr2 < sizeof(new_unq); ctr2 += 2) {
    str = STRINGD->trim_whitespace(new_unq[ctr2]);
    if (STRINGD->stricmp(str, "dtd") == 0) {
      /* parse DTD */
      if (typeof(new_unq[ctr2 + 1]) != T_ARRAY) {
	error("Nie wygląda dostatecznie skomplikowanie aby być prawdziwym DTD!");
      }

      new_unq[ctr2+1] = UNQ_PARSER->trim_empty_tags(new_unq[ctr2+1]);
      for(ctr = 0; ctr < sizeof(new_unq[ctr2+1]); ctr+=2) {
	new_dtd_element(new_unq[ctr2+1][ctr], new_unq[ctr2+1][ctr + 1]);
      }
	  
    } else if (STRINGD->stricmp(str, "import") == 0) {
      /* import another file */
      if (typeof(new_unq[ctr2 + 1]) != T_STRING) {
	error ("Nazwa pliku musi być string w import!");
      }

      import = read_file(new_unq[ctr2 + 1]);

      if (import == nil) {
	error("Nie mozna załadować pliku " + new_unq[ctr + 1]);
      }
      
      /* recusively call load */
      load(import);
    } else {
      error("Nieznany typ tagu: " + new_unq[ctr2]);
    }
  }
}

private void new_dtd_element(string label, mixed data) {
  string* tmp_arr;
  string inh;

  label = STRINGD->trim_whitespace(label);
  if (sscanf(label, "%s:%s", label, inh) == 1 || inh == "") {
    inh = nil;
  }

  if(dtd[label] || label == "struct" || UNQ_DTD->is_builtin(label))
    error("Ponowna definicja etykiety " + label + " w UNQ DTD!");

  if(typeof(data) == T_STRING) {
    data = STRINGD->trim_whitespace(data);

    tmp_arr = explode(data, ",");
    dtd[label] = dtd_struct(tmp_arr);

    if (inh != nil) {
      if (dtd[inh] != nil) {
	/* Mark down the inheritance relationship */
	base_type[label] = inh;
      } else {
	error("Bazowy typ " + inh + " nie zdefiniowany kiedy parsowano " + label);
      }
    }

    return;
  } else if (typeof(data) == T_ARRAY) {
    if (sizeof(data) != 0) {
      error("rozbudowane subtype jeszcze nie są wspierane!");
    } else {
      if (inh != nil) {
	/* add types allowed for base type of the derived type */
	dtd[label] = dtd[inh];
      } else {
	error("Bazowy typ " + inh + " nie zdefiniowany kiedy parsowano " + label);
      }
    }
  } else {
    error("Błąd typu -- problem z parserem UNQ?");
  }
}

private mixed* dtd_string_with_mods(string str) {
  string mod;

  str = STRINGD->trim_whitespace(str);
  if(str == nil)
    error("Nil przekazany do dtd_string_with_mods!");

  if(STRINGD->is_ident(str))
    return ({ str });

  if (sscanf(str, "%s<%s>", str, mod) == 2) {
    mod = "<" + mod + ">";
    return ({ str, mod });
  }

  mod = str[strlen(str)-1..strlen(str)-1];
  str = str[..strlen(str)-2];
  if(mod == "?" || mod == "*" || mod == "+")
    return ({ str, mod });

  error("Nie mogę rozpoznać modyfikatorów typu w UNQ '" + str + "'");
}

private mixed* dtd_struct(string* array) {
  mixed* ret, *tmp;
  int    ctr;

  ret = ({ "struct" });

  for(ctr = 0; ctr < sizeof(array); ctr++) {
    array[ctr] = STRINGD->trim_whitespace(array[ctr]);
    tmp = dtd_string_with_mods(array[ctr]);
    ret += ({ tmp });
  }

  return ret;
}


void clear(void) {
  dtd = ([ ]);
}


/****************** Helper funcs ***************************/

/* Returns the number of buckets used. */
private int set_up_fields_mapping(mapping fields, string label) {
  int ctr;
  int count;
  mixed *type;

  type = dtd[label];

  /* if there's a base class, add the base class' types */
  if (base_type[label]) {
    count = set_up_fields_mapping(fields, base_type[label]);
  } else {
    count = 0;
  }

  /* Set up fields array to know what bucket of instance_tracker
     different labels belong in */
  for(ctr = 1; ctr < sizeof(type); ++ctr) {
    if(typeof(type[ctr]) == T_STRING) {
      if (fields[type[ctr]] == nil) {
	++count;
      }
      /* <1> in second element says there must be exactly one of these */
      fields[type[ctr]] = ({ count - 1, "<1>" });
    } else if(typeof(type[ctr]) == T_ARRAY) {
      if (fields[type[ctr][0]] == nil) {
	++count;
      }
      if (sizeof(type[ctr]) == 1) {
	fields[type[ctr][0]] = ({ count - 1, "<1>" });
      } else {
	fields[type[ctr][0]] = ({ count - 1, type[ctr][1] });
      }
    } else {
      error("Nieznany typ w strukturze DTD!");
    }
  }

  return count;
}
