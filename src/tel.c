#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tel.h"

struct Line;

struct Buffer {
    char *filename;
    FILE *fp;
    unsigned int line, col;
    struct Line {
        struct Line *next;
        char *data;
        unsigned int size;
    } *head, *focus;
};

enum {
    LEFT, RIGHT, UP, DOWN
};

static int tel_buffer_slurp(struct Buffer *buffer);
static int tel_buffer_refocus(struct Buffer *buffer);
static void tel_buffer_freebuf(struct Buffer *buffer);

struct Buffer *tel_buffer_open(char *name, int *error, unsigned int line, unsigned int col) {
    int slurp_exit = 0, len = strlen(name) + 1;
    struct Buffer *buffer = NULL;

    if (!(buffer = malloc(sizeof(struct Buffer)))) {
        *error = TEL_ERROR_MALLOC_FAILED;
        return NULL;
    }

    buffer->line = (line > 0) ? line : 1;
    buffer->col = (col > 0) ? col : 1;
    buffer->focus = buffer->head = NULL;

    if (!(buffer->filename = malloc((sizeof(char) * len)))) {
        *error = TEL_ERROR_MALLOC_FAILED;
    } else if (!memcpy(buffer->filename, name, len)) {
        *error = TEL_ERROR_STRDUP_FAILED;
    } else if (!(buffer->fp = fopen(buffer->filename, "r+"))) {
        *error = TEL_ERROR_OPEN_READ;
    } else if ((slurp_exit = tel_buffer_slurp(buffer))) {
        *error = slurp_exit;
    }

    if (*error) {
        tel_buffer_close(buffer);
        return NULL;
    }

    return buffer;
}

static void tel_buffer_freebuf(struct Buffer *buffer) {
    struct Line *next = NULL;
    while (buffer->head) {
        next = buffer->head->next;
        free(buffer->head->data);
        free(buffer->head);
        buffer->head = next;
    }
}

void tel_buffer_close(struct Buffer *buffer) {
    tel_buffer_freebuf(buffer);
    if (buffer->fp) fclose(buffer->fp);
    free(buffer->filename);
    free(buffer);
}

int tel_buffer_save(struct Buffer *buffer) {
    struct Line *scan = buffer->head;

    if (fclose(buffer->fp) || !(buffer->fp = fopen(buffer->filename, "w")))
        return TEL_ERROR_OPEN_WRITE;

    while (scan) {
        unsigned int count = 0;
        while (scan->data[count]) count++;
        if (!scan->next && scan->data[0] == LINE_FEED) break;
        fwrite(scan->data, sizeof(char), count, buffer->fp);
        scan = scan->next;
    }

    if (fclose(buffer->fp) || !(buffer->fp = fopen(buffer->filename, "r+")))
        return TEL_ERROR_OPEN_READ;

    return TEL_SUCCESS;
}

static int tel_buffer_slurp(struct Buffer *buffer) {
    struct Line *prev = NULL;
    long start = 0L;
    int ch = 0;

    if (buffer->head) return TEL_ERROR_BUFFER_ALLOCATED;
    if (fseek(buffer->fp, start, SEEK_SET)) return TEL_ERROR_FSEEK_FAILED;

    while (ch != EOF) {
        unsigned int i = 0;

        if (!(buffer->focus = malloc(sizeof(struct Line))))
            return TEL_ERROR_MALLOC_FAILED;

        if (!buffer->head) buffer->head = buffer->focus;
        if (prev) prev->next = buffer->focus;
        buffer->focus->size = 0;
        buffer->focus->data = NULL;
        buffer->focus->next = NULL;

        start = ftell(buffer->fp);
        while ((ch = fgetc(buffer->fp)) != LINE_FEED && ch != EOF)
            buffer->focus->size++;

        if (fseek(buffer->fp, start, SEEK_SET)) {
            tel_buffer_freebuf(buffer);
            return TEL_ERROR_FSEEK_FAILED;
        }

        buffer->focus->size += LINE_EXTRA + 1;

        if (!(buffer->focus->data = malloc(sizeof(char) * buffer->focus->size))) {
            tel_buffer_freebuf(buffer);
            return TEL_ERROR_MALLOC_FAILED;
        }

        for (i = 0; (ch = fgetc(buffer->fp)) != LINE_FEED && ch != EOF; i++)
            buffer->focus->data[i] = ch;

        buffer->focus->data[i++] = LINE_FEED;
        buffer->focus->data[i] = '\0';
        prev = buffer->focus;
    }

    return tel_buffer_refocus(buffer);
}

static int tel_buffer_refocus(struct Buffer *buffer) {
    unsigned int left = buffer->line;

    buffer->focus = buffer->head;
    while (--left && buffer->focus) buffer->focus = buffer->focus->next;

    if (!buffer->focus) return TEL_ERROR_LINE_OOB;

    return TEL_SUCCESS;
}

/* TODO */
int tel_buffer_move(struct Buffer *buffer, int arrow) {
    if (arrow == LEFT) {
        if (buffer->col > 1) {
            buffer->col--;
        } else if (buffer->line > 1) {
            buffer->line--;
            if (tel_buffer_refocus(buffer)) return 1;
            buffer->col = 1;
            while (buffer->focus->data[buffer->col]) buffer->col++;
        }
    } else if (arrow == RIGHT) {
        if (buffer->focus->data[buffer->col]) {
            buffer->col++;
        } else if (buffer->focus->next) {
            buffer->line++;
            buffer->col = 1;
            buffer->focus = buffer->focus->next;
        }
    } else if (arrow == UP) {
        if (buffer->line > 1) {
            int saved_col = buffer->col;

            buffer->line--;
            if (tel_buffer_refocus(buffer)) return 1;
            buffer->col = 0;
            while (buffer->focus->data[buffer->col] && saved_col > buffer->col) buffer->col++;
        }
    } else if (arrow == DOWN) {
        if (buffer->focus->next) {
            int saved_col = buffer->col;

            buffer->line++;
            buffer->focus = buffer->focus->next;
            buffer->col = 0;
            while (buffer->focus->data[buffer->col] && saved_col > buffer->col) buffer->col++;
        }
    } else {
        return 2;
    }

    return TEL_SUCCESS;
}

/* TODO */
int tel_buffer_insert(struct Buffer *buffer, char ch) {
    /*
    unsigned int pos = 0;

    if (ch <= 0) return 1;

    while (buffer->focus->data[pos]) pos++;
    pos++;

    if (pos >= buffer->focus->size) {
        int i = 0;
        char *new_data = NULL;
        unsigned int new_size = buffer->focus->size + LINE_EXTRA;

        if (!(new_data = malloc(sizeof(char) * new_size))) return 2;

        for (i = 0; i < new_size + 1; i++)
            new_data[i] = buffer->focus->data[i];

        free(buffer->focus->data);
        buffer->focus->data = new_data;
        buffer->focus->size = new_size;
    }

    while (pos > buffer->col - 1) {
        buffer->focus->data[pos] = buffer->focus->data[pos - 1];
        pos--;
    }

    buffer->focus->data[pos] = ch;
    buffer->col++;
    */
    return TEL_SUCCESS;
}

/* TODO */
int tel_buffer_delete(struct Buffer *buffer) {
    return TEL_SUCCESS;
}

/* TODO */
int tel_buffer_render(struct Buffer *buffer) {
    return TEL_SUCCESS;
}