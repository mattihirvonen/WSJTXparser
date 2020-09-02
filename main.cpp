#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include <iostream>

//using namespace std;

#define ALLTXT     "C:\\Users\\matti.hirvonen\\AppData\\Local\\WSJT-X\\ALL.TXT"
#define MAXTOKENS  16
#define MAXCALLS   100000
#define TOKENLEN   16

typedef struct {
    char   callsign[16];
    char   square[8];
    char   band[8];
    char   mode[8];
    //
    int    rows;          // counter: call sign exist in rows
    int    received;      // flag: real received call sign
    int    picked;        // flag: picked call sign from message (opposite station)

} calldata_t;

typedef struct {
    int         calls;           // Count of active: call signs / band / mode
    calldata_t  data[MAXCALLS];
} callinfo_t;

// WSJT-X's ALL.TXT log data fields:
enum WSJTX_fields { DATETIME=0, BAND, RXTX, MODE, RXLEVEL, SOMETHING, HZ, TOKEN1, TOKEN2, TOKEN3, TOKEN4, TOKEN5, TOKEN6 };

char tokens[MAXTOKENS][TOKENLEN];

callinfo_t callinfo;

//------------------------------------------------------------------------------
// Some test functions:

void print_tokens( char tokens[][TOKENLEN], int nr )
{
    printf("%d: ",nr);
    for (int i = 0; i < nr; i++) {
        printf("%s,",tokens[i]);
    }
    printf("\n");

}


void print_callsigns( void )
{
    for (int i = 0; i < callinfo.calls; i++ )
        printf("%5d  %10s  %8s  %8s  %8s\n", i, callinfo.data[i].callsign,
               callinfo.data[i].band, callinfo.data[i].mode, callinfo.data[i].square );
}


void check_dupes( void )
{
    for (int i = 0; i < callinfo.calls - 1; i++ ) {
        for ( int j = i+1; j < callinfo.calls; j++ ) {
            if (strcmp(callinfo.data[i].callsign,callinfo.data[j].callsign) == 0)
                printf("dupe i=%d j=%d %s\n", i, j,callinfo.data[i].callsign );
        }
    }
}
//------------------------------------------------------------------------------
// ALL.TXT log file parser:

int tokenize( char tokens[][TOKENLEN], char *line )
{
    int   field, space;
    char *token;

    for (field=0; field<MAXTOKENS; field++) {
        tokens[field][0] = 0;
    }
    for (token=&tokens[0][0], space=field=0; *line; line++) {
        if (!isspace(*line)) {
            *token++ = *line;
            *token   =  0;
             space   =  0;
        }
        else if (!space) {
            token = tokens[++field];
            space = 1;
        }
    }
    return field;
}


int issquare( char *token )
{
    if (!token)              return 0;
    if (!isalpha(token[0]))  return 0;
    if (!isalpha(token[1]))  return 0;
    if (!isdigit(token[2]))  return 0;
    if (!isdigit(token[3]))  return 0;

    if (!strcmp(token,"RR73"))  return 0;

    return 1;
}


int find_callsign( char *callsign, char *band, char *mode )
{
    for ( int ix = 0; ix < callinfo.calls; ix++ )
    {
        if ( strcmp(callinfo.data[ix].callsign,callsign)==0 &&
             strcmp(callinfo.data[ix].band,    band    )==0 &&
             strcmp(callinfo.data[ix].mode,    mode    )==0 )
        {
            return ix;
        }
    }
    return -1;
}


int add_callsign( char *callsign, char *band, char *mode, char *square  )
{
    int newcall = 0;
    int ix      = find_callsign( callsign, band, mode );

    if (ix < 0) {
        ix = callinfo.calls++;
        newcall++;
    //  printf("newcall  ix=%d  %s\n", ix, callsign);
    }
    strcpy(callinfo.data[ix].callsign, callsign);
    strcpy(callinfo.data[ix].band,     band);
    strcpy(callinfo.data[ix].mode,     mode);
    if (issquare(square))
        strcpy(callinfo.data[ix].square, square);
    return newcall;
}


int analyze_tokens( char tokens[][TOKENLEN])
{
    int newcall;

    if (strcmp(tokens[TOKEN1],"CQ") == 0) {
        if (strlen(tokens[TOKEN2])  >= 4) {
            newcall = add_callsign(tokens[TOKEN2], tokens[BAND], tokens[MODE], tokens[TOKEN3]);
        }
        else {
            newcall = add_callsign(tokens[TOKEN3], tokens[BAND], tokens[MODE], tokens[TOKEN4]);
        }
    }
    #if 1
    // Reply to "CQ"
    else {
        newcall = add_callsign(tokens[TOKEN2], tokens[BAND], tokens[MODE], tokens[TOKEN3]);
    }
    #endif
    return newcall;
}


int parse_row(char *line)
{
    int nr = tokenize(tokens, line);

    nr = nr;
//  print_tokens( tokens, nr );
    return analyze_tokens( tokens );
}


int read_AllTxt( const char *filename )
{
    int  rows = 0;
    FILE *fd  = fopen( filename, "r" );

    if (!fd) {
        return -1;
    }

    char   line[256];
    while (fgets(line, sizeof(line)-1, fd) != NULL) {
        parse_row( line );
        rows++;
    }
    print_callsigns();

    fclose( fd );
    return rows;
}


int main()
{
    int rows = read_AllTxt( ALLTXT );

    printf("\n rows=%d call+bands+modes=%d\n", rows, callinfo.calls);
    check_dupes();
    return 0;
}
