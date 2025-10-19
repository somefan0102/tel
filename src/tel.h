#ifndef TEL_H
#define TEL_H

#define TEL_VERSION "tel 0\n"

#define TEL_HELP \
"usage:   tel [option]... [file]\n"\
"another minimalist text editor for the terminal\n"\
"\n"\
"options: \n"\
"  -h   show help\n"\
"  -v   show version\n"\

#define LINE_EXTRA 0x10
#define LINE_FEED '\n'

typedef struct Buffer Buffer;

Buffer *tel_buffer_open(char *name, unsigned int my_line, unsigned int my_col);
int tel_buffer_close(Buffer *buffer);
int tel_buffer_move(Buffer *buffer, int arrow);
int tel_buffer_render(Buffer *buffer);
int tel_buffer_insert(Buffer *buffer, char ch);
int tel_buffer_delete(Buffer *buffer);
int tel_buffer_save(Buffer *buffer);

#endif