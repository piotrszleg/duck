#include "table.h"

void table_component_init(Table* t){
    t->elements_count=0;
    t->array_size=INITIAL_ARRAY_SIZE;
    t->map_size=INITIAL_MAP_SIZE;
    t->array=calloc(t->array_size, sizeof(Object));
    t->map=calloc(t->map_size, sizeof(MapElement*));
    t->protected=false;
}

// returns true for string that doesn't start with number and has only letters, numbers and underscores inside
bool is_valid_name(char* s){
    bool first=true;
    if(s[0]=='\0'){
        return false;
    }
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

unsigned table_hash(Executor* E, Table* t, Object* error) {
    unsigned result;
    TableIterator it=table_get_iterator(t);
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        Object field_error=null_const;
        #define ERROR_CHECK(hashed_value) \
            if(field_error.type!=t_null){ \
                NEW_ERROR(*error, "HASH_ERROR", \
                multiple_causes(E, (Object[]){wrap_heap_object((HeapObject*)t), hashed_value, field_error}, 3), \
                "Hashing table failed because of error in hashing one of it's fields."); \
                return result; \
            }
        unsigned key_hash=hash(E, i.key, &field_error);
        ERROR_CHECK(i.key)
        unsigned value_hash=hash(E, i.value, &field_error);
        ERROR_CHECK(i.value)
        // integer overflow is expected here
        result+=key_hash+value_hash;
    }
    return result;
}

Object table_get(Executor* E, Table* t, Object key) {
    if(key.type==t_int) {
        if(key.int_value<t->array_size && key.int_value>=0){
            return t->array[(int)key.int_value];
        }
    }
    Object error=null_const;
    int hashed=hash(E, key, &error)%t->map_size;
    if(error.type!=t_null){
        RETURN_ERROR("GET_ERROR", multiple_causes(E, (Object[]){wrap_heap_object((HeapObject*)t), key, error}, 3), 
                     "Getting a field from table failed because of hashing error.")
    }
    MapElement* e=t->map[hashed];
    while(e){
        if(compare(E, e->key, key)==0){
            return e->value;
        }
        e=e->next;
    }
    return null_const;
}

void table_protect(Table* t){
    t->protected=true;
}

bool table_is_protected(Table* t){
    return t->protected;
}

TableIterator table_get_iterator(Table* iterated){
    TableIterator result={iterated, true, 0, iterated->map[0]};
    return result;
}

IterationResult table_iterator_next(TableIterator* it){
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
        result.key=to_int(it->index);
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

// to avoid code duplication in table_serialize and table_stringify
static char* table_to_text(Executor* E, Table* t, bool serialize) {
    TableIterator it=table_get_iterator(t);
    stream s;
    stream_init(&s, 64);
    bool first=true;
    float last_array_index=-1;
    bool array_part_holey=false;
    int max_hole_size=3;
    
    bool add_whitespace=t->elements_count>3||t->protected;
    if(serialize && t->protected){
        stream_push_const_string(&s, "protect(");
    }
    stream_push_const_string(&s, "[");
    if(add_whitespace){
        stream_push_const_string(&s, "\n\t");
    }
    if(!serialize && t->protected){
        stream_push_const_string(&s, "# protected\n\t");
    }
    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        if(serialize && (!is_serializable(i.key) || !is_serializable(i.value))){
            // skip fields that can't be serialized if serialize is true
            continue;
        }
        if(first){
            first=false;
        } else {
            if(add_whitespace){
                stream_push_const_string(&s, "\n\t");
            } else {
                stream_push_const_string(&s, ", ");
            }
        }
        bool self_reference=i.value.type==t_table && i.value.tp==t;
        char* stringified_value;
        if(self_reference){
            stringified_value="<self>";
        } else {
            stringified_value=stringify(E, i.value);
        }
        // detect if there are holes in array part
        if(i.inside_array && !array_part_holey && i.key.type==t_int){
            float indexes_difference=i.key.int_value-last_array_index;
            if(indexes_difference!=1){
                if(indexes_difference-1<=max_hole_size){
                    for(int i=0; i<indexes_difference-1; i++){
                        stream_push_string_indented(&s, "null");
                        if(add_whitespace){
                            stream_push_const_string(&s, "\n\t");
                        } else {
                            stream_push_const_string(&s, ", ");
                        }
                    }
                } else {
                    array_part_holey=true;
                }
            }
            last_array_index=i.key.int_value;
        }
        if(i.inside_array && !array_part_holey){
            // print the array part without keys
            stream_push(&s, stringified_value, strlen(stringified_value));
        } else if(i.key.type==t_string && is_valid_name(i.key.text)){
            stream_push_string_indented(&s, i.key.text);
            stream_push(&s, "=", 1);
            stream_push_string_indented(&s, stringified_value);
        } else {
            char* stringified_key=stringify(E, i.key);
            stream_push_const_string(&s, "$[");
            stream_push_string_indented(&s, stringified_key);
            stream_push_const_string(&s, "]=");
            stream_push_string_indented(&s, stringified_value);
            free(stringified_key);
        }
        if(!self_reference){
            free(stringified_value);
        }
    }
    if(add_whitespace){
        stream_push_const_string(&s, "\n");
    }
    stream_push_const_string(&s, "]");
    if(serialize && t->protected){
        stream_push_const_string(&s, ")");
    }
    stream_push_const_string(&s, "\0");
    
    return (char*)stream_get_data(&s);
}

