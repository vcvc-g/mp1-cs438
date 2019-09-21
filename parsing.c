void append(char *str, char ch){
    int len = strlen(str);
	//printf("%d\n", len);
    str[len] = ch;
    str[len + 1] = '\0';
}



char** str_split(char* str, char delim)
{   
    char** splitStr  = 0;
    size_t count     = 0;
    char* strPtr     = str;
    char delimArray[2];   //I HAVE NOT IDEA WHY strok wants to take an array as argument......stackflow save my life at this point....
    delimArray[0] = delim;
    delimArray[1] = '\0';

    /* Count how many elements will be extracted. */
    while (*strPtr){
        if (delim == *strPtr){
            count++;
        }
        strPtr++;
    }

    count++;

    splitStr = malloc(sizeof(char*) * count);

    if (splitStr){
        size_t idx  = 0;
        char* token = strtok(str, delimArray);

        while (token){
            *(splitStr + idx++) = strdup(token);
            token = strtok(0, delimArray);
        }
        *(splitStr + idx) = NULL;
    }

    return splitStr;
}

void remove_chars(char* str, char c) {
    char *strOld = str, *strNew = str;
    while (*strOld) {
        *strNew = *strOld;
        strNew += (*strNew != c);
		strOld++;
    }
    *strNew = '\0';
}