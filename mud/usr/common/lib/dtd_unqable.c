#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <limits.h>
#include <status.h>

private object dtd;
private string dtd_text;
private string dtd_filename;

private void update_dtd_vars(void);

mixed* to_dtd_unq(void);
void   from_unq(mixed* unq);
void   from_dtd_unq(mixed* unq);

static void create(varargs int clone) {
  if(!find_object(UNQ_PARSER)) { compile_object(UNQ_PARSER); }
  if(!find_object(UNQ_DTD)) { compile_object(UNQ_DTD); }
}

void destructed(int clone) {
  if(dtd)
    destruct_object(dtd);
}

void upgraded(varargs int clone) {
  if(dtd_filename) {
    update_dtd_vars();
  }
}

/* This sets the DTD object given as this object's DTD.  That means
   the object may destruct the DTD at its leisure, among other things. */
void set_dtd(object new_dtd) {
  dtd = new_dtd;
}

/* This supplies the text of a DTD file to be this object's DTD. */
void set_dtd_text(string new_dtd_text) {
  dtd_text = new_dtd_text;
  update_dtd_vars();
}

/* This supplies a filename to load the object's DTD from. */
void set_dtd_file(string new_dtd_filename) {
  dtd_filename = new_dtd_filename;
  update_dtd_vars();
}

/* The updates dtd and dtd_text from themselves and dtd_filename. */
private void update_dtd_vars(void) {
  if(dtd_filename) {
    string buf;

    buf = read_file(dtd_filename);
    if(!buf) {
      error("Nie mogę odczytać pliku DTD " + dtd_filename + "!");
    }
    if(strlen(buf) > status(ST_STRSIZE) - 3) {
      error("Plik DTD jest zbyt duży w funkcji update_dtd_vars!");
    }

    dtd_text = buf;
  }
  if(dtd_text) {
    if(dtd) {
      dtd->clear();
    } else {
      dtd = clone_object(UNQ_DTD);
    }

    dtd->load(dtd_text);
  }
}

/* This method serializes the object as UNQ text and returns the
   appropriate string, suitable for writing to an UNQ file. */
string to_unq_text(void) {
  string result;

  if(!dtd) {
    error("Musisz nadpisać domyślną funkcję to_unq_text dla "
	  + object_name(this_object()) + " albo podać plik DTD!");
  }

  result = dtd->serialize_to_dtd(to_dtd_unq());
  if(!result) {
    LOGD->write_syslog(dtd->get_parse_error_stack(), LOG_WARNING);
  }
  return result;
}


/* Just in case we need it... */
string get_parse_error_stack(void) {
  return dtd->get_parse_error_stack();
}


/* This method converts the object to the same sort of template
   that parsing an UNQ file with a DTD yields.  This template
   can then be converted to text. */
mixed* to_dtd_unq(void) {
  error("Musisz nadpisać domyślną funkcję to_dtd_unq dla "
	+ object_name(this_object()));
}

/* This method takes a string of UNQ text as loads it into the
   object. */
void from_unq_text(string text) {
  from_unq(UNQ_PARSER->basic_unq_parse(text));
}

/* This method takes a chunk of parsed UNQ and loads it into the
   object. */
void from_unq(mixed* unq) {
  mixed* dtd_unq;

  if(!dtd) {
    error("Musisz nadpisać domyślną metodę from_unq dla "
	  + object_name(this_object()) + " albo podać DTD!");
  }

  dtd_unq = dtd->parse_to_dtd(unq);
  if(!dtd_unq)
    error("Nie mogę sparsować UNQ według DTD!");

  from_dtd_unq(dtd_unq);
}


/* This method takes the result of parsing the object according to
   its DTD and loads it into the object. */
void from_dtd_unq(mixed* unq) {
  error("Musisz nadpisać domyślną metodę from_dtd_unq dla "
	+ object_name(this_object()));
}

/* This method loads the necessary file, parses it with the supplied
   DTD, and calls the from_dtd_unq method to load its contents into
   the object. */
void load_from_file(string filename) {
  string str;
  mixed* unq, *dtd_unq;

  str = read_file(filename);
  if(!str)
    error("Nie mogę odczytać pliku '" + filename + "' w load_from_file!");
  if(strlen(str) > status(ST_STRSIZE) - 3)
    error("Plik '" + filename + "' jest zbyt duży w load_from_file!");

  unq = UNQ_PARSER->basic_unq_parse(str);
  if(!unq)
    error("Nie mogę sparsować zawarości pliku do UNQ! (" + filename + ")");

  dtd_unq = dtd->parse_to_dtd(unq);
  if(!dtd_unq) {
    LOGD->write_syslog("Błąd:\n"
		       + dtd->get_parse_error_stack(), LOG_WARN);
    error("Nie mogę sparsować UNQ według DTD!");
  }

  from_dtd_unq(dtd_unq);
}
