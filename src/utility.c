#include "utility.h"

// source: https://stackoverflow.com/questions/1694036/why-is-the-gets-function-so-dangerous-that-it-should-not-be-used
char* fgets_no_newline(char *buffer, size_t buflen, FILE* fp) {
    if (fgets(buffer, buflen, fp) != 0)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        return buffer;
    } else{ 
        return NULL;
    }
}

char* read_entire_file(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    long file_size=ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer=malloc(file_size+1);
    size_t read_characters=fread(buffer, 1, file_size, fp);

    if(read_characters){
        buffer[read_characters]='\0';
        return buffer;
    } else {
        free(buffer);
        return NULL;
    }
}

uint nearest_power_of_two(uint number){
    unsigned long result=number;
    result--;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    #if INT_MAX>=INT16_MAX
        result |= result >> 8;
            #if INT_MAX>=INT32_MAX
                result |= result >> 16;
                #if INT_MAX>=INT64_MAX
                    result |= result >> 32;
                #endif
        #endif
    #endif
    result++;
    return result;
}

bool strings_counted_equal(char* a, char* b, size_t count){
    for(int i=0; i<count; i++){
        if(a[i]!=b[i]){
            return false;
        }
    }
    return true;
}

int sign(int x){
    return (x > 0) - (x < 0);
}

char* string_replace_multiple(char* original, ReplacementPair* replacement_pairs, int replacement_pairs_count){
    int original_length=strlen(original);
    int new_size=original_length;
    int occurrences_size=16;
    Occurrence* occurrences=malloc(sizeof(Occurrence)*occurrences_size);
    CHECK_ALLOCATION(occurrences)
    int occurrences_count=0;

    for(int i=0; i<original_length; i++){
        for(int p=0; p<replacement_pairs_count; p++){
            if(strings_counted_equal(&original[i], replacement_pairs[p].to_replace, replacement_pairs[p].to_replace_length)){
                if(occurrences_count>=occurrences_size){
                    occurrences_size*=2;
                    occurrences=realloc(occurrences, sizeof(Occurrence)*occurrences_size);
                    CHECK_ALLOCATION(occurrences)
                }
                new_size+=replacement_pairs[p].replacement_length-replacement_pairs[p].to_replace_length;
                Occurrence occurrence={&original[i], p};
                occurrences[occurrences_count]=occurrence;
                occurrences_count++;
                i+=replacement_pairs[p].to_replace_length-1;
            }
        }
    }
    new_size++;// for null terminator
    char* result=malloc(new_size);
    char* result_write=result;
    char* original_read=original;
    
    for(int o=0; o<occurrences_count; o++){
        strncpy(result_write, original_read, occurrences[o].position-original_read);
        result_write+=occurrences[o].position-original_read;
        original_read=occurrences[o].position+replacement_pairs[occurrences[o].pair_index].to_replace_length;
        strcpy(result_write, replacement_pairs[occurrences[o].pair_index].replacement);
        result_write+=replacement_pairs[occurrences[o].pair_index].replacement_length;
    }
    strncpy(result_write, original_read, original_length-(original_read-original));
    result[new_size-1]='\0';
    free(occurrences);
    return result;
}

char* string_add(const char* a, const char* b){
    char* buffer=malloc(strlen(a)+strlen(b)+1);
    CHECK_ALLOCATION(buffer);
    strcpy(buffer, a);
    strcat(buffer, b);
    return buffer;
}

char* string_repeat(const char* str, int times){
    size_t str_length=strlen(str);
    size_t buffer_size=str_length*times+1;
    char* buffer=malloc(buffer_size);
    CHECK_ALLOCATION(buffer);
    
    for(int i=0; i<buffer_size; i++){
        buffer[i]=str[i%str_length];
    }
    buffer[buffer_size-1]='\0';
    return buffer;
}

void* copy_memory(void* source, size_t size){
    void* result=malloc(size);
    CHECK_ALLOCATION(result)
    memcpy(result, source, size);
    return result;
}

void* realloc_zero(void* previous, size_t previous_size, size_t new_size) {
    void* reallocated = realloc(previous, new_size);
    if(reallocated!=NULL){
        size_t difference = new_size - previous_size;
        memset(reallocated, 0, difference);
    }
    return reallocated;
}

// printf variant that returns pointer to formatted string
char* suprintf (const char * format, ...){
    char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
    CHECK_ALLOCATION(buffer);
    int buffer_size=STRINGIFY_BUFFER_SIZE;
    va_list args;

    while(true){
        va_start (args, format);
        int vsprintf_result=vsnprintf (buffer, buffer_size, format, args);
        va_end (args);
        if(vsprintf_result>=buffer_size){
            buffer_size*=2;
            buffer=realloc(buffer, buffer_size*sizeof(char));
            CHECK_ALLOCATION(buffer);
        } else {
            break;
        }
    }
    return buffer;
}

char* strdup_optional(char* str){
    if(str!=NULL)
        return strdup(str);
    else
        return NULL;
}