#include "table.h"

/*int hash(object* o)
void set(object t, object key, object value)
object get(object t, object key)*/

void table_component_init(table* t){
    t->array_size=INITIAL_ARRAY_SIZE;
    t->map_size=INITIAL_MAP_SIZE;
    t->array=calloc(t->array_size, sizeof(object));
    t->map=calloc(t->map_size, sizeof(map_element*));
}

int compare(object a, object b);
int hash(object o, int hash_max) {
    return 0;
}

object get_table(table* t, object key) {
    if(key.type==t_number) {
        if(key.value<t->array_size && key.value>=0){
            return t->array[(int)key.value];
        }
    }
    int hashed=hash(key, t->map_size);
    map_element* e=t->map[hashed];
    while(e){
        if(compare(e->key, key)==0){
            return e->value;
        }
        e=e->next;
    }
    return null_const;
}

bool array_resize_allowed(table* t, int index){
    return index<t->array_size*8;
}

void move_from_map_to_array(table* t, int max_index){
  for(int i=0; i<max_index; i++) {
    map_element* previous=NULL;
    for(map_element* e=t->map[i]; e->next!=NULL; e=e->next){
        if(e->key.type==t_number){
            t->array[i]=e->value;
            if(previous!=NULL){
                previous->next=e->next;
            } else {
                t->map[i]=e->next;
            }
            free(t->map+i);
            t->map[i]=NULL;
        }
        previous=e;
    }
  }
}

void set_table(table* t, object key, object value) {
    reference(&value);
    // try to insert value into array
    if(key.type==t_number && key.value>=0) {
        if(key.value<t->array_size){
            t->array[(int)key.value]=value;
            return;
        } else if(array_resize_allowed(t, (int)key.value)) {
            t->array_size*=2;
            t->array=realloc(t->array, sizeof(object)*t->array_size);// move indexes from map
            t->array[(int)key.value]=value;
            return;
        }
    }
    // insert value into map
    int hashed=hash(key, t->map_size);
    reference(&key);
    map_element* e=t->map[hashed];
    while( e){
        if(e!=NULL && compare(e->key, key)==0) {
            // there is a map_element with this key, so we replace it's value
            dereference(&e->value);
            e->value=value;
            return;
        }
        e=e->next;
    }
    {
        map_element* e=malloc(sizeof(map_element));
        e->key=key;
        e->value=value;
        e->hash=hashed;
        e->next=t->map[hashed];
        t->map[hashed]=e;
    }
}