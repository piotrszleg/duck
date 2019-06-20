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

int nearest_power_of_two(int number){
    int i;
    for(i=1; i<=number;i*=2);
    return i;
}

static bool strings_counted_equal(char* a, char* b, int count){
    for(int i=0; i<count; i++){
        if(a[i]!=b[i]){
            return false;
        }
    }
    return true;
}

char* string_replace_multiple(char* original, ReplacementPair* replacement_pairs, int replacement_pairs_count){
    int original_length=strlen(original);
    int new_size=original_length;
    int occurences_size=16;
    Occurence* occurences=malloc(sizeof(Occurence)*occurences_size);
    CHECK_ALLOCATION(occurences)
    int occurences_count=0;

    for(int i=0; i<original_length; i++){
        for(int p=0; p<replacement_pairs_count; p++){
            if(strings_counted_equal(&original[i], replacement_pairs[p].to_replace, replacement_pairs[p].to_replace_length)){
                if(occurences_count>=occurences_size){
                    occurences_size*=2;
                    occurences=realloc(occurences, sizeof(Occurence)*occurences_size);
                    CHECK_ALLOCATION(occurences)
                }
                new_size+=replacement_pairs[p].replacement_length-replacement_pairs[p].to_replace_length;
                Occurence occurence={&original[i], p};
                occurences[occurences_count]=occurence;
                occurences_count++;
                i+=replacement_pairs[p].to_replace_length-1;
            }
        }
    }
    new_size++;// for null terminator
    char* result=malloc(new_size);
    char* result_write=result;
    char* original_read=original;
    
    for(int o=0; o<occurences_count; o++){
        strncpy(result_write, original_read, occurences[o].position-original_read);
        result_write+=occurences[o].position-original_read;
        original_read=occurences[o].position+replacement_pairs[occurences[o].pair_index].to_replace_length;
        strcpy(result_write, replacement_pairs[occurences[o].pair_index].replacement);
        result_write+=replacement_pairs[occurences[o].pair_index].replacement_length;
    }
    strncpy(result_write, original_read, original_length-(original_read-original));
    result[new_size-1]='\0';
    free(occurences);
    return result;
}