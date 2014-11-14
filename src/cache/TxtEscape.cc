#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "TxtEscape.h"

int cTxtEscape::EscapeStr(char szTo[], const char* szFrom, int iLen)
{
	char* p=szTo;

	if(iLen < 0)
		iLen = strlen(szFrom);
	for(int i=0; i<iLen; i++){
		switch(*szFrom){
			case '\\':
				*p++ = '\\';
				*p++ = '\\';
				break;
	
			case '\t':
				*p++ = '\\';
				*p++ = 't';
				break;
			case '\b':
				*p++ = '\\';
				*p++ = 'b';
				break;
			case '\r':
				*p++ = '\\';
				*p++ = 'r';
				break;
			case '\n':
				*p++ = '\\';
				*p++ = 'n';
				break;
			default:
				*p++ = *szFrom;
				break;
		}
		szFrom++;
	}
	*p = 0;
	
	return(p-szTo);
}

int cTxtEscape::UnescapeStr(char** ppszStr, char chDelimiter)
{
	int iEscape;
	char* begin=*ppszStr;
	char* p=*ppszStr;
	
	iEscape = 0;
	while(**ppszStr && **ppszStr!='\t' && **ppszStr!=chDelimiter){
		switch(**ppszStr){
			case '\\':
				iEscape++;
				if(iEscape == 2){
					*p = '\\';
					iEscape = 0;
					p++;
				}
				break;
			case 't':
			case 'b':
			case 'r':
			case 'n':
				if(iEscape == 1){
					switch(**ppszStr){
						case 't': *p = '\t'; break;
						case 'b': *p = '\b'; break;
						case 'r': *p = '\r'; break;
						case 'n': *p = '\n'; break;
					}
					iEscape = 0;
					p++;
				}
				else{
					*p = **ppszStr;
					p++;
				}
				break;
			default:
				iEscape = 0;
				*p = **ppszStr;
				p++;
				break;
		}
		(*ppszStr)++;
	}
//	*p = 0;
	
	return(p-begin);
}

int cTxtEscape::EscapeBin(char szTo[], int iLen, const char* szFrom)
{
	char* p;
	unsigned char* q;
	
	q = (unsigned char*)szFrom;
	p = szTo;
	for(int i=0; i<iLen; i++){
		p += snprintf(p, 3, "%02x", *q);
		q++;
	}
	*p = 0;
	
	return(p-szTo);
}

int cTxtEscape::UnescapeBin(char** ppszStr, char chDelimiter)
{
	int iLen;
	unsigned int iTmp;
	char* begin=*ppszStr;
	char* p=*ppszStr;
	
	while(**ppszStr && **ppszStr!='\t' && **ppszStr != chDelimiter){
		if(sscanf(*ppszStr, "%02x%n", &iTmp, &iLen) != 1)
			return(-1);
		*p++ = iTmp;
		*ppszStr = (*ppszStr)+iLen;
	}
	
	return(p - begin);
}


