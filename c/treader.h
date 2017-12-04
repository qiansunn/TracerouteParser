#ifndef TREADER_H
#define TREADER_H

/*
 * file treader.h
 * Header for treader
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>

#include "list.h"

#define LINE_MAX_BUFFER 512
#define UNEQUAL 1
#define UPDATE_FLAG 2

/* 
 * struct route_t
 * Structure to keep one route record history
 */

typedef struct route {
    int counts;			/* Number of hops */
    struct sockaddr_in * sa;	/* Points to a series of IP address */
    double time_interval;	/* Time interval between route start and end*/	
    struct tm time_start;	/* Route start time */
    struct tm time_end;		/* Route end time */
} route_t;

/* The total route events number */
static int g_route_num;

/* Converts a number of seconds in human readable time
 * param duration: seconds to be converted 
 */

char * convert_seconds(double duration);

/* Output simultaneously onto console and a log file */
int ffprintf(FILE *fp1, FILE *fp2, char const *fmt, ...);

/*
 * Compare two IP address stored in sockaddr_in
 * param x and y: Structur need to be compared
 */

int sockaddr_cmp(struct sockaddr_in * x, struct sockaddr_in * y);

/*
 * Delete a route element structure
 * param element: The struture to delete
 */

void route_free(void * element);

/*
 * Print a route to user
 * param route: The route structure need output
 * param output_file: The file to store output
 */

void route_print(const route_t * route, FILE * output_file);

/*
 * Compare two route structure
 * param r1 and r2: Pointer to route need to be compared
 * return 
 */

int route_cmp(const route_t * r1, const route_t * r2);

/*
 * Parses one line traceroute info into a route structure
 * param line: Pointer to a string need to be parsed
 * return a route structure represents the line info
 */

route_t * route_parse(const char * line);

/*
 * Compare two line in traceroute output file and push the change events into tr_list
 * param cur: Pointer to a string for current route
 * param line: Pointer to a string 
 */

int route_str_cmp(const char * cur, const char * line);

/*
 * Find the most popular route based on total length of duration
 * param tr_list: Pointer to the list to be found
 * return most popular route
 */

route_t * tr_find_pop(list_t * tr_list);

/*
 * Read the traceroute output file and find the route changes events
 * param file: Pointer to the traceroute output file
 * return a newly created list contains route history
 */

list_t * tr_data_read(FILE * file);

/*
 * Print the route change events as well as the most popular route
 * param tr_list: Pointer to the list contain route events
 * param output_file: The file to store output
 */

void tr_data_print(list_t * tr_list, FILE * output_file);


#endif
