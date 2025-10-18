#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tel.h"

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

struct Buffer *tel_buffer_open(char *name, int my_line, int my_col) {
    int len = strlen(name)+1;
    struct Buffer *buffer = NULL;

    if (!(buffer = malloc(sizeof(struct Buffer)))) {
        fprintf(stderr, "Allocation for buffer \"%s\" failed.\n", name);
        return NULL;
    } else if (!(buffer->filename = malloc((sizeof(char) * len))) || !memcpy(buffer->filename, name, len)) {
        fprintf(stderr, "Allocation for buffer name \"%s\" failed.\n", name);
        tel_buffer_close(buffer);
        return NULL;
    } else if (!(buffer->fp = fopen(buffer->filename, "r+"))) {
        fprintf(stderr, "Opening buffer \"%s\" failed.\n", buffer->filename);
        tel_buffer_close(buffer);
        return NULL;
    } else if (my_line <= 0 || my_col <= 0) {
        fprintf(stderr, "Line and column numbers to open buffer \"%s\" should be natural.\n", buffer->filename);
        tel_buffer_close(buffer);
        return NULL;
    }
    
    buffer->line = my_line;
    buffer->col = my_col;
    buffer->head = NULL;

    return buffer;
}

int tel_buffer_close(struct Buffer *buffer) {
    struct Line *worm = buffer->head, *next = NULL;

    if (worm) {
        while (worm) {
            next = worm->next;
            free(worm->data);
            free(worm);
            worm = next;
        }
    }

    if (buffer->fp && fclose(buffer->fp)) {
        fprintf(stderr, "Closing buffer \"%s\" failed.\n", buffer->filename);
        return 1;
    }
    
    free(buffer->filename);
    free(buffer);

    return 0;
}

int tel_buffer_save(struct Buffer *buffer) {
    if (fflush(buffer->fp)) {
        fprintf(stderr, "Saving buffer to file \"%s\" failed.\n", buffer->filename);
        return 1;
    }

    return 0;
}

static int tel_buffer_slurp(struct Buffer *buffer) {
    struct Line *prev = NULL;
    long start = 0L;
    int ch = 0;

    rewind(buffer->fp);

    while (ch != EOF) {
        unsigned int i = 0, nbytes = 1;

        if (!(buffer->focus = malloc(sizeof(struct Line)))) {
            fprintf(stderr, "Allocation for a line for buffer \"%s\" failed.\n", buffer->filename);
            return 1;
        }
        if (!buffer->head) buffer->head = buffer->focus;
        if (prev) prev->next = buffer->focus;
        buffer->focus->size = 0;
        buffer->focus->data = NULL;
        buffer->focus->next = NULL;

        start = ftell(buffer->fp);
        while ((ch = fgetc(buffer->fp)) != NEWLINE && ch != EOF) nbytes++;

        if (fseek(buffer->fp, start, SEEK_SET)) {
            fprintf(stderr, "Seeking in buffer \"%s\" failed.\n", buffer->filename);
            return 1;
        }
        
        buffer->focus->size = nbytes + LINE_EXTRA + 1;
        if (!(buffer->focus->data = malloc(sizeof(char) * buffer->focus->size))) {
            fprintf(stderr, "Line allocation for \"%s\" failed.\n", buffer->filename);
            return 1;
        }

        for (i = 0; (ch = fgetc(buffer->fp)) != NEWLINE && ch != EOF; i++)
            buffer->focus->data[i] = ch;
        buffer->focus->data[i++] = ch;
        buffer->focus->data[i] = '\0';
        prev = buffer->focus;
    }

    return 0;
}

/* TODO */
int tel_buffer_move(struct Buffer *buffer, int arrow) {
    return 0;
}

/* TODO */
int tel_buffer_insert(struct Buffer *buffer, char ch) {
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