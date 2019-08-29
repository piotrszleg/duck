#include "../../object_system/object.h"
#include "../../execution.h"
#define HAVE_STRUCT_TIMESPEC
#include "pthread.h"

/*
TODO:
- shared table
- properly copying functions and coroutines
*/

void set_bound_destructor(Executor* E, Object table, ObjectSystemFunction native_function) {
    table_set(E, table.tp, OVERRIDE(E, destroy), to_bound_function(E, table, 1, false, native_function));
}

typedef struct {
    pthread_mutex_t mutex;
    uint reference_count;
} ReferenceCountedMutex;

#define GET_ZERO_POINTER(variable_type, variable_name) \
    variable_type variable_name; \
    { \
        Object self=arguments[0]; \
        REQUIRE_ARGUMENT_TYPE(self, t_table) \
        Object pointer=table_get(E, self.tp, to_int(0)); \
        REQUIRE_TYPE(pointer, t_pointer) \
        variable_name=(variable_type)pointer.p; \
    }

Object mutex_lock(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(ReferenceCountedMutex*, rc_mutex)
    pthread_mutex_lock(&rc_mutex->mutex);
    return null_const;
}

Object mutex_unlock(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(ReferenceCountedMutex*, rc_mutex)
    pthread_mutex_unlock(&rc_mutex->mutex);
    return null_const;
}

Object mutex_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(ReferenceCountedMutex*, rc_mutex)
    pthread_mutex_lock(&rc_mutex->mutex);
    rc_mutex->reference_count--;
    if(rc_mutex->reference_count==0){
        pthread_mutex_unlock(&rc_mutex->mutex);
        // this was the last object referencing rc_mutex so it can be freed now
        free(rc_mutex);
    }
    pthread_mutex_unlock(&rc_mutex->mutex);
    return null_const;
}

Object mutex_copy(Executor* E, Object scope, Object* arguments, int arguments_count);
Object new_mutex_table(Executor* E, ReferenceCountedMutex* rc_mutex){
    Object result;
    table_init(E, &result);
    table_set(E, result.tp, to_int(0), to_pointer(rc_mutex));
    set_function_bound(E, result, "lock", 1, false, mutex_lock);
    set_function_bound(E, result, "unlock", 1, false, mutex_unlock);
    table_set(E, result.tp, OVERRIDE(E, copy), to_bound_function(E, result, 1, false, mutex_copy));
    set_bound_destructor(E, result, mutex_destroy);
    table_protect(result.tp);
    return result;
}

Object mutex_copy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(ReferenceCountedMutex*, rc_mutex)
    pthread_mutex_lock(&rc_mutex->mutex);
    rc_mutex->reference_count++;
    Object result=new_mutex_table(E, rc_mutex);
    pthread_mutex_unlock(&rc_mutex->mutex);
    return result;
}

Object new_mutex(Executor* E, Object scope, Object* arguments, int arguments_count){
    ReferenceCountedMutex* rc_mutex=malloc(sizeof(ReferenceCountedMutex));
    if (pthread_mutex_init(&rc_mutex->mutex, NULL) != 0)
    {
        RETURN_ERROR("THREAD_ERROR", null_const, "Creating the mutex thread failed")
    } else {
        return new_mutex_table(E, rc_mutex);
    }
}

typedef struct {
    pthread_mutex_t struct_mutex;
    pthread_cond_t condition;
    uint reference_count;
    Object object;
} Channel;

Object channel_receive(Executor* E, Object scope, Object* arguments, int arguments_count) {
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(Channel*, channel)
    pthread_mutex_lock(&channel->struct_mutex);
    pthread_cond_wait(&channel->condition, &channel->struct_mutex);
    Object result=channel->object;
    attach(E, &result);
    pthread_mutex_unlock(&channel->struct_mutex);
    return result;
}

Object channel_send(Executor* E, Object scope, Object* arguments, int arguments_count) {
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(Channel*, channel)
    pthread_mutex_lock(&channel->struct_mutex);
    channel->object=copy(E, arguments[1]);
    detach(E, &channel->object);
    pthread_cond_signal(&channel->condition);
    pthread_mutex_unlock(&channel->struct_mutex);
    return null_const;
}

Object channel_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(Channel*, channel)
    pthread_mutex_lock(&channel->struct_mutex);
    channel->reference_count--;
    if(channel->reference_count==0){
        pthread_mutex_unlock(&channel->struct_mutex);
        // this was the last object referencing rc_mutex so it can be freed now
        free(channel);
    }
    pthread_mutex_unlock(&channel->struct_mutex);
    return null_const;
}

