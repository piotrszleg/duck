#include "Table.h"

void table_component_init(Table* t){
    t->elements_count=0;
    t->array_size=INITIAL_ARRAY_SIZE;
    t->map_size=INITIAL_MAP_SIZE;
    t->array=calloc(t->array_size, sizeof(Object));
    t->map=calloc(t->map_size, sizeof(MapElement*));
}

// returns true for string that doesn't start with number and has only letters, numbers and underscores inside
bool is_valid_name(char* s){
    bool first=true;
    for(char* p=s; *p!='\0'; p++){
        if(!(
            (*p>='a' && *p<='z')
        ||  (*p>='A' && *p<='Z')
        ||  (!first && *p>='0' && *p<='9')
        ||  *p=='_'
        )){
            return false;
        }
        first=false;
    }
    return true;
}

unsigned hash_string(const char *str) {
    unsigned hashed = 5381;
    while (*str) {
      hashed = ((hashed << 5) + hashed) ^ *str++;
    }
    return hashed;
}

unsigned hash(Object o) {
    switch(o.type){
        case t_string:
            return hash_string(o.text);
        case t_number:
            return o.value;
        case t_null:
            return 0;
        case t_table:
            THROW_ERROR(NOT_IMPLEMENTED, "Hashing tables is not implemented yet.");
            return 0;
        case t_function:
            return (unsigned)o.fp;
        default:
            THROW_ERROR(INCORRECT_OBJECT_POINTER, "Incorrect object type passed to hash function.");
            return 0;
    }
}

int compare(Object a, Object b);

Object get_table(Table* t, Object key) {
    if(key.type==t_number) {
        if(key.value<t->array_size && key.value>=0){
            return t->array[(int)key.value];
        }
    }
    int hashed=hash(key)%t->map_size;
    MapElement* e=t->map[hashed];
    while(e){
        if(compare(e->key, key)==0){
            return e->value;
        }
        e=e->next;
    }
    return null_const;
}

TableIterator start_iteration(Table* iterated){
    TableIterator result={iterated, true, 0, iterated->map[0]};
    return result;
}

