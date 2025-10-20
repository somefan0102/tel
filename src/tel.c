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

struct Buffer *tel_buffer_open(char *name, unsigned int my_line, unsigned int my_col) {
    int len = strlen(name) + 1;
    struct Buffer *buffer = NULL;

    if (!(buffer = malloc(sizeof(struct Buffer)))) {
        fprintf(stderr, "Error: Allocation for buffer \"%s\" failed.\n", name);
        return NULL;
    }

    buffer->line = my_line;
    buffer->col = my_col;
    buffer->focus = buffer->head = NULL;

    if (!(buffer->filename = malloc((sizeof(char) * len))) || !memcpy(buffer->filename, name, len)) {
        fprintf(stderr, "Error: Allocation for buffer name \"%s\" failed.\n", name);
        tel_buffer_close(buffer);
        return NULL;
    } else if (!(buffer->fp = fopen(buffer->filename, "r+"))) {
        fprintf(stderr, "Error: Opening buffer \"%s\" failed.\n", buffer->filename);
        tel_buffer_close(buffer);
        return NULL;
    } else if (my_line <= 0 || my_col <= 0) {
        fprintf(stderr, "Error: Line and column numbers to open buffer \"%s\" should be natural.\n", buffer->filename);
        tel_buffer_close(buffer);
        return NULL;
    }

    return buffer;
}

int tel_buffer_close(struct Buffer *buffer) {
    struct Line *next = NULL;

    if (buffer->head) {
        while (buffer->head) {
            next = buffer->head->next;
            free(buffer->head->data);
            free(buffer->head);
            buffer->head = next;
        }
    }

    if (buffer->fp && fclose(buffer->fp)) {
        fprintf(stderr, "Error: Closing buffer \"%s\" failed.\n", buffer->filename);
        return 1;
    }

    free(buffer->filename);
    free(buffer);

    return 0;
}

int tel_buffer_save(struct Buffer *buffer) {
    struct Line *scan = NULL;

    if (!buffer->head)
        if (tel_buffer_slurp(buffer)) return 1;

    if (fclose(buffer->fp) || !(buffer->fp = fopen(buffer->filename, "w"))) {
        fprintf(stderr, "Error: Reading buffer for saving \"%s\" failed.\n", buffer->filename);
        return 1;
    }

    scan = buffer->head;
    while (scan) {
        unsigned int count = 0;
        while (scan->data[count]) count++;
        if (!scan->next && scan->data[0] == LINE_FEED) break;
        fwrite(scan->data, sizeof(char), count, buffer->fp);
        scan = scan->next;
    }

    if (fclose(buffer->fp) || !(buffer->fp = fopen(buffer->filename, "r+"))) {
        fprintf(stderr, "Error: Reopening buffer after saving \"%s\" failed.\n", buffer->filename);
        return 1;
    }

    return 0;
}

static int tel_buffer_slurp(struct Buffer *buffer) {
    struct Line *prev = NULL;
    long start = 0L;
    int ch = 0;

    if (buffer->head) {
        fprintf(stderr, "Error: Buffer contents of \"%s\" already allocated.\n");
        return 1;
    } else if (fseek(buffer->fp, start, SEEK_SET)) {
        fprintf(stderr, "Error: Seeking in buffer \"%s\" failed.\n", buffer->filename);
        return 1;
    }

    while (ch != EOF) {
        unsigned int i = 0, nbytes = 0;

        if (!(buffer->focus = malloc(sizeof(struct Line)))) {
            fprintf(stderr, "Error: Allocation for a line for buffer \"%s\" failed.\n", buffer->filename);
            return 1;
        }

        if (!buffer->head) buffer->head = buffer->focus;
        if (prev) prev->next = buffer->focus;
        buffer->focus->size = 0;
        buffer->focus->data = NULL;
        buffer->focus->next = NULL;

        start = ftell(buffer->fp);
        while ((ch = fgetc(buffer->fp)) != LINE_FEED && ch != EOF) nbytes++;

        if (fseek(buffer->fp, start, SEEK_SET)) {
            fprintf(stderr, "Error: Seeking in buffer \"%s\" failed.\n", buffer->filename);
            return 1;
        }

        buffer->focus->size = nbytes + LINE_EXTRA + 1;
        if (!(buffer->focus->data = malloc(sizeof(char) * buffer->focus->size))) {
            fprintf(stderr, "Error: Line allocation for \"%s\" failed.\n", buffer->filename);
            return 1;
        }

        for (i = 0; (ch = fgetc(buffer->fp)) != LINE_FEED && ch != EOF; i++)
            buffer->focus->data[i] = ch;

        buffer->focus->data[i++] = LINE_FEED;
        buffer->focus->data[i] = '\0';
        prev = buffer->focus;
    }

    tel_buffer_refocus(buffer);

    return 0;
}

static int tel_buffer_refocus(struct Buffer *buffer) {
    unsigned int left = NULL;

    if (!buffer->head)
        if (tel_buffer_slurp(buffer)) return 1;

    left = buffer->line;

    buffer->focus = buffer->head;
    while (--left && buffer->focus) buffer->focus = buffer->focus->next;

    if (!buffer->focus) {
        fprintf(stderr, "Error: Line out of bounds.\n", buffer->filename);
        return 1;
    }

    return 0;
}

int tel_buffer_move(struct Buffer *buffer, int arrow) {
    if (!buffer->head)
        if (tel_buffer_slurp(buffer)) return 1;

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
        fprintf(stderr, "Error: Invalid key.\n", buffer->filename);
        return 1;
    }

    return 0;
}

int tel_buffer_insert(struct Buffer *buffer, char ch) {
    unsigned int pos = 0;

    if (!buffer->head)
        if (tel_buffer_slurp(buffer)) return 1;

    if (ch <= 0) {
        fprintf(stderr, "Error: Invalid character.\n");
        return 1;
    }

    while (buffer->focus->data[pos]) pos++;
    pos++;

    if (pos >= buffer->focus->size) {
        int i = 0;
        char *new_data = NULL;
        unsigned int new_size = buffer->focus->size + LINE_EXTRA;

        if (!(new_data = malloc(sizeof(char) * new_size))) {
            fprintf(stderr, "Error: Line reallocation for \"%s\" failed.\n", buffer->filename);
            return 1;
        }

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

    return 0;
}

/* TODO */
int tel_buffer_delete(struct Buffer *buffer) {
    return 0;
}

/* TODO */
int tel_buffer_render(struct Buffer *buffer) {
    return 0;
}