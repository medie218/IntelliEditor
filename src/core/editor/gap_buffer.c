#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * GAP BUFFER — INTERNAL
 * ============================================================================ */

static bool gap_buffer_ensure_gap(GapBuffer *gb, size_t needed) {
    size_t gap_size = gb->gap_end - gb->gap_start;
    if (gap_size >= needed) return true;

    size_t new_cap = gb->capacity * 2;
    if (new_cap < gb->capacity + needed + GAP_BUFFER_MIN_GAP_SIZE)
        new_cap = gb->capacity + needed + GAP_BUFFER_MIN_GAP_SIZE;

    char *new_buf = realloc(gb->buf, new_cap);
    if (!new_buf) return false;

    size_t after_len = gb->capacity - gb->gap_end;

    memmove(new_buf + new_cap - after_len,
            new_buf + gb->gap_end,
            after_len);

    gb->gap_end  = new_cap - after_len;
    gb->capacity = new_cap;
    gb->buf      = new_buf;

    return true;
}

static void gap_buffer_move_gap(GapBuffer *gb, size_t pos) {
    if (pos == gb->gap_start) return;

    size_t gap_size = gb->gap_end - gb->gap_start;

    if (pos < gb->gap_start) {
        size_t move_len = gb->gap_start - pos;

        memmove(gb->buf + gb->gap_end - move_len,
                gb->buf + pos,
                move_len);

        gb->gap_start = pos;
        gb->gap_end   = pos + gap_size;

    } else {
        size_t move_len = pos - gb->gap_start;

        memmove(gb->buf + gb->gap_start,
                gb->buf + gb->gap_end,
                move_len);

        gb->gap_start = pos;
        gb->gap_end   = pos + gap_size;
    }
}

/* ============================================================================
 * LIFECYCLE
 * ============================================================================ */

EditorDocument *editor_create(void) {
    EditorDocument *doc = calloc(1, sizeof(EditorDocument));
    if (!doc) return NULL;

    doc->gap.buf = malloc(GAP_BUFFER_INITIAL_SIZE);
    if (!doc->gap.buf) {
        free(doc);
        return NULL;
    }

    doc->gap.capacity  = GAP_BUFFER_INITIAL_SIZE;
    doc->gap.gap_start = 0;
    doc->gap.gap_end   = GAP_BUFFER_INITIAL_SIZE;

    doc->history.top = -1;
    doc->history.redo_top = -1;

    doc->dirty = false;
    doc->filepath = NULL;

    return doc;
}

void editor_destroy(EditorDocument *doc) {
    if (!doc) return;

    free(doc->gap.buf);

    for (int i = 0; i <= doc->history.top; i++) {
        free(doc->history.stack[i].text);
    }

    free(doc->filepath);
    free(doc);
}

/* ============================================================================
 * TEXT OPERATIONS
 * ============================================================================ */

bool editor_insert(EditorDocument *doc, const char *text, size_t length) {
    if (!doc || !text) return false;

    if (length == 0) length = strlen(text);
    if (length == 0) return true;

    if (!gap_buffer_ensure_gap(&doc->gap, length)) return false;

    memcpy(doc->gap.buf + doc->gap.gap_start, text, length);
    size_t pos = doc->gap.gap_start;
    doc->gap.gap_start += length;

    if (doc->history.top < UNDO_STACK_MAX_DEPTH - 1) {
        doc->history.top++;
        doc->history.redo_top = -1;

        Command *cmd = &doc->history.stack[doc->history.top];
        cmd->type = CMD_INSERT;
        cmd->position = pos;
        cmd->length = length;

        cmd->text = malloc(length + 1);
        if (cmd->text) {
            memcpy(cmd->text, text, length);
            cmd->text[length] = 0;
        }
    }

    doc->dirty = true;
    return true;
}

