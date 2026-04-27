/*
* Copyright (C) 2026 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>

#include <stdlib.h>

#include <ctype.h>

#include <string.h>

int prn = 0;
static int days_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

char readchar(void) {
    char c = getchar();
    //    if (prn) { putchar(c); fflush(stdout); }
    return c;
}

// skips blanks and returns the first nonblank

char skipblanks(void) {
    char cp;
    while (isspace(cp = readchar()));
    return cp;
}

// matches c or exits

void matchar(char c) {
    char cp;
    if ((cp = skipblanks()) != c) {
        fprintf(stderr, "Error, expected %c and read %c\n", c, cp);
        exit(1);
    }
}

// skip string, the first " is already read

void skipstring(void) {
    char c;
    while (1) {
        c = readchar();
        if (c == '"') break;
        if (c == '\\') readchar();
    }
}

// has read a { or [, will continue until reading the matching } or ]
// brackets inside "..." are not considered
// op and cl are {} or []

void close(char op, char cl) {
    int depth = 1;
    while (depth) {
        char c = readchar();
        if (c == '"') skipstring();
        else if (c == op) depth++;
        else if (c == cl) depth--;
    }
}

// reads until finding a new label (") or the end of a construction (})
// returns if it was the first case

int nextlabel(void) {
    char c;
    while (((c = readchar()) != '"') && (c != '}'));
    if (c == '}') {
        ungetc(c, stdin);
        return 0;
    }
    return 1;
}

// reads a label and returns if it equals label
// assumes the first " is already read
// returns 1 if matched, 0 if not, but always reads the last "

int matchlabel(char *label) {
    char c;
    while ((c = readchar()) == *(label++));
    if ((c == '"') && (label[-1] == 0)) return 1;
    while (c != '"') c = readchar();
    return 0;
}

// reads a label and returns it in label
// assumes the first " is already read
// always reads the last "

void readlabel(char *label) {
    while ((*(label++) = readchar()) != '"');
    label[-1] = 0;
}

int fits_int64(const char *s) {
    // skip sign
    if (*s == '+' || *s == '-') s++;

    // skip leading zeros
    while (*s == '0') s++;

    int len = strlen(s);

    // empty or just zeros → fits
    if (len == 0) return 1;

    // max int64 = 9223372036854775807 (19 digits)
    if (len < 19) return 1;
    if (len > 19) return 0;

    // length == 19 → compare lexicographically
    return strcmp(s, "9223372036854775807") <= 0;
}

int is_plain_integer(const char *s) {
    if (*s == '+' || *s == '-') s++;

    if (!isdigit(*s)) return 0;

    while (*s) {
        if (!isdigit(*s)) return 0;
        s++;
    }
    return 1;
}

// similar to readlabel but ignores a leading '+' and keeps '-'
int readamount(char *label) {
    char* start = label;
    char c = readchar();
    if (c == '+') {
        while ((*(label++) = readchar()) != '"');
        label[-1] = 0;
    } else {
        while ((*(label++) = c) != '"')
            c = readchar();
        label[-1] = 0;
    }

    // label now contains the raw amount string
    return (is_plain_integer(start) && fits_int64(start));
}

int year_out_of_range(const char *t) {
    //fprintf(stderr, "Year out of range: %s\n", t);
    // expects format: +YYYY-MM-DD... or -YYYY-MM-DD...
    long long year = atoll(t);
    return (year > 9999LL || year < -9999LL);
}

int is_leap(long long year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int is_valid_date(long long year, int month, int day) {
    //if (month == 0 && day == 0) return 1; // allow just a year
    //if (day == 0) return 1; // allow just a year and month

    if (month < 1 || month > 12) return 0;
    if (day < 1) return 0;



    // Ajustar febrero si es bisiesto
    if (month == 2 && is_leap(year)) {
        return day <= 29;
    }

    return day <= days_month[month - 1];
}

int readtime(char* label) {
    char* start = label;
    while ((*(label++) = readchar()) != '"');
    label[-1] = 0;

    char year[21], month[3], day[3];
    year[0] = start[0]; // copy the sign
    int i = 1;
    while (start[i] && start[i] != '-' && i < 20) {
        year[i] = start[i];
        i++;
    }
    if (i >= 20) return 0; //year too long, not a valid time
    year[i] = 0;
    if (year_out_of_range(year)) return 0;
    ++i; // skip '-'
    month[0] = start[i]; month[1] = start[i+1]; month[2] = 0;
    if (month[0] == '0' && month[1] == '0') { // avoid month 00, consider it as 01
        month[1] = '1';
        start[i+1] = '1';
    }
    i += 3; // skip month and '-'
    day[0] = start[i]; day[1] = start[i+1]; day[2] = 0;
    if (day[0] == '0' && day[1] == '0') { // avoid day 00, consider it as 01
        day[1] = '1';
        start[i+1] = '1';
    }
    long long y = atoll(year);
    int m = atoi(month);
    int d = atoi(day);
    //fprintf(stderr, "Read time: %s, year: %s, ok: %i\n", start, year, ok);
    return is_valid_date(y, m, d);
}

// reads a string and returns it between quotes
// assumes the first " is already read
// always reads the last "
void readstring(char *label) {
    *(label++) = '"';
    while ((*(label++) = readchar()) != '"');
    *label = 0;
}

void readstring2(char *label) {
    *(label++) = '"';
    char c;
    int escape = 0;
    while (1) {
        c = readchar();
        if (!escape && c == '"') break;
        if (!escape && c == '\\') escape = 1;
        else escape = 0;
        *(label++) = c;
    }
    *(label++) = '"';
    *label = 0;
}

int readcoord(char* label) {
    char c = skipblanks();
    int ok = 0;
    while (c != ',' && c != '}') {
        *(label++) = c;
        c = readchar();
        if (ok && c == '.') return 0; // more than one dot, not a valid coordinate
        if (!ok && c == '.') ok = 1;
    }
    *label = 0;
    return ok;
}

// passes the value of an undesired label. the value can be numeric,
// a string, or something between { ... }
// returns 1 if there is some next item in the list, 0 if read a }

int passvalue(void) {
    char c;
    matchar(':');
    c = skipblanks();
    if (c == '"') skipstring(); // a string
    else if (c == '{') close('{', '}');
    else if (c == '[') close('[', ']');
    else // something else, scan for , or }
    {
        while ((c != ',') && (c != '}')) c = readchar();
        ungetc(c, stdin);
    }
    while (isspace(c = readchar()));
    if (c == ',') return 1;
    if (c == '}') {
        ungetc(c, stdin);
        return 0;
    }
    fprintf(stderr, "Error: expected ',' or '}', but read a %c\n", c);
    exit(1);
}

// skips pairs name:value until name = label
// returns if it found it

int findlabel(char *label) {
    while (nextlabel()) // there is a next label
    {
        if (matchlabel(label)) return 1;
        if (!passvalue()) return 0;
    }
    return 0; // could reach here if there is a , and then the }
}

void main(int argc, char **argv) {
    char entity[1024];
    char property[1024];
    char properties[1024000];
    char labels[102400];
    char rentity[1024];
    char rank[1024];
    char time[1024];
    char value[2048];
    char lat[128], lon[128];
    int good1, good2;
    char c;
    int count = 0;

    // output files
    FILE *edges = fopen("edges.tsv", "w");
    FILE *nodes = fopen("nodes.tsv", "w");
    if (!edges || !nodes) {
        fprintf(stderr, "Files cannot be opened\n");
        return;
    }


    matchar('[');
    while (skipblanks() == '{') // another entity
    {
        labels[0] = properties[0] = 0;
        if (!findlabel("id")) {
            matchar('}');
            if (skipblanks() == ']') break; // en of list
            continue;
        }
        ++count;
        fprintf(stderr, "Processed %d entities\n", count);

        matchar(':');
        matchar('"');
        readlabel(entity);
       // fprintf(stderr, "Entity name: %s\n", entity);
        if (findlabel("claims")) {
            matchar(':');
            matchar('{');
            while (skipblanks() == '"') // there is another property (claim)
            {
                readlabel(property);
         //       printf("Property name: %s\n", property);
         //       fflush(stdout);
                // if (!strcmp(entity,"Q31") && !strcmp(property,"P85"))
                //    prn = 1; // breakpoint
                matchar(':');
                matchar('['); // list of items of property
                int first_item = 1;
                while (skipblanks() == '{') // another property item
                {
                    good1 = good2 = 0;
                    time[0] = rank[0] = lat[0] = lon[0] = value[0] = rentity[0] = 0;

                    if (!first_item) {
                        close('{', '}');
                        if (skipblanks() == ']') break;
                        continue;  // next item
                    }


                    if (findlabel("mainsnak")) {
                        matchar(':');
                        matchar('{');
                        if (findlabel("datavalue")) {
                            matchar(':');
                            matchar('{');
                            if (findlabel("value")) {
                                matchar(':');
                                if ((c = skipblanks()) == '{') {
                                    matchar('"');
                                    char label1[1024];
                                    readlabel(label1);
                                    if (!strcmp(label1, "entity-type")) { // entity
                                        while (nextlabel()) { // check all the labels until finding id
                                            readlabel(label1);
                                            if (!strcmp(label1, "id")) {
                                                matchar(':');
                                                matchar('"');
                                                readlabel(rentity);
                                                if (rentity[0] == 'Q') good1 = 1;
                                                else good1 = 0; // avoid creating an edge with a non-entity (LEXEMES)
                                                break;
                                            }
                                        }
                                        if (!strcmp(property, "P31")) {
                                            // instance of
                                            // set good1 to 0 to avoid creating an edge
                                            good1 = 0;
                                            char label_entry[2560];
                                            sprintf(label_entry, ":%s", rentity);
                                            strcat(labels, label_entry);
                                        }
                                    } else if (!strcmp(label1, "amount")) { //quantity
                                        matchar(':');
                                        matchar('"');
                                        if (readamount(value)) good1 = 2; // Literal
                                        else good1 = 0;
                                    } else if (!strcmp(label1, "latitude")) { // coordinates
                                        matchar(':');
                                        int okcoord = readcoord(lat);
                                        //matchar(',');
                                        //printf("Lat: %s\n", lat);
                                        //printf("OK: %i\n", okcoord);
                                        //fflush(stdout);
                                        if (okcoord && findlabel("longitude")) {
                                            matchar(':');
                                            okcoord = readcoord(lon);
                                            //printf("Lon: %s\n", lon);
                                            //printf("OK: %i\n", okcoord);
                                            //fflush(stdout);
                                            if (!okcoord) good1 = 0; // avoid creating an edge with an incorrect coord
                                            else good1 = 3; // Literal with coords
                                        }else good1 = 0;

                                    } else if (!strcmp(label1, "time")) { // time
                                        matchar(':');
                                        matchar('"');
                                        if (readtime(value)) good1 = 2; // Literal
                                        else good1 = 0; // avoid creating an edge with an incorrect time
                                    }
                                    close('{', '}'); // of value
                                } else if (c == '"') { //String
                                    readstring2(value);
                                    good1 = 2; // Literal
                                }
                                // there is no other option: https://www.wikidata.org/wiki/Help:Data_type
                            }
                            close('{', '}'); // of datavalue
                            if (good1 && findlabel("datatype")) {
                                matchar(':');
                                matchar('"');
                                if (matchlabel("wikibase-item"))
                                    good2 = 1;
                                else
                                    good2 = good1; // Literal
                            }
                        }
                        close('{', '}'); // of mainsnak
                        if (good2 == 1) { //edge
                            char label0[1024];
                            fprintf(edges,"%s\t%s\t%s",
                                   entity, property, rentity);
                            while (nextlabel()) {
                                readlabel(label0);
                                if (!strcmp(label0, "qualifiers")) {
                                    matchar(':');
                                    matchar('{');
                                    while (nextlabel()) {
                                        char label[1024];
                                        readlabel(label);
                                        matchar(':');
                                        matchar('[');
                                        if ((c = skipblanks()) == '{') {
                                            if (findlabel("datavalue")) {
                                                matchar(':');
                                                matchar('{');
                                                if (findlabel("value")) {
                                                    matchar(':');
                                                    if ((c = skipblanks()) == '{') {
                                                        char label1[1024];
                                                        matchar('"');
                                                        readlabel(label1);
                                                        //if (findlabel("time")) {
                                                        if (!strcmp(label1, "time")) {
                                                            matchar(':');
                                                            matchar('"');
                                                            int oktime = readtime(time);
                                                            if (oktime) fprintf(edges, "\t%s:%s", label, time);
                                                        }else if (!strcmp(label1, "amount")) {
                                                                matchar(':');
                                                                matchar('"');
                                                                int okamount = readamount(value);
                                                                if (okamount) fprintf(edges, "\t%s:%s", label, value);
                                                        } else if (!strcmp(label1, "latitude")) {
                                                            // coordinates
                                                            matchar(':');
                                                            int okcoord = readcoord(lat);
                                                            //matchar(',');
                                                            if (okcoord && findlabel("longitude")) {
                                                                matchar(':');
                                                                okcoord = readcoord(lon);
                                                                if (okcoord) fprintf(edges, "\t%slat:%s\t%slon:%s", label, lat, label, lon);
                                                            }
                                                        }
                                                    } else if (c == '"') skipstring();
                                                }
                                            }
                                        } else if (c == '"') skipstring();
                                        close('[', ']'); // only one elem of list
                                    }
                                    close('{', '}'); // of qualifiers
                                } else if (!strcmp(label0, "rank")) {
                                    matchar(':');
                                    matchar('"');
                                    readstring2(rank);
                                } else // another label0
                                    passvalue();
                            }
                            if (!rank[0]) strcpy(rank, "?");
                            fprintf(edges, "\trank:%s\n", rank);
                        }

                        if (good2 == 2) {
                            char prop_entry[2560];
                            sprintf(prop_entry, "\t%s:%s", property, value);
                            strcat(properties, prop_entry);
                            first_item = 0; // avoid processing more than one item
                        }

                        if (good2 == 3) {
                            char prop_entry[2560];
                            sprintf(prop_entry, "\t%slat:%s", property, lat);
                            strcat(properties, prop_entry);
                            sprintf(prop_entry, "\t%slon:%s", property, lon);
                            strcat(properties, prop_entry);
                        }
                    }
                    close('{', '}'); // of property list item

                    if (skipblanks() == ']') break; // else a , = more items
                }
                if (skipblanks() == '}') break; // else a , = more properties
            }
        }
        fprintf(nodes, "%s\t%s\t%s\n", entity, labels, properties);
        fflush(nodes); fflush(edges);
        close('{', '}'); // end of entity
        if (skipblanks() == ']') break;
    }
    fprintf(stderr, "Finished\n");
}
