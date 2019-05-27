#include <string.h>
#include <stdlib.h>

int sum_int(int a, int b){
    return a+b;
}
float sum_float(float a, float b){
    return a+b;
}
char* sum_string(char* a, char* b){
    int resulting_length=strlen(a)+strlen(b);
    char* result=malloc(resulting_length*sizeof(char));
    strncpy(result, a, resulting_length);
    strncat(result, b, resulting_length);
}
void sum_string_to_buffer(char* buffer, int buffer_count, char* a, char* b){
    strncpy(buffer, a, buffer_count);
    strncat(buffer, b, buffer_count);
}
typedef struct {
    int x;
    int y;
} point;
point sum_struct(point a, point b){
    point result={a.x+b.x, a.y+b.y};
    return result;
}
point* sum_struct_pointers(point* a, point* b){
    point* result=malloc(sizeof(point));
    result->x=a->x+b->x;
    result->x=a->y+b->y;
    return result;
}
void sum_struct_pointers_in_place(point* a, point* b){
    a->x+=b->x;
    a->y+=b->y;
}