char* table_serialize(Executor* E, Table* t) {
    return table_to_text(E, t, true);
}

char* table_stringify(Executor* E, Table* t){
    return table_to_text(E, t, false);
}

Object table_copy(Executor* E, Table* t) {
    Object copied;
    table_init(E, &copied);
    TableIterator it=table_get_iterator(t);

    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        table_set(E, copied.tp, copy(E, i.key), copy(E, i.value));
    }
    return copied;
}

void table_foreach_children(Executor* E, Table* t, ManagedPointerForeachChildrenCallback callback){
    for(int i=0; i<t->array_size; i++){
        callback(E, &t->array[i]);
    }
    for(int i=0; i<t->map_size; i++){
        MapElement* e=t->map[i];
        while(e){
            callback(E, &e->key);
            callback(E, &e->value);
            e=e->next;
        }
    }
}

void table_free(Table* t){
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

int table_compare(Executor* E, Table* a, Table* b, Object* error){
    if(a==b) {
        return 0;
    }
    int difference=a->elements_count-b->elements_count;
    if(difference==0){
        TableIterator it=table_get_iterator(a);
        for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
            Object in_b=table_get(E, b, i.key);
            int compare_result=compare_and_get_error(E, i.value, in_b, error);
            if(error->type!=t_null){
                return 1;
            } else if(compare_result!=0){
                return compare_result;
            }
        }
        return 0;
    } else {
        return sign(difference);
    }
}

static bool array_upsize_allowed(Table* t, int new_size){
    return new_size<t->array_size*8;
}

static bool array_downsize_allowed(Table* t, int new_size){
    return new_size<t->array_size/3 && new_size>=INITIAL_ARRAY_SIZE;
}

static void move_from_map_to_array(Table* t){
  for(int i=0; i<MIN(t->array_size, t->map_size); i++) {
    MapElement* previous=NULL;
    MapElement* e=t->map[i];
    while(e) {
        if(e->key.type==t_int){
            t->array[i]=e->value;
            if(previous!=NULL){
                previous->next=e->next;
                free(e);
            } else {
                t->map[i]=e->next;
                free(e);
            }
            break;
        }
        previous=e;
        e=e->next;
    }
  }
}

// does array part contain elements after the index
static bool elements_after(Table* t, int index){
    for(int i=index+1; i<t->array_size; i++){
        if(t->array[i].type!=t_null){
            return true;
        }
    }
    return false;
}

void table_set(Executor* E, Table* t, Object key, Object value) {
    reference(&value);
    // try to insert value into array
    if(key.type==t_int && key.int_value>=0) {
        int index=(int)key.int_value;
        if(index<t->array_size){
            // key lies within array part
            dereference(E, &t->array[index]);
            if(value.type!=t_null){
                if(t->array[index].type==t_null){
                    t->elements_count++;
                }
                t->array[index]=value;
            } else if (t->array[index].type!=t_null){
                t->elements_count--;
                // reduce the array part size if possible
                int proposed_new_size=nearest_power_of_two(index);
                if(proposed_new_size!=t->array_size && array_downsize_allowed(t, proposed_new_size) && !elements_after(t, index)){
                    t->array_size=proposed_new_size;
                    t->array=realloc(t->array, sizeof(Object)*t->array_size);
                    if(index<t->array_size){// if key still lies inside of the array part set it to null
                        t->array[index]=null_const;
                    }
                    return;
                } else {
                    // simply set array field to null
                    t->array[index]=null_const;
                }
            }
            return;
        } else {
            int proposed_new_size=nearest_power_of_two(index);
            if(array_upsize_allowed(t, proposed_new_size)){
                t->array_size=proposed_new_size;
                t->array=realloc(t->array, sizeof(Object)*t->array_size);
                move_from_map_to_array(t);
                t->array[index]=value;
                return;
            }
        }
    }
    Object error=null_const;
    int hashed=hash(E, key, &error)%t->map_size;
    if(error.type!=t_null){
        destroy_unreferenced(E, &error);
    }
    reference(&key);
    MapElement* previous=NULL;
    MapElement* e=t->map[hashed];
    // traverse linked list
    while(e){
        if(compare(E, e->key, key)==0){
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

Object table_iterator_object_next(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table);
    Object result;
    table_init(E, &result);

    Object iterator_address=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(iterator_address, t_pointer);
    TableIterator* it=(TableIterator*)iterator_address.p;
    IterationResult iterator_result=table_iterator_next(it);
    set(E, result, to_string("key"), iterator_result.key);
    set(E, result, to_string("value"), iterator_result.value);
    set(E, result, to_string("finished"), to_int(iterator_result.finished));
    set(E, result, to_string("inside_array"), to_int(iterator_result.inside_array));
    return result;
}

Object table_iterator_object_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table);
    Object iterator_address=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(iterator_address, t_pointer);
    free((TableIterator*)iterator_address.p);
    return null_const;
}

Object table_get_iterator_object(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object iterator;
    table_init(E, &iterator);
    REQUIRE_TYPE(self, t_table);
    TableIterator* it=malloc(sizeof(TableIterator));
    *it=table_get_iterator(self.tp);
    set(E, iterator, to_int(0), to_pointer(it));
    set(E, iterator, to_string("table"), self);// ensures that table won't be destroyed before the iterator
    set_function_bound(E, iterator, "next", 1, false, table_iterator_object_next);
    set_function_bound(E, iterator, "destroy", 1, false, table_iterator_object_destroy);
    table_protect(iterator.tp);
    return iterator;
}