bool editor_delete(EditorDocument *doc, size_t position, size_t count) {
    if (!doc || count == 0) return false;

    size_t len = editor_get_length(doc);
    if (position + count > len) return false;

    gap_buffer_move_gap(&doc->gap, position);

    char *deleted = malloc(count + 1);
    if (deleted) {
        memcpy(deleted, doc->gap.buf + doc->gap.gap_end, count);
        deleted[count] = 0;
    }

    doc->gap.gap_end += count;

    if (deleted && doc->history.top < UNDO_STACK_MAX_DEPTH - 1) {
        doc->history.top++;
        doc->history.redo_top = -1;

        Command *cmd = &doc->history.stack[doc->history.top];
        cmd->type = CMD_DELETE;
        cmd->position = position;
        cmd->length = count;
        cmd->text = deleted;
    } else {
        free(deleted);
    }

    doc->dirty = true;
    return true;
}

void editor_move_cursor(EditorDocument *doc, size_t position) {
    if (!doc) return;
    gap_buffer_move_gap(&doc->gap, position);
}

char *editor_get_text(const EditorDocument *doc) {
    if (!doc) return NULL;

    size_t before = doc->gap.gap_start;
    size_t after  = doc->gap.capacity - doc->gap.gap_end;
    size_t total  = before + after;

    char *res = malloc(total + 1);
    if (!res) return NULL;

    memcpy(res, doc->gap.buf, before);
    memcpy(res + before, doc->gap.buf + doc->gap.gap_end, after);

    res[total] = '\0';
    return res;
}

size_t editor_get_length(const EditorDocument *doc) {
    if (!doc) return 0;
    return doc->gap.capacity - (doc->gap.gap_end - doc->gap.gap_start);
}

/* ============================================================================
 * UNDO / REDO
 * ============================================================================ */

bool editor_undo(EditorDocument *doc) {
    if (!doc || doc->history.top < 0) return false;

    Command *cmd = &doc->history.stack[doc->history.top];

    if (cmd->type == CMD_INSERT) {
        gap_buffer_move_gap(&doc->gap, cmd->position);
        doc->gap.gap_end += cmd->length;

    } else if (cmd->type == CMD_DELETE) {
        gap_buffer_ensure_gap(&doc->gap, cmd->length);
        gap_buffer_move_gap(&doc->gap, cmd->position);

        memcpy(doc->gap.buf + doc->gap.gap_start,
               cmd->text,
               cmd->length);

        doc->gap.gap_start += cmd->length;
    }

    doc->history.redo_top++;
    doc->history.top--;

    return true;
}

bool editor_redo(EditorDocument *doc) {
    if (!doc || doc->history.redo_top < 0) return false;

    Command *cmd = &doc->history.stack[doc->history.top + 1];

    if (cmd->type == CMD_INSERT) {
        gap_buffer_ensure_gap(&doc->gap, cmd->length);
        gap_buffer_move_gap(&doc->gap, cmd->position);

        memcpy(doc->gap.buf + doc->gap.gap_start,
               cmd->text,
               cmd->length);

        doc->gap.gap_start += cmd->length;

    } else if (cmd->type == CMD_DELETE) {
        gap_buffer_move_gap(&doc->gap, cmd->position);
        doc->gap.gap_end += cmd->length;
    }

    doc->history.top++;
    doc->history.redo_top--;

    return true;
}

bool editor_can_undo(const EditorDocument *doc) {
    return doc && doc->history.top >= 0;
}

bool editor_can_redo(const EditorDocument *doc) {
    return doc && doc->history.redo_top >= 0;
}

/* ============================================================================
 * STATS
 * ============================================================================ */

void editor_compute_stats(const EditorDocument *doc, DocStats *stats) {
    if (!doc || !stats) return;

    memset(stats, 0, sizeof(DocStats));

    char *text = editor_get_text(doc);
    if (!text) return;

    int in_word = 0;
    int prev_nl = 0;

    stats->line_count = 1;

    for (size_t i = 0; text[i]; i++) {

        if ((text[i] & 0xC0) != 0x80)
            stats->char_count++;

        if (text[i] == '\n') {
            stats->line_count++;
            if (prev_nl) stats->paragraph_count++;
            prev_nl = 1;
        } else {
            prev_nl = 0;
        }

        if (text[i] == ' ' || text[i] == '\n' || text[i] == '\t') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            stats->word_count++;
        }
    }

    stats->paragraph_count++;

    free(text);
}
