#ifndef __TXT_ESCAPE_H
#define __TXT_ESCAPE_H

class cTxtEscape{
	public:
		static int EscapeStr(char szTo[], const char* szFrom, int iLen=-1);
		static int UnescapeStr(char** ppszStr, char chDelimiter);
		static int EscapeBin(char szTo[], int iLen, const char* szFrom);
		static int UnescapeBin(char** ppszStr, char chDelimiter);
};

#endif
