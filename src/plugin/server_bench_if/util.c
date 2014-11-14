#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

static const char default_ifs[256] = { [9]=1, [10]=1, [13]=1, [32]=1 };

void inline build_ifs(char *tifs, const char *ifs0) 
{
	const u_char *ifs = (u_char*)ifs0;
	memset(tifs, 0, 256);
	while(*ifs) tifs[*ifs++] = 1;
}

/*
 * NULL IFS: default blanks
 * first byte is NULL, IFS table
 * first byte is NOT NULL, IFS string
 */
int str_explode(const char *ifs, char *line0, char *field[], int n) 
{
	u_char *line = (u_char*)line0;
	int i;

	if(ifs==NULL) {
		ifs = default_ifs;
	} else if(*ifs) {
		char *implicit_ifs = alloca(256);
		build_ifs(implicit_ifs, ifs);
		ifs = implicit_ifs ;
	}

	i = 0;
	while(1) {
		while(ifs[*line]) line++;
		if(!*line) break;
		field[i++] = (char*)line;
		if(i>=n) {
			line += (strlen((const char*)line) - 1);
			while(ifs[*line]) line--;
			line[1] = '\0';
			break;
		}
		while(*line && !ifs[*line]) line++;
		if(!*line) break;
		*line++ = '\0';
	}

	return i;
}

void mkdir_path(const char *path)
{
    if(mkdir(path, 0777)==-1 && errno==ENOENT) {
        char *p = strdup(path);        
        char *q = strrchr(p, '/');     
        if(q) {
            *q = '\0';
            mkdir_path(p); 
            mkdir(path, 0777);
        }
    }  
}

