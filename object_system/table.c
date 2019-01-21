#include "table.h"

void table_component_init(table* t){
    t->array_size=INITIAL_ARRAY_SIZE;
    t->map_size=INITIAL_MAP_SIZE;
    t->array=calloc(t->array_size, sizeof(object));
    t->map=calloc(t->map_size, sizeof(map_element*));
}

unsigned hash_string(const char *str) {
    unsigned hashed = 5381;
    while (*str) {
      hashed = ((hashed << 5) + hashed) ^ *str++;
    }
    return hashed;
}

unsigned hash(object o) {
    switch(o.type){
        case t_string:
            return hash_string(o.text);
        case t_number:
            return o.value;
        case t_null:
            return 0;
        case t_table:
            ERROR(NOT_IMPLEMENTED, "Hashing tables is not implemented yet.");
            return 0;
        case t_function:
            return (unsigned)o.fp;
        default:
            ERROR(INCORRECT_OBJECT_POINTER, "Incorrect object type passed to hash function.");
            return 0;
    }
}

int compare(object a, object b);

object get_table(table* t, object key) {
    if(key.type==t_number) {
        if(key.value<t->array_size && key.value>=0){
            return t->array[(int)key.value];
        }
    }
    int hashed=hash(key)%t->map_size;
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

table_iterator start_iteration(table* iterated){
    table_iterator result={iterated, true, 0, iterated->map[0]};
    return result;
}

iteration_result table_next(table_iterator* it){
    iteration_result result;
    table* iterated=it->iterated;
    result.finished=false;

    if(it->inside_array && (it->index>=iterated->array_size || iterated->array[it->index].type==t_null) ){
        // switch from iterating array part to iterating map part
        it->inside_array=false;
        it->index=0;
    }
    result.inside_array=it->inside_array;

    if(it->inside_array) {
        // iterate array part
        result.inside_array=true;
        result.key=to_number(it->index);
        result.value=it->iterated->array[it->index++];
    } else {
        // skip empty map_elements
        while(it->element==NULL){
            it->index++;
            it->element=iterated->map[it->index];
            if(it->index>=iterated->map_size){
                result.finished=true;
                return result;
            }
        }
        result.key=it->element->key;
        result.value=it->element->value;
        it->element=it->element->next;// traverse linked list
    }
    return result;
}

char* stringify_kvp(const char* key, const char* value){// kvp = key value pair
    char* buf=malloc(32);// TODO: resizing buffer
    snprintf(buf, 32, "%s=%s", key, value);
    return buf;
}

char* stringify_table(table* t){
    table_iterator it=start_iteration(t);
    stream s;
    init_stream(&s, 64);
    bool first=true;
    
    stream_push(&s, "[", 1);
    for(iteration_result i=table_next(&it); !i.finished; i=table_next(&it)) {
        if(first){
            first=false;
        } else {
            stream_push(&s, ", ", 2);
        }
        bool self_reference=i.value.type==t_table && i.value.tp==t;
        char* stringified_value;
        if(self_reference){
            stringified_value="<self>";
        } else {
            stringified_value=stringify(i.value);
        }
        if(i.inside_array){
            USING_STRING(stringified_value,
                stream_push(&s, str, strlen(str)));
        } else {
            char* stringified_key=stringify(i.key);
            USING_STRING(stringify_kvp(stringified_key, stringified_value),
                stream_push(&s, str, strlen(str)));
            free(stringified_key);
        }
        if(!self_reference){
            free(stringified_value);
        }
    }
    stream_push(&s, "]", 2);// push ] with null terminator
    
    return (char*)s.data;
}

void delete_table(table* t){
    for(int i=0; i<t->array_size; i++){
        dereference(&t->array[i]);
    }
    free(t->array);
    for(int i=0; i<t->map_size; i++){
        map_element* e=t->map[i];
        while(e){
            dereference(&e->key);
            dereference(&e->value);
            map_element* next=e->next;
            free(e);
            e=next;
        }
    }
    free(t->map);

    free(t);
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
    int hashed=hash(key)%t->map_size;
    reference(&key);
    map_element* e=t->map[hashed];
    while(e){
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
        e->next=t->map[hashed];
        t->map[hashed]=e;
    }
}