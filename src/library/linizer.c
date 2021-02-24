#include "./linizer.h"

void line_dump(FILE *stream, const Line *line)
{
    switch (line->kind) {
    case LINE_KIND_INSTRUCTION:
        fprintf(stream, FL_Fmt": INSTRUCTION: name: "SV_Fmt", operand: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_instruction.name),
                SV_Arg(line->value.as_instruction.operand));
        break;
    case LINE_KIND_LABEL:
        fprintf(stream, FL_Fmt": LABEL: name: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_label.name));
        break;
    case LINE_KIND_DIRECTIVE:
        fprintf(stream, FL_Fmt": DIRECTIVE: name: "SV_Fmt", body: "SV_Fmt"\n",
                FL_Arg(line->location),
                SV_Arg(line->value.as_directive.name),
                SV_Arg(line->value.as_directive.body));
        break;
    }
}

bool linizer_from_file(Linizer *linizer, Arena *arena, String_View file_path)
{
    assert(linizer);

    if (arena_slurp_file(arena, file_path, &linizer->source) < 0) {
        return false;
    }

    linizer->location.file_path = file_path;

    return true;
}

bool linizer_peek(Linizer *linizer, Line *output)
{
    // We already have something cached in the peek buffer.
    // Let's just return it.
    if (linizer->peek_buffer_full) {
        *output = linizer->peek_buffer;
        return true;
    }

    // Skipping empty lines
    String_View line = {0};
    do {
        line = sv_trim(sv_chop_by_delim(&linizer->source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));
        linizer->location.line_number += 1;
    } while (line.count == 0 && linizer->source.count > 0);

    // We reached the end. Nothing more to linize.
    if (line.count == 0 && linizer->source.count == 0) {
        return false;
    }

    // Linize
    Line result = {0};
    result.location = linizer->location;
    if (sv_starts_with(line, sv_from_cstr("%"))) {
        sv_chop_left(&line, 1);
        result.kind = LINE_KIND_DIRECTIVE;
        result.value.as_directive.name = sv_trim(sv_chop_by_delim(&line, ' '));
        result.value.as_directive.body = sv_trim(line);
    } else if (sv_ends_with(line, sv_from_cstr(":"))) {
        result.kind = LINE_KIND_LABEL;
        result.value.as_label.name = sv_trim(sv_chop_by_delim(&line, ':'));
    } else {
        result.kind = LINE_KIND_INSTRUCTION;
        result.value.as_instruction.name = sv_trim(sv_chop_by_delim(&line, ' '));
        result.value.as_instruction.operand = sv_trim(line);
    }

    if (output) {
        *output = result;
    }

    // Cache the linize into the peek buffer
    linizer->peek_buffer_full = true;
    linizer->peek_buffer = result;

    return true;
}

bool linizer_next(Linizer *linizer, Line *output)
{
    if (linizer_peek(linizer, output)) {
        linizer->peek_buffer_full = false;
        return true;
    }

    return false;
}

size_t linize_source(String_View source, Line *lines, size_t lines_capacity, File_Location location)
{
    size_t lines_size = 0;
    while (source.count > 0 && lines_size < lines_capacity) {
        String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BASM_COMMENT_SYMBOL));
        location.line_number += 1;

        if (line.count > 0) {
            lines[lines_size].location = location;
            if (sv_starts_with(line, sv_from_cstr("%"))) {
                sv_chop_left(&line, 1);
                lines[lines_size].kind = LINE_KIND_DIRECTIVE;
                lines[lines_size].value.as_directive.name = sv_trim(sv_chop_by_delim(&line, ' '));
                lines[lines_size].value.as_directive.body = sv_trim(line);
            } else if (sv_ends_with(line, sv_from_cstr(":"))) {
                lines[lines_size].kind = LINE_KIND_LABEL;
                lines[lines_size].value.as_label.name = sv_trim(sv_chop_by_delim(&line, ':'));
            } else {
                lines[lines_size].kind = LINE_KIND_INSTRUCTION;
                lines[lines_size].value.as_instruction.name = sv_trim(sv_chop_by_delim(&line, ' '));
                lines[lines_size].value.as_instruction.operand = sv_trim(line);
            }
            lines_size += 1;
        }
    }

    return lines_size;
}
