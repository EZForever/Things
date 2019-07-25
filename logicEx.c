#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _tlist {
    unsigned int var, val;
} tlist;

void rmSpaces(char buf[]) {
    char *pBuf1, *pBuf2;
    pBuf1 = pBuf2 = buf;
    while(*pBuf1) {
        if(*pBuf1 != ' ' && *pBuf1 != '\n')
            *pBuf2++ = *pBuf1;
        pBuf1++;
    }
    *pBuf2 = 0;
}

unsigned int eval(char *start, char *end, tlist *ctx);

unsigned int getVar(char **start, char *end, tlist *ctx) {
    unsigned int ret = 0, flgNot = 0, varId, stk;
    char *pEnd;
    if(**start == '!') {
        flgNot = 1;
        (*start)++;
    }
    switch(**start) {
    case '(':
        stk = 1;
        pEnd = *start + 1;
        while(stk) {
            if(*pEnd == '(')
                stk++;
            else if(*pEnd == ')')
                stk--;
            pEnd++;
        }
        ret = eval(*start + 1, pEnd - 2, ctx);
        *start = pEnd;
        break;
    case 'T':
        ret = 1;
        (*start)++;
        break;
    case 'F':
        ret = 0;
        (*start)++;
        break;
    default:
        varId = **start - 'a';
        ret = (ctx->var & (1u << varId)) >> varId;
        (*start)++;
    }
    if(flgNot)
        ret = !ret;
    return ret;
}

unsigned int eval(char *start, char *end, tlist *ctx) {
    unsigned int ret = getVar(&start, end, ctx), v;
    char op;
    while(start < end) {
        op = *start++;
        v = getVar(&start, end, ctx);
        switch(op) {
        case '&':
            ret = ret && v;
            break;
        case '|':
            ret = ret || v;
            break;
        case '>':
            ret = !ret || v;
            break;
        default:
            fprintf(stderr, "Unknown operator '%c'\n", op);
        }
    }
    return ret;
}

int vCnt(char buf[]) {
    int ret = 0, v;
    char *pBuf = buf;
    while(*pBuf) {
        if(*pBuf >= 'a' && *pBuf <= 'z') {
            v = *pBuf - 'a' + 1;
            ret = ret < v ? v : ret;
        }
        pBuf++;
    }
    return ret;
}

void printTList(tlist *list, const unsigned int cnt) {
    unsigned int i, j;
    for(i = 0; i < cnt; i++)
        printf("==");
    puts("=");
    for(i = 0; i < cnt; i++)
        printf("%c ", 'a' + i);
    printf("R\n");
    for(i = 0; i < cnt; i++)
        printf("--");
    puts("-");
    for(i = 0; i < (1u << cnt); i++) {
        for(j = 0; j < cnt; j++)
            printf("%c ", (list[i].var & (1u << j)) ? 'T' : 'F');
        printf("%c\n", list[i].val ? 'T' : 'F');
    }
    for(i = 0; i < cnt; i++)
        printf("==");
    puts("=");
}

int main(void) {
    char buf1[32], buf2[32];
    unsigned int i, listLen, cnt;
    tlist *list1, *list2;

    puts("Input two expressions:");
    fgets(buf1, 32, stdin);
    fgets(buf2, 32, stdin);
    rmSpaces(buf1);
    rmSpaces(buf2);

    cnt = vCnt(buf1);
    if(cnt < vCnt(buf2))
        cnt = vCnt(buf2);
    listLen = 1u << cnt;
    list1 = (tlist *)malloc(sizeof(tlist) * listLen);
    list2 = (tlist *)malloc(sizeof(tlist) * listLen);
    for(i = 0; i < listLen; i++) {
        list1[i].var = list2[i].var = i;
        list1[i].val = eval(buf1, buf1 + strlen(buf1), &list1[i]);
        list2[i].val = eval(buf2, buf2 + strlen(buf2), &list2[i]);
    }

    printf("\nTable of %s:\n", buf1);
    printTList(list1, cnt);
    printf("\nTable of %s:\n", buf2);
    printTList(list2, cnt);

    printf("\nEquations are%s equivalent.\n",
        memcmp(list1, list2, sizeof(tlist) * listLen) ? " not" : ""
    );
    free(list1);
    free(list2);
    return 0;
}

