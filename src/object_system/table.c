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
        unsigned key_hash=hash_and_get_error(E, i.key, &field_error);
        ERROR_CHECK(i.key)
        unsigned value_hash=hash_and_get_error(E, i.value, &field_error);
        ERROR_CHECK(i.value)
        // integer overflow is expected here
        result+=key_hash+value_hash;
    }
    return result;
}

Object table_get_with_hashing_error(Executor* E, Table* t, Object key, Object* error){
    if(key.type==t_int) {
        if(key.int_value<t->array_size && key.int_value>=0){
            return t->array[(int)key.int_value];
        }
    }
    Object hashing_error=null_const;
    uint hashed=hash_and_get_error(E, key, &hashing_error)%t->map_size;
    if(hashing_error.type!=t_null){
        NEW_ERROR(*error, "GET_ERROR", multiple_causes(E, (Object[]){wrap_heap_object((HeapObject*)t), key, hashing_error}, 3), 
                     "Getting a field from table failed because of hashing error.")
        return null_const;
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

/* Returns value in table with unchanged reference count or null constant.
   If you want to do anything with it other than reading it's memory you need to reference it. 
   Dismisses eventual hashing error, it'll be passed to executor_on_unhandled_error function. */
Object table_get(Executor* E, Table* t, Object key) {
    Object error=null_const;
    Object result=table_get_with_hashing_error(E, t, key, &error);
    if(error.type!=t_null){
        dereference(E, &error);
    }
    return result;
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
            it->map_element=iterated->map[it->index];
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
        while(it->map_element==NULL){
            it->index++;
            if(it->index>=iterated->map_size){
                // map iteration finished
                result.key=null_const;
                result.value=null_const;
                result.finished=true;
                return result;
            } else {
                it->map_element=iterated->map[it->index];
            }
        }
        result.key=it->map_element->key;
        result.value=it->map_element->value;
        it->map_element=it->map_element->next;// traverse linked list
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
    uint max_hole_size=3;
    
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
                // fill in the hole inside of array with nulls if the hole is small enough
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
                    // if the hole is too big display following elements with .[key]=value syntax instead
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
            stream_push_const_string(&s, ".[");
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

Object table_copy(Executor* E, Table* t, CopyingState* state){
    Object copied;
    table_init(E, &copied);
    TableIterator it=table_get_iterator(t);

    #define CHECK_COPIES(before_return) \
    { \
        /*Object copies_field=table_get(E, state->copies, to_pointer(t)); \
        if(copies_field.type==t_pointer){ \
            Object existing_copy=wrap_heap_object((HeapObject*)copies_field.p); \
            dereference(E, &copied); \
            before_return \
            return existing_copy; \
        }*/ \
    }

    CHECK_COPIES()

    for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
        Object copied_key=copy_recursive(E, i.key, state);
        if(is_error(E, copied_key)){
            dereference(E, &copied);
            RETURN_ERROR("COPY_ERROR", MULTIPLE_CAUSES(wrap_heap_object((HeapObject*)t), copied_key), "Copying table failed, because copying one of it's keys failed.");
        }
        CHECK_COPIES(
            dereference(E, &copied_key);
        )
        Object copied_value=copy_recursive(E, i.value, state);
        if(is_error(E, copied_value)){
            dereference(E, &copied_key);
            dereference(E, &copied);
            RETURN_ERROR("COPY_ERROR", MULTIPLE_CAUSES(wrap_heap_object((HeapObject*)t), copied_value), "Copying table failed, because copying one of it's values failed.");
        }
        CHECK_COPIES(
            table_set(E, existing_copy.tp, copied_key, copied_value);
            dereference(E, &copied_key); 
            dereference(E, &copied_value);
        )
        table_set(E, copied.tp, copied_key, copied_value);
    }
    return copied;
}

void table_foreach_children(Executor* E, Table* t, ForeachChildrenCallback callback, void* data){
    for(int i=0; i<t->array_size; i++){
        callback(E, &t->array[i], data);
    }
    for(int i=0; i<t->map_size; i++){
        MapElement* e=t->map[i];
        while(e){
            callback(E, &e->key, data);
            callback(E, &e->value, data);
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
    return new_size<t->array_size*4;
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

// tru if elements still lies within the array
void try_downsizing_array(Table* t, int index){
    uint proposed_new_size=nearest_power_of_two(index);
    if(proposed_new_size!=t->array_size 
        && array_downsize_allowed(t, proposed_new_size) && !elements_after(t, index)
    ){
        t->array_size=proposed_new_size;
        t->array=realloc(t->array, sizeof(Object)*t->array_size);
    }
}

static bool remove_element_from_array_part(Executor* E, Table* t, Object key){
    if(key.type==t_int && key.int_value>=0) {
        int index=(int)key.int_value;
        if(index<t->array_size){
            t->elements_count--;
            dereference(E, &t->array[index]);
            t->array[index]=null_const;
            if(index>t->array_size/2){
                // this might be the last element of the array part
                try_downsizing_array(t, index);
            }
            return true;
        }
    }
    return false;
}

static void remove_element_from_map_part(Executor* E, Table* t, Object key){
    int hashed=hash(E, key)%t->map_size;
    MapElement* previous=NULL;
    MapElement* e=t->map[hashed];
    // traverse linked list
    while(e){
        if(compare(E, e->key, key)==0){
            dereference(E, &e->key);
            dereference(E, &e->value);
            if(previous!=NULL){
                previous->next=e->next;
            } else {
                t->map[hashed]=e->next;
            }
            free(e);
            t->elements_count--;
            return;
        }
        previous=e;
        e=e->next;
    }
}

static void remove_element(Executor* E, Table* t, Object key){
    if(!remove_element_from_array_part(E, t, key)){
        remove_element_from_map_part(E, t, key);
    }
}

bool try_upsizing_array_part(Table* t, int index){
    return false;
    uint proposed_new_size=nearest_power_of_two(index);
    if(array_upsize_allowed(t, proposed_new_size)){
        t->array=realloc_zero(t->array, t->array_size*sizeof(Object), proposed_new_size*sizeof(Object));
        t->array_size=proposed_new_size;
        move_from_map_to_array(t);
        return true;
    }
    return false;
}

static bool try_changing_element_in_array_part(Executor* E, Table* t, Object key, Object value){
    if(key.type==t_int && key.int_value>=0) {
        int index=(int)key.int_value;
        if(index<t->array_size){
            // key lies within array part
            if(t->array[index].type!=t_null){
                dereference(E, &t->array[index]);
            } else {
                t->elements_count++;
            }
            t->array[index]=value;
            return true;
        }  else if(try_upsizing_array_part(t, index)){
            t->elements_count++;
            t->array[index]=value;
            return true;
        }
    }
    return false;
}

static MapElement* new_map_element(Object key, Object value, MapElement* next){
    MapElement* new_element=malloc(sizeof(MapElement));
    new_element->key=key;
    new_element->value=value;
    new_element->next=next;
    return new_element;
}

static void change_element_in_map_part(Executor* E, Table* t, Object key, Object value){
    reference(&key);
    uint hashed=hash(E, key)%t->map_size;
    MapElement* e=t->map[hashed];
    // traverse linked list
    while(e){
        if(compare(E, e->key, key)==0){
            // there is a MapElement with this key
            dereference(E, &e->value);
            dereference(E, &e->key);
            e->key=key;
            e->value=value;
            return;
        }
        e=e->next;
    }
    // searching for a MapElement with given key failed, create new and insert it
    t->map[hashed]=new_map_element(key, value, t->map[hashed]);
    t->elements_count++;
}

static void change_element(Executor* E, Table* t, Object key, Object value){
    if(!try_changing_element_in_array_part(E, t, key, value)){
        change_element_in_map_part(E, t, key, value);
    }
}

void table_set(Executor* E, Table* t, Object key, Object value) {
    if(value.type==t_null){
        remove_element(E, t, key);
    } else {
        reference(&value);
        change_element(E, t, key, value);
    }
}

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
    table_set(E, result.tp, to_string("key"), iterator_result.key);
    table_set(E, result.tp, to_string("value"), iterator_result.value);
    table_set(E, result.tp, to_string("finished"), to_int(iterator_result.finished));
    table_set(E, result.tp, to_string("inside_array"), to_int(iterator_result.inside_array));
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
    table_set(E, iterator.tp, to_int(0), to_pointer(it));
    table_set(E, iterator.tp, to_string("table"), self);// ensures that table won't be destroyed before the iterator
    set_function_bound(E, iterator, to_string("next"), 1, false, table_iterator_object_next);
    set_function_bound(E, iterator, OVERRIDE(E, destroy), 1, false, table_iterator_object_destroy);
    table_protect(iterator.tp);
    return iterator;
}