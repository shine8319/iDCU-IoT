#include <stdio.h>
#include <ctype.h> // isspace()
#include <string.h> // strcpy_s() strlen()

#define MAX_STR_LEN 4000
// 문자열 우측 공백문자 삭제 함수
char* rtrim(char* s) {
    char t[MAX_STR_LEN];
    char *end;

    // Visual C 2003 이하에서는
    // strcpy(t, s);
    // 이렇게 해야 함
    //strcpy_s(t, s); // 이것은 Visual C 2005용
    strcpy(t, s);
    end = t + strlen(t) - 1;
    while (end != t && isspace(*end))
	end--;
    *(end + 1) = '\0';
    s = t;

    return s;
}


// 문자열 좌측 공백문자 삭제 함수
char* ltrim(char *s) {
    char* begin;
    begin = s;

    while (*begin != '\0') {
	if (isspace(*begin))
	    begin++;
	else {
	    s = begin;
	    break;
	}
    }

    return s;
}


// 문자열 앞뒤 공백 모두 삭제 함수
char* trim(char *s) {
    return rtrim(ltrim(s));
}
