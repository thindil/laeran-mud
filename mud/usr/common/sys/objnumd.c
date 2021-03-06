#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

private mapping segments;
private string* owners;
private int     segments_full;
private int     highest_segment;

/* Prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  if(clone) {
    error("Nie można klonować objnumd!");
  }
  segments = ([ ]);
  segments_full = -1;
  highest_segment = -1;

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON())
    owners = ({ MAPD, EXITD, MOBILED });
}

void destructed(varargs int clone) {
  if(SYSTEM()) {

  }
}

private int owner_for_program(string program_name) {
  int owner;

  for(owner = 0; owner < sizeof(owners); owner++) {
    if(program_name == owners[owner])
      return owner;
  }

  return -1;
}

string get_segment_owner(int segment) {
  if(!SYSTEM() && !COMMON())
    return nil;

  if(segments[segment]) {
    return owners[segments[segment][0]];
  } else
    return nil;
}

int get_highest_segment(void) {
  if(!SYSTEM() && !COMMON())
    return -1;

  return highest_segment;
}

private void set_segment_owner(int segment, int owner) {
  if(segment < 0)
    error("Nie można przypisywać ujemnych segmentów w set_segment_owner!");

  if(segments[segment]) {
    segments[segment][0] = owner;
    return;
  }

  /* This location defines segment structure... */
  segments[segment] = ({ owner, ({ }) });

  if(segments_full == segment - 1) {
    while(segments[segments_full + 1])
      segments_full++;
  }
  if(segment > highest_segment)
    highest_segment = segment;
}

int allocate_new_segment(void) {
  int owner;
  int seg;

  owner = owner_for_program(previous_program());
  if(owner == -1)
    error("Nieznany właściciel " + previous_program()
	  + " wywołał allocate_new_segment!");

  seg = segments_full + 1;
  if(get_segment_owner(seg)) {
    error("Wewnętrzny błąd -- próba ponownego przydzielenia segmentu!");
  }

  set_segment_owner(seg, owner);

  return seg;
}

private void unallocate_segment(int segment) {
  error("Nie powinno być używane?");

  if(segments_full >= segment) {
    segments_full = segment - 1;
  }

  if(segment == highest_segment) {
    error("Nie zaimplementowano przeliczanie najwyższego segmentu!");
  }

  segments[segment] = nil;
}

/* Note:  doing an allocate_in_segment with a valid tracking number
   and nil as the object is a quite acceptable way to allocate or
   pre-grow a segment */
void allocate_in_segment(int segment, int tr_num, object obj) {
  int    offs;
  mixed* seg;
  int    owner;

  if(tr_num < 0)
    error("Ujemny numer śledzenia w allocate_in_segment!");
  if(tr_num / 100 != segment)
    error("Śledzony numer nie jest w segmencie w allocate_in_segment!");

  owner = owner_for_program(previous_program());
  if(owner == -1)
    error("Nieznany właściciel " + previous_program()
	  + " wywołał allocate_in_segment!");

  offs = tr_num % 100;
  seg = segments[segment];
  if(!seg) {
    /* Allocate a new segment for caller */
    set_segment_owner(segment, owner);
    seg = segments[segment];
    if(!seg)
      error("Nie można przydzielić segmentu -- dlaczego?");
  }
  if(seg[0] != owner)
    error(owners[owner] + " nie można przydzielić obiektu " + tr_num + " do segmentu "
	  + segment + "!\n"
	  + "Ten segment jest posiadany przez " + owners[seg[0]] + "!");

  if(sizeof(segments[segment][1]) <= offs) {
    seg[1] += allocate(offs - sizeof(seg[1]) + 1);
  }

  if(seg[1][offs])
    error("Ponowne przyznawanie użytego numeru #" + tr_num + " w allocate_in_segment!");

  seg[1][offs] = obj;
}

/* Note: because a destructed object's references will all become
   nil automatically, doing a remove_from_segment is unnecessary
   if the object has already been destructed */
void remove_from_segment(int segment, int tr_num) {
  mixed* seg;
  int    offs;

  seg = segments[segment];
  if(!seg)
    error("Nie można usuwać z nieprzydzielonego segmentu!");

  if(tr_num / 100 != segment)
    error("Nie można usunąć tr_num z innego segmentu!");

  offs = tr_num % 100;

  if(sizeof(seg[1]) <= offs || !seg[1][offs])
    error("Obiekt nie jest w segmencie w remove_from_segment!");

  seg[1][offs] = nil;
}

/* A segment owner calls this function to retrieve an object from its
   own segment. */
object get_object(int tr_num) {
  int    segment, offs;
  int    owner;
  object ret;

  if(tr_num < 0)
    error("Numer śledzony musi być większy od zera!");
  segment = tr_num / 100;
  offs = tr_num % 100;
  owner = segments[segment] ? segments[segment][0] : -1;
  if(owner == -1
     || owners[owner] != previous_program()) {
    return nil;
  }

  if(sizeof(segments[segment][1]) <= offs) {
    return nil;
  }

  ret = segments[segment][1][offs];

  return ret;
}

/* Attempts to allocate a new tracking number in the given
   segment.  If the segment is full, it returns -1. */
int new_in_segment(int segment, object obj) {
  mixed* seg;
  int    ctr;

  seg = segments[segment];
  if(!seg) {
    set_segment_owner(segment, owner_for_program(previous_program()));
    seg = segments[segment];
  }
  if(owners[seg[0]] != previous_program())
    return -1;

  for(ctr = 0; ctr < sizeof(seg[1]); ctr++) {
    if(!seg[1][ctr]) {
      seg[1][ctr] = obj;
      return segment * 100 + ctr;
    }
  }

  if(sizeof(seg[1]) < 100) {
    int offs;

    offs = sizeof(seg[1]);

    seg[1] += allocate(1);
    seg[1][offs] = obj;

    return segment * 100 + offs;
  }

  return -1;
}

/* Returns a list of object numbers in an owned segment.  Caller
   must be the segment owner and the segment must be allocated.
*/
int* objects_in_segment(int segment) {
  int*   objs;
  int    ctr, tr_num;
  mixed* seg;

  seg = segments[segment];
  if(!seg || previous_program() != owners[seg[0]])
    error("Nie można wylistować segmentu " + segment
	  + " ponieważ nie jesteś jego właścielem!");

  objs = ({ });
  for(ctr = 0, tr_num = segment*100; ctr < sizeof(seg[1]); ctr++, tr_num++) {
    if(seg[1][ctr]) {
      objs += ({ tr_num });
    }
  }

  return objs;
}
