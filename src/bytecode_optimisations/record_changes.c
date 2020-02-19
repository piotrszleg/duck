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

void print_transformations(BytecodeManipulation* manipulation){
    for(int i=0; i<vector_count(manipulation->transformations); i++){
        print_transformation(manipulation->output_file, 
            vector_index_instruction(manipulation->instructions, i),
            vector_index_transformation(manipulation->transformations, i));
    }
}

void initialize_recording(BytecodeManipulation* manipulation){
    manipulation->output_file=fopen("output.html", "w");
    fprintf(manipulation->output_file, "<head><link rel=\"stylesheet\" href=\"style.css\"></head><body>");
    fprintf(manipulation->output_file, "<div class=\"panel\">");
    fprintf(manipulation->output_file, "<h1 class=\"title\">Before optimisations</h1>");
    fprintf(manipulation->output_file,   "<pre class=\"code\">");
    print_transformations(manipulation);
    fprintf(manipulation->output_file,   "</pre>");
}
void finish_recording(BytecodeManipulation* manipulation){
    fprintf(manipulation->output_file, "<div class=\"panel\">");
    fprintf(manipulation->output_file, "<h1 class=\"title\">Final</h1>");
    fprintf(manipulation->output_file,   "<pre class=\"code\">");
    print_transformations(manipulation);
    fprintf(manipulation->output_file,   "</pre>");
    fprintf(manipulation->output_file, "</body>");
    fclose(manipulation->output_file);
}

void begin_recording_change(BytecodeManipulation* manipulation, char* name, int* highlighted_lines){
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
void end_recording_change(BytecodeManipulation* manipulation){
    fprintf(manipulation->output_file,   "<pre class=\"after code\">");
    print_transformations(manipulation);
    fprintf(manipulation->output_file,   "</pre>");
    fprintf(manipulation->output_file, "</div>");
}