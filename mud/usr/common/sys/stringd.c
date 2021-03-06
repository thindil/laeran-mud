#include <type.h>

static void create(varargs int clone) {
}


int char_is_whitespace(int char) {
  if((char == '\n') || (char == '\t')
     || char == ' ')
    return 1;

  return 0;
}


int char_to_lower (int char)
{
        if ((char <= 'Z' && char >= 'A'))
                char |= 0x20;

        return char;
}


int char_to_upper (int char)
{
        if ((char <= 'z' && char >= 'a'))
                char &= ~0x20;

        return char;
}


int is_whitespace(string str) {
  int len;
  int iter;

  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(!char_is_whitespace(str[iter]))
      return 0;
  }
  return 1;
}


int is_alpha(string str) {
  int ctr;

  return !!parse_string("regstring = /[a-zA-Z]+/\n"
			+ "notreg = /[^a-zA-Z]+/\n"
			+ "full : regstring", str);
}


int is_alphanum(string str) {
  int ctr;

  return !!parse_string("regstring = /[a-zA-Z0-9]+/\n"
			+ "notreg = /[^a-zA-Z0-9]+/\n"
			+ "full : regstring", str);
}


int is_ident(string str) {
  return !!parse_string("regstring = /[a-zA-Z\-_]+/\n"
			+ "notreg = /[^a-zA-Z\-_]+/\n"
			+ "full : regstring", str);
}


int string_has_char(int char, string str) {
  int len, iter;

  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(str[iter] == char)
      return 1;
  }
  return 0;
}


string trim_whitespace(string str) {
  int start, end;

  if(!str || str == "") return str;
  start = 0;
  end = strlen(str) - 1;
  while((start <= end) && char_is_whitespace(str[start]))
    start ++;
  while((start <= end) && char_is_whitespace(str[end]))
    end--;

  return str[start..end];
}


string trim_leading_whitespace(string str) {
  int start;

  if(!str || str == "") return str;
  start = 0;
  while((start < strlen(str) - 1) && char_is_whitespace(str[start]))
    start ++;

  return str[start..];
}


string trim_trailing_whitespace(string str) {
  int end;

  if(!str || str == "") return str;
  end = strlen(str) - 1;
  while((0 <= end) && char_is_whitespace(str[end]))
    end--;

  return str[..end];
}


string to_lower(string text) {
  int ctr;
  int len;
  string newword;

  newword = text;
  len = strlen(newword);
  for(ctr=0; ctr<len; ctr++) {
    newword[ctr] = char_to_lower(newword[ctr]);
  }
  return newword;
}


string to_upper(string text) {
  int ctr;
  int len;
  string newword;

  newword = text;
  len = strlen(newword);
  for(ctr=0; ctr<len; ctr++) {
    newword[ctr] = char_to_upper(newword[ctr]);
  }
  return newword;
}


/* If s1 is later in alphabetical order, return 1.  If s2 is later,
   return -1.  If neither, return 0. */
int stricmp(string s1, string s2) {
  int tmp1, tmp2, len1, len2;
  int len, iter;

  len1 = strlen(s1);
  len2 = strlen(s2);
  len = len1 > len2 ? len2 : len1;

  for(iter = 0; iter < len; iter++) {
    tmp1 = s1[iter]; tmp2 = s2[iter];
    if(tmp1 <= 'Z' && tmp1 >= 'A')
      tmp1 += 'a' - 'A';
    if(tmp2 <= 'Z' && tmp2 >= 'A')
      tmp2 += 'a' - 'A';

    if(tmp1 > tmp2)
      return 1;
    if(tmp2 > tmp1)
      return -1;
  }

  if(len1 == len2) return 0;
  if(len1 > len2) return 1;
  return -1;
}


/* If s1 is later in alphabetical order, return 1.  If s2 is later,
   return -1.  If neither, return 0. */
int strcmp(string s1, string s2) {
  int len1, len2;
  int len, iter;

  len1 = strlen(s1);
  len2 = strlen(s2);
  len = len1 > len2 ? len2 : len1;

  for(iter = 0; iter < len; iter++) {
    if(s1[iter] > s2[iter])
      return 1;
    if(s2[iter] > s1[iter])
      return -1;
  }

  if(len1 == len2) return 0;
  if(len1 > len2) return 1;
  return -1;
}