Object channel_copy(Executor* E, Object scope, Object* arguments, int arguments_count);
Object new_channel_table(Executor* E, Channel* channel){
    Object result;
    table_init(E, &result);
    table_set(E, result.tp, to_int(0), to_pointer(channel));
    set_function_bound(E, result, "receive", 1, false, channel_receive);
    set_function_bound(E, result, "send", 2, false, channel_send);
    table_set(E, result.tp, OVERRIDE(E, copy), to_bound_function(E, result, 1, false, channel_copy));
    set_bound_destructor(E, result, channel_destroy);
    table_protect(result.tp);
    return result;
}

Object channel_copy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    GET_ZERO_POINTER(Channel*, channel)
    pthread_mutex_lock(&channel->struct_mutex);
    channel->reference_count++;
    Object result=new_channel_table(E, channel);
    pthread_mutex_unlock(&channel->struct_mutex);
    return result;
}

Object new_channel(Executor* E, Object scope, Object* arguments, int arguments_count) {
    Channel* channel=malloc(sizeof(Channel));
    
    if (pthread_mutex_init(&channel->struct_mutex, NULL) != 0
    ||  pthread_cond_init(&channel->condition, NULL) != 0)
    {
        RETURN_ERROR("THREAD_ERROR", null_const, "Creating the mutex thread failed")
    } else {
        return new_channel_table(E, channel);
    }
}

Object thread_join(Executor* E, pthread_t thread){
    Object* returned_value;
    if(pthread_join(thread, (void**)&returned_value)) {
        RETURN_ERROR("THREAD_ERROR", null_const, "Creating the thread failed")
    } else {
        Object dereferenced=*returned_value;
        free(returned_value);
        attach(E, &dereferenced);
        return dereferenced;
    }
}

Object thread_join_and_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_ARGUMENT_TYPE(self, t_table)
    Object thread=table_get(E, self.tp, to_int(0));
    REQUIRE_TYPE(thread, t_pointer)
    Object argument=table_get(E, self.tp, to_int(1));
    REQUIRE_TYPE(argument, t_pointer)
    if(thread.p!=NULL){
        Object result=thread_join(E, *(pthread_t*)thread.p);
        attach(E, &result);
        free(thread.p);
        table_set(E, self.tp, to_int(0), to_pointer(NULL));
        free(argument.p);
        table_set(E, self.tp, to_int(1), to_pointer(NULL));
        return result;
    } else {
        return null_const;
    }
}

typedef struct
{
    Options* options;
    Object* arguments; 
    int arguments_count;
} ThreadEntryArgument;

void* thread_entry(void* arguments) {
    ThreadEntryArgument* thread_entry=(ThreadEntryArgument*)arguments;
    Executor E;
    executor_init(&E);
    E.options=*thread_entry->options;
    for(int i=0; i<thread_entry->arguments_count; i++){
        attach(&E, &thread_entry->arguments[i]);
    }
    Object* result=malloc(sizeof(Object));
    *result=call(&E, thread_entry->arguments[0], thread_entry->arguments+1, thread_entry->arguments_count-1);
    detach(&E, result);
    executor_deinit(&E);
    return result;
}

Object threads_start(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object* copied_arguments=malloc(sizeof(Object)*(arguments_count));
    for(int i=0; i<arguments_count; i++){
        copied_arguments[i]=copy(E,arguments[i]);
        detach(E, &copied_arguments[i]);
    }
    ThreadEntryArgument* argument=malloc(sizeof(ThreadEntryArgument));
    argument->options=&E->options;
    argument->arguments=copied_arguments;
    argument->arguments_count=arguments_count;

    pthread_t* thread=malloc(sizeof(pthread_t));
    if(pthread_create(thread, NULL, &thread_entry, argument)){
        for(int i=1; i<arguments_count; i++){
            dereference(E, &copied_arguments[i]);
        }
        RETURN_ERROR("THREAD_ERROR", null_const, "Creating the thread failed")
    }

    Object result;
    table_init(E, &result);
    table_set(E, result.tp, to_int(0), to_pointer(thread));
    table_set(E, result.tp, to_int(1), to_pointer(argument));
    set_function_bound(E, result, "join", 1, false, thread_join_and_destroy);
    set_bound_destructor(E, result, thread_join_and_destroy);
    table_protect(result.tp);
    return result;
}

Object duck_module_init(Executor* E){
    Object module;
    table_init(E, &module);
    set_function(E, module, "start", 1, true, threads_start);
    set_function(E, module, "new_mutex", 0, false, new_mutex);
    set_function(E, module, "new_channel", 0, false, new_channel);
    return module;
}