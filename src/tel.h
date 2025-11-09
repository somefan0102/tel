#ifndef TEL_H
#define TEL_H

#define TEL_VERSION "tel 0\n"

#define LINE_EXTRA 0x10
#define LINE_FEED '\n'

typedef struct Buffer Buffer;

enum {
    TEL_SUCCESS,
    TEL_ERROR_REOPEN,
    TEL_ERROR_BUFFER_ALLOCATED,
    TEL_ERROR_FSEEK_FAILED,
    TEL_ERROR_MALLOC_FAILED,
    TEL_ERROR_STRDUP_FAILED,
    TEL_ERROR_LINE_OOB
};

Buffer *tel_buffer_open(char *name, int *error, unsigned int line, unsigned int col);
void tel_buffer_close(Buffer *buffer);
int tel_buffer_move(Buffer *buffer, int arrow);
int tel_buffer_render(Buffer *buffer);
int tel_buffer_insert(Buffer *buffer, char ch);
int tel_buffer_delete(Buffer *buffer);
int tel_buffer_save(Buffer *buffer);

#endif