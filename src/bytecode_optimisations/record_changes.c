#include "record_changes.h"

/*
begin_recording("Removing local variable")
highlight important instructions
before | after
end_recording

<div class="change">
<h1 class="title"></h1>
<div class="before"></div>
<div class="after"><div>
</div>

*/

const char* CHANGE_TYPE_NAMES[]={
    "add",
    "remove",
    "change"
};

static void print_dummies(FILE* output, Dummy** dummies, uint dummies_count){
    if(dummies_count>1){
        fprintf(output, "(");
    }
    for(int i=0; i<dummies_count; i++){
        if(i!=0){
            fprintf(output, ", ");
        }
        dummy_print(output, dummies[i]);
        IF_DEBUGGING(fprintf(output, "#%i", dummies[i]->mp.hp.ref_count))
    }
    if(dummies_count>1){
        fprintf(output, ")");
    }
}

static void print_transformation(FILE* output, Instruction* instruction, Transformation* transformation){
    fprintf(output, "%s ", INSTRUCTION_NAMES[instruction->type]);
    print_dummies(output, transformation->inputs, transformation->inputs_count);
    fprintf(output, "->");
    print_dummies(output, transformation->outputs, transformation->outputs_count);
    fprintf(output, "\n");
}

static void print_transformations(BytecodeManipulation* manipulation){
    for(int i=0; i<vector_count(manipulation->transformations); i++){
        print_transformation(manipulation->output_file, 
            vector_index_instruction(manipulation->instructions, i),
            vector_index_transformation(manipulation->transformations, i));
    }
}

void initialize_recording(BytecodeManipulation* manipulation){
    vector_init(&manipulation->changes, sizeof(Change), 4);
    USING_STRING(suprintf("output/%#x.html",  manipulation->program),
        manipulation->output_file=fopen(str, "w"));
    fprintf(manipulation->output_file, "<head><link rel=\"stylesheet\" href=\"style.css\"></head><body>");
    fprintf(manipulation->output_file, "<div class=\"panel\">");
    fprintf(manipulation->output_file, "<h1 class=\"title\">Before optimisations</h1>");
    fprintf(manipulation->output_file,   "<pre class=\"code\">");
    print_transformations(manipulation);
    fprintf(manipulation->output_file,   "</pre>");
    fprintf(manipulation->output_file, "</div>");
}
void finish_recording(BytecodeManipulation* manipulation){
    fprintf(manipulation->output_file, "<div class=\"panel\">");
    fprintf(manipulation->output_file, "<h1 class=\"title\">Final</h1>");
    fprintf(manipulation->output_file,   "<pre class=\"code\">");
    print_transformations(manipulation);
    fprintf(manipulation->output_file,   "</pre>");
    fprintf(manipulation->output_file, "</div>");
    fprintf(manipulation->output_file, "</body>");
    fclose(manipulation->output_file);
    vector_deinit(&manipulation->changes);
}

void begin_recording_change(BytecodeManipulation* manipulation, char* name, int* highlighted_lines){
    vector_clear(&manipulation->changes);
    fprintf(manipulation->output_file, "<div class=\"change panel\">");
    fprintf(manipulation->output_file,   "<h1 class=\"title\">%s</h1>", name);
    fprintf(manipulation->output_file,   "<pre class=\"before code\">");
    for(int i=0; i<vector_count(manipulation->transformations); i++){
        bool highlight_line=false;
        if(highlighted_lines!=NULL){
            for(int j=0; highlighted_lines[j]!=-1; j++){
                if(highlighted_lines[j]==i){
                    highlight_line=true;
                }
            }
        }
        if(highlight_line){
            fprintf(manipulation->output_file, "<span class=\"highlighted\">");
        }
        print_transformation(manipulation->output_file, 
            vector_index_instruction(manipulation->instructions, i),
            vector_index_transformation(manipulation->transformations, i));
        if(highlight_line){
            fprintf(manipulation->output_file, "</span>");
        }
    }
    fprintf(manipulation->output_file,   "</pre>");
}
void add_change(BytecodeManipulation* manipulation, ChangeType type, uint line){
    Change change={.type=type, .line=line};
    vector_push(&manipulation->changes, &change);
}
void end_recording_change(BytecodeManipulation* manipulation){
    fprintf(manipulation->output_file,   "<pre class=\"after code\">");
    for(int i=0; i<vector_count(manipulation->transformations); i++){
        const char* change_type=NULL;
        for(int j=0; j<vector_count(&manipulation->changes); j++){
            Change* change=vector_index(&manipulation->changes, j);
            if(change->line==i){
                change_type=CHANGE_TYPE_NAMES[change->type];
            }
        }
        if(change_type!=NULL){
            fprintf(manipulation->output_file, "<span class=\"%s\">", change_type);
        }
        print_transformation(manipulation->output_file, 
            vector_index_instruction(manipulation->instructions, i),
            vector_index_transformation(manipulation->transformations, i));
        if(change_type!=NULL){
            fprintf(manipulation->output_file, "</span>");
        }
    }
    fprintf(manipulation->output_file,   "</pre>");
    fprintf(manipulation->output_file, "</div>");
}