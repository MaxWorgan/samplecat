#define AYYI_UTILS_C
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <glib.h>

#include "ayyi_utils.h"

extern int debug;


void
debug_printf(const char* func, int level, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    if (level <= debug) {
        fprintf(stderr, "%s(): ", func);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}


void
errprintf(char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s ", ayyi_err);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


void
errprintf2(const char* func, char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s %s(): ", ayyi_err, func);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


void
errprintf3(const char* func, char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s %s(): ", ayyi_err, func); //move to log_handler

  char str[256];

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vsprintf(str, format, argp);
  va_end(argp);           //clean up

  g_critical(str);
}


void 
warnprintf2(const char* func, char *format, ...)
{
  //fn prints a warning string, then passes arguments on to vprintf.

  printf("%s %s(): ", smwarn, func);

  va_list argp;
  va_start(argp, format);
  vprintf(format, argp);
  va_end(argp);
}


void
warn_gerror(const char* msg, GError** error)
{
	//print and free the GEerror
	if(*error){
		printf("%s %s: %s\n", smwarn, msg, (*error)->message);
		g_error_free(*error);
		*error = NULL;
	}
}


void
info_gerror(const char* msg, GError** error)
{
	//print and free the GEerror
	if(*error){
		printf("%s: %s\n", msg, (*error)->message);
		g_error_free(*error);
		*error = NULL;
	}
}


gchar*
path_from_utf8(const gchar* utf8)
{
	gchar *path;
	GError *error = NULL;

	if (!utf8) return NULL;

	path = g_locale_from_utf8(utf8, -1, NULL, NULL, &error);
	if (error)
		{
		printf("Unable to convert to locale from UTF-8:\n%s\n%s\n", utf8, error->message);
		g_error_free(error);
		}
	if (!path)
		{
		/* if invalid UTF-8, text probaby still in original form, so just copy it */
		path = g_strdup(utf8);
		}

	return path;
}


GList*
get_dirlist(const char* path)
{
	/*
	scan a directory and return a list of any subdirectoies. Not recursive.
	-the list, and each entry in it,  must be freed.
	*/

	GList* dir_list = NULL;
	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError *error = NULL;
	GDir *dir;
	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, 128, "%s/%s", path, file);

			if(g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				dbg (2, "found dir: %s", filepath);
				dir_list = g_list_append(dir_list, g_strdup(filepath));
			}
		}
		g_dir_close(dir);
	}else{
		gwarn ("cannot open directory. %s", error->message);
		g_error_free(error);
		error = NULL;
	}
	return dir_list;
}


int
get_terminal_width()
{
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, (void *)&ws) != 0) printf("ioctl failed\n");

	//printf("terminal width = %d\n", ws.ws_col);

	return ws.ws_col;
}