string mixed_sprint(mixed data) {
  int    iter;
  string tmp;
  mixed* arr;

  switch(typeof(data)) {
  case T_NIL:
    return "nil";

  case T_STRING:
    return "\"" + data + "\"";

  case T_INT:
    return "" + data;

  case T_FLOAT:
    return "" + data;

  case T_ARRAY:
    if(data == nil) return "array/nil";
    if(sizeof(data) == 0) return "({ })";

    tmp = "({ ";
    for(iter = 0; iter < sizeof(data); iter++) {
      tmp += mixed_sprint(data[iter]);
      if(iter < sizeof(data) - 1) {
	tmp += ", ";
      }
    }
    return tmp + " })";

  case T_MAPPING:
    arr = map_indices(data);
    tmp = "([ ";
    for(iter = 0; iter < sizeof(arr); iter++) {
      tmp += arr[iter] + " => " + mixed_sprint(data[arr[iter]]) + ",";
    }
    return tmp + "])";

  case T_OBJECT:
    return "<Obiekt (" + object_name(data) + ")>";

  default:
    error("Nieznany typ DGD w mixed_sprint");
  }
}

string tree_sprint(mixed data, int indent) {
  string strInd, tmp;
  int iter;

  strInd = "";

  for (iter = 0; iter < indent; ++iter) {
    strInd += " ";
  }
  
  switch (typeof(data)) {
  case T_ARRAY:
    if (data == nil) {
      return strInd + "array/nil";
    }
    if (sizeof(data) == 0) {
      return strInd + "({ })";
    }

    tmp = "";
    for(iter = 0; iter < sizeof(data); iter++) {
      tmp += tree_sprint(data[iter], indent+2);

      if(iter < sizeof(data) - 1) {
	tmp += ",\r\n";
      }
    }

    return strInd + "({ " + tmp[indent+2..] + " })";
  default:
    return strInd + mixed_sprint(data);
  }
}

string unq_escape(string str) {
  string ret;
  int    index;

  if(!str) return nil;

  ret = "";
  for(index = 0; index < strlen(str); index++) {
    switch(str[index]) {
    case '{':
    case '}':
    case '~':
    case '\\':
      ret += "\\";
    default:
      ret += str[index..index];
      break;
    }
  }
  return ret;
}

int prefix_string(string prefix, string str) {
  if(strlen(str) < strlen(prefix))
    return 0;

  if(str[0..strlen(prefix)-1] == prefix) {
    return 1;
  }

  return 0;
}


/* This function takes a list of strings, sorts them alphabetically,
   and returns them.  Uninitialized (nil) strings aren't guaranteed
   to be preserved. */
string* alpha_sort_list(string* list) {
  int     ctr, new_offset, bottom_offset, len;
  string* new_list;

  if(sizeof(list) <= 1)
    return list;

  new_list = ({ });

  bottom_offset = 0;
  while(1) {
    new_offset = bottom_offset;
    while(bottom_offset < sizeof(list) && !list[new_offset]) {
      new_offset++;
      bottom_offset++;
    }

    if(bottom_offset == sizeof(list)) {
      /* Done. */
      return new_list;
    }

    for(ctr = 1; ctr < sizeof(list); ctr++) {
      if(list[ctr] && strcmp(list[ctr], list[new_offset]) < 0) {
	new_offset = ctr;
      }
    }

    if(!list[new_offset]) {
      /* Nothing but nil entries left, so we're done. */
      return new_list;
    }

    /* Okay, new_offset is now the minimum-sized entry */
    new_list += ({ list[new_offset] });
    list[new_offset] = nil;
  }

  /* We shouldn't actually get here */
  error("Wewnętrzny błąd w sortowaniu łańcuchów!");
}


/********** Locale-specific Functions ************/

/* A specific check for English to see if the word "a" or the word "an"
   is a more appropriate article for the following string. */
int should_use_an(string str) {
  int ctr, letter;

  ctr = 0;
  while(strlen(str) > ctr) {
    letter = str[ctr];

    if(letter <= '9' && letter >= '0') {
      /* It's a leading digit.  Only the word for "eight" begins with a
	 vowel sound ("one" is pronouned as "wun"), so... */
      if(letter == '8') return 1;
      return 0;
    }

    if(letter >= 'a' && letter <= 'z') {
      letter += 'A' - 'a';
    }
    if(letter >= 'A' && letter <= 'Z') {
      if(letter == 'A' || letter == 'E' || letter == 'I' || letter == 'O'
	 || letter == 'U') {
	return 1;
      }
      /* For now, assume a leading H or Y acts as a consonant -- not always
	 true, alas */
      return 0;
    }

    /* Assume English-characters that aren't letters or numbers don't
       affect the a-versus-an thing.  Hard to tell if that's true.  But
       it means, for instance, that we'd say "an %ox", "an #$%@$ automobile",
       or "an ^evil^ mage".  Whatever. */
    ctr++;
  }
  return 0;
}