IterationResult table_next(TableIterator* it){
    IterationResult result;
    Table* iterated=it->iterated;
    result.finished=false;

    if(it->inside_array){
        // skip null fields in array part
        while(it->index<iterated->array_size && iterated->array[it->index].type==t_null){
            it->index++;
        }
        if(it->index>=iterated->array_size){
            // switch from iterating array part to iterating map part
            it->inside_array=false;
            it->index=0;
        }
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
                // map iteration finished
                result.key=null_const;
                result.value=null_const;
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

char* stringify_table(Executor* E, Table* t){
    TableIterator it=start_iteration(t);
    stream s;
    init_stream(&s, 64);
    bool first=true;
    float last_array_index=-1;
    bool array_part_holey=false;
    int max_hole_size=3;
    
    stream_push(&s, "[", 1);
    for(IterationResult i=table_next(&it); !i.finished; i=table_next(&it)) {
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
            stringified_value=stringify(E, i.value);
        }
        // detect if there are holes in array part
        if(i.inside_array && !array_part_holey && i.key.type==t_number){
            float indexes_difference=i.key.value-last_array_index;
            if(indexes_difference!=1){
                if(indexes_difference-1<=max_hole_size){
                    for(int i=0; i<indexes_difference-1; i++){
                        stream_push(&s, "null, ", 6);
                    }
                } else {
                    array_part_holey=true;
                }
            }
            last_array_index=i.key.value;
        }
        if(i.inside_array && !array_part_holey){
            // print the array part without keys
            stream_push(&s, stringified_value, strlen(stringified_value));
        } else if(i.key.type==t_string && is_valid_name(i.key.text)){
            stream_push(&s, i.key.text, strlen(i.key.text));
            stream_push(&s, "=", 1);
            stream_push(&s, stringified_value, strlen(stringified_value));
        } else {
            char* stringified_key=stringify(E, i.key);
            stream_push(&s, "[", 1);
            stream_push(&s, stringified_key, strlen(stringified_key));
            stream_push(&s, "]=", 2);
            stream_push(&s, stringified_value, strlen(stringified_value));
            free(stringified_key);
        }
        if(!self_reference){
            free(stringified_value);
        }
    }
    stream_push(&s, "]\0", 2);
    
    return (char*)stream_get_data(&s);
}

void dereference_children_table(Executor* E, Table* t){
    for(int i=0; i<t->array_size; i++){
        dereference(E, &t->array[i]);
    }
    for(int i=0; i<t->map_size; i++){
        MapElement* e=t->map[i];
        while(e){
            dereference(E, &e->key);
            dereference(E, &e->value);
            e=e->next;
        }
    }
}

void free_table(Table* t){
    free(t->array);
    for(int i=0; i<t->map_size; i++){
         MapElement* e=t->map[i];
        while(e){
            MapElement* next=e->next;
            free(e);
            e=next;
        }
    }
    free(t->map);
    free(t);
}

bool array_upsize_allowed(Table* t, int index){
    return index<t->array_size*8;
}

bool array_downsize_allowed(Table* t, int index){
    return index<t->array_size/3;
}

void move_from_map_to_array(Table* t){
  for(int i=0; i<t->array_size; i++) {
    MapElement* previous=NULL;
    MapElement* e=t->map[i];
    while(e) {
        if(e->key.type==t_number){
            t->array[i]=e->value;
            if(previous!=NULL){
                previous->next=e->next;
                free(e);
            } else {
                free(e);
                t->map[i]=e->next;
            }
            break;
        }
        previous=e;
        e=e->next;
    }
  }
}

// does array part contain elements after the index
bool elements_after(Table* t, int index){
    for(int i=index+1; i<t->array_size; i++){
        if(t->array[i].type!=t_null){
            return true;
        }
    }
    return false;
}

void set_table(Executor* E, Table* t, Object key, Object value) {
    reference(&value);
    // try to insert value into array
    if(key.type==t_number && key.value>=0) {
        int index=(int)key.value;
        if(key.value<t->array_size){
            // key lies within array part
            if(value.type!=t_null){
                if(t->array[index].type==t_null){
                    t->elements_count++;
                }
                t->array[index]=value;
            } else if (t->array[index].type!=t_null){
                t->elements_count--;
                // reduce the array part size if possible
                if(index<t->array_size/2 && array_downsize_allowed(t, index) && !elements_after(t, index)){
                    t->array_size/=2;
                    t->array=realloc(t->array, sizeof(Object)*t->array_size);
                    return;
                } else {
                    // simply set array field to null
                    t->array[index]=null_const;
                }
            }
            return;
        } else if(array_upsize_allowed(t, index)) {
            t->array_size*=2;
            t->array=realloc(t->array, sizeof(Object)*t->array_size);
            move_from_map_to_array(t);
            t->array[index]=value;
            return;
        }
    }
    // insert value into map
    int hashed=hash(key)%t->map_size;
    reference(&key);
    MapElement* previous=NULL;
    MapElement* e=t->map[hashed];
    // traverse linked list
    while(e){
        if(e!=NULL && compare(e->key, key)==0) {
            // there is a MapElement with this key
            dereference(E, &e->value);
            if(value.type==t_null){
                // assigning value to null removes the MapElement from the table
                t->elements_count--;
                if(previous!=NULL){
                    previous->next=e->next;
                } else {
                    t->map[hashed]=NULL;
                }
                dereference(E, &e->key);
                free(e);
            } else {
                e->value=value;
            }
            return;
        }
        previous=e;
        e=e->next;
    }
    // searching for a MapElement with given key failed, create new and insert it
    MapElement* new_element=malloc(sizeof(MapElement));
    new_element->key=key;
    new_element->value=value;
    new_element->next=t->map[hashed];
    t->map[hashed]=new_element;
    t->elements_count++;
}

Object get(Executor* E, Object o, Object key);
Object set(Executor* E, Object o, Object key, Object value);

Object iterator_next_function(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object result_object;
    table_init(E, &result_object);

    Object iterator_address=get(E, self, to_number(0));
    REQUIRE_TYPE(iterator_address, t_number);
    TableIterator* it=(TableIterator*)(intptr_t)iterator_address.value;
    IterationResult result=table_next(it);
    set(E, result_object, to_string("key"), result.key);
    set(E, result_object, to_string("value"), result.value);
    set(E, result_object, to_string("finished"), to_number(result.finished));
    set(E, result_object, to_string("inside_array"), to_number(result.inside_array));
    return result_object;
}

Object destroy_iterator(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object iterator_address=get(E, self, to_number(0));
    REQUIRE_TYPE(iterator_address, t_number);
    free((TableIterator*)(intptr_t)iterator_address.value);
    return null_const;
}

// TODO: protect pointer from overriding from script for safety reasons
Object get_table_iterator(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object iterator;
    table_init(E, &iterator);
    REQUIRE_TYPE(self, t_table);
    TableIterator* it=malloc(sizeof(TableIterator));
    *it=start_iteration(self.tp);
    set(E, iterator, to_number(0), to_number((intptr_t)it));
    set(E, iterator, to_string("Table"), self);
    set(E, iterator, to_string("call"), to_function(E, iterator_next_function, NULL, 1));
    set(E, iterator, to_string("destroy"), to_function(E, destroy_iterator, NULL, 1));
    return iterator;
}