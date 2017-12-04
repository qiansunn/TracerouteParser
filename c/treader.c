/*
 *  Treader:     Traceroute result reader
 *
 *  Authors:     Sun Qian <sunnlo@outlook.com>
 *
 *  A reader for traceroute text.
 *  A traceroute consist of a sequence of hops and time information.
 *  This software find out the route change events and output the most popular route.
 *
 */

#include "treader.h"


char * convert_seconds(double duration) {
    char * s = (char*)calloc(64, sizeof(char*));  
    int hr,min,sec;
    if (duration > 3600) {
	min =(int)duration/60;
	sec = fmod((int)duration, 60);
	hr = min/60;
	min = fmod(min, 60);
	sprintf(s, "Route duration: %d hours %d mins %d secs",hr,min,sec);		} else {
	min = (int)duration/60;
	sec = fmod((int)duration, 60);
	sprintf(s, "Route duration: %d mins %d secs",min,sec);
    }
    return s;
}


int ffprintf(FILE *fp1, FILE *fp2, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int rc1 = vfprintf(fp1, fmt, args);
    va_end(args);
    va_start(args, fmt);
    int rc2 = vfprintf(fp2, fmt, args);
    va_end(args);
    assert(rc1 == rc2);
    return rc1;
}


int sockaddr_cmp(struct sockaddr_in * x, struct sockaddr_in * y) {
    if (x == NULL || y == NULL) {
	return 0;
    }
#define CMP(a, b) if (a != b) return a < b ? -1 : 1
    struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
    CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
    //CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
#undef CMP

    return 0;
}


void route_free(void * element) {
    if (element == NULL) {
	return;
    }
    route_t * route = (route_t *)element;
    
    if(route->sa == NULL)
    {
	free(route); 
	return; 
    } 	
    free(route->sa);
    free(route);
    
    return;
}


void route_print(const route_t * route, FILE * output_file) {
    if (route == NULL) {
	return;
    }
    char ip_s[INET_ADDRSTRLEN] = {0};
    char tm_s[64] = {0};

    strftime(tm_s, 32, "Time:%Y/%m/%d %H:%M:%S", &route->time_start);
    ffprintf(stdout, output_file, "%s\t",tm_s);
    ffprintf(stdout, output_file, "Hops:%d\nIPs:", route->counts);
    for(int i=0; i<route->counts; i++) {
	inet_ntop(AF_INET, &(route->sa[i].sin_addr), ip_s, INET_ADDRSTRLEN);
	ffprintf(stdout, output_file, "%s\t", ip_s);
    }
    ffprintf(stdout, output_file, "\n");

    return;
}


int route_cmp(const route_t * r1, const route_t * r2) {
    if(r1 == NULL || r2 == NULL) {
	return 0;
    }

    if(r1->counts != r2->counts) {
	return 1;
    }
    for (int i = 0; i<r1->counts; i++) {
	if(sockaddr_cmp(&r1->sa[i], &r2->sa[i])) {
	    return 1;	
	}
    }

    return 0;
}


route_t * route_parse(const char * line) { 
    if (line == NULL) {
	return NULL;
    }
    char delim[] = " '";
    char ip_buf[LINE_MAX_BUFFER] = {0};
    char tm_buf[14] = {0};
    route_t * route; 
    
    route = calloc(1, sizeof(route_t));
    if(route == NULL)
    {
       return NULL;
    }
    sscanf(line, "%[^','],%d,%[^\n]", tm_buf, &route->counts, ip_buf);
    strptime(tm_buf, "%Y%m%d%H%M%S", &route->time_start);
    route->sa = calloc(route->counts, sizeof(struct sockaddr_in));
    if(route->sa == NULL )
    {
    	free(route);
        return NULL; 
    }
    char * save_ptr = NULL; 
    for (char * token = strtok_r(ip_buf,delim,&save_ptr), i=0;
	 token != NULL; 
	 token = strtok_r(NULL, delim, &save_ptr), i++) {
	if (strcmp("*",token)) {
	    inet_pton(AF_INET, token, &(route->sa[i].sin_addr));
	}
    }

    return route;
}


/* Just compare string in order to speed up */
int route_str_cmp(const char * cur, const char * line) {
    char delim[] = " '";
    bool need_update = false;
    /* Strtok changes character after token */
    char cur_buf[LINE_MAX_BUFFER] = {0},
	 line_buf[LINE_MAX_BUFFER] = {0};
    char * cur_tok,
	 * line_tok;
    char * save_cur = NULL,
	 * save_line = NULL;

    strncpy(cur_buf, cur, sizeof(cur_buf)-1);
    strncpy(line_buf, line, sizeof(line_buf)-1);
    cur_tok = strchr(cur_buf, '\'');
    line_tok = strchr(line_buf, '\'');
    /* Most lines are the same */
    if (!strcmp(cur_tok, line_tok)) {
	return 0;
    }
    cur_tok = strtok_r(cur_tok, delim, &save_cur);
    line_tok = strtok_r(line_tok, delim, &save_line);
    while (NULL != cur_tok || NULL != line_tok) {
	/* String contains with '*' */
	if (strcmp(line_tok, "*")) {
	    if (!strcmp(cur_tok, "*")) {
		need_update = true;
	    } else if (strncmp(cur_tok, line_tok, strlen(line_tok))) {
		return UNEQUAL;
	    }
	}
	/* Update strtok pointers */
	cur_tok += strlen(cur_tok) + 1;
	line_tok += strlen(line_tok) + 1;
	/* Get next token */
	cur_tok = strtok_r(cur_tok, " ", &save_cur);
	line_tok = strtok_r(line_tok, " ", &save_line);
    }
    if (need_update) {
	return UPDATE_FLAG;
    }

    return 0;
}


route_t * tr_find_pop(list_t * tr_list) {
    if(tr_list == NULL) {
	return NULL;
    }
    list_cell_t * list_cell,
		* find_cell,
		* tmp_cell;
    route_t * r1,
	    * r2,
	    * route; 
    
    route = (route_t *)tr_list->head->element;
    route->time_interval = difftime(mktime(&route->time_start), mktime(&route->time_end));
    //Merge time interval if the route is same and free duplicate route records */
    for (list_cell = tr_list->head; list_cell; list_cell = list_cell->next) {
	find_cell = list_cell;
	r1 = (route_t *)list_cell->element;
	r1->time_interval = difftime(mktime(&r1->time_end), mktime(&r1->time_start));
	while(find_cell->next) {
	    r2 = (route_t *)find_cell->next->element;
	    if (!route_cmp(r1, r2)) {
		r2->time_interval = difftime(mktime(&r2->time_end), mktime(&r2->time_start));
		r1->time_interval += r2->time_interval;
		tmp_cell = find_cell->next;
		find_cell->next = tmp_cell->next;
		list_cell_free(tmp_cell, route_free);
	    } else {
		find_cell = find_cell->next;
	    }
	}
	if (route->time_interval < r1->time_interval) {
	    route = r1;
	}
    }

    /* Return route with max time interval */
    return route;
}



list_t * tr_data_read(FILE * file) {
    list_t  * tr_list;
    route_t * route,
	    * pre_route;
    /* Current line */
    char line[LINE_MAX_BUFFER] = {0};
    /* Keep current route record */
    char cur[LINE_MAX_BUFFER] = {0};

    /* Read first line */
    if (fgets(line, sizeof(line), file)) {
	tr_list = list_create();
	if (tr_list == NULL) {
	    return NULL;
	}
	route = route_parse(line);
	g_route_num++;
	list_push_element(tr_list, route);
	strncpy(cur, line, sizeof(cur)-1);
    } else {
	fprintf(stderr, "ERROR: Traceroute file is empty\n");
	exit(EXIT_FAILURE);
    }
    /* Read and compare follows */
    while (fgets(line, sizeof(line), file)) {
	if (line[0] == '\0') {
	    fprintf(stderr, "ERROR: Line too short\n");
	    continue;
	}
	if (strlen(line)-1 > LINE_MAX_BUFFER) {
	    fprintf(stderr, "ERROR: Line starting with '%s' is too long\n", line);
	    continue;
	}
	switch (route_str_cmp(cur, line)) {
	    /* push the new route */
	    case UNEQUAL:
		pre_route = (route_t *)tr_list->tail->element;
		route = route_parse(line);
		memcpy(&pre_route->time_end, &route->time_start, sizeof(struct tm));
		g_route_num++;
		list_push_element(tr_list, route);
		strncpy(cur, line, sizeof(cur)-1);
		break;
	    /*update the timestamp and route of an existing entry in tr_list */
	    case UPDATE_FLAG:
		pre_route = (route_t *)tr_list->tail->element;
		route = route_parse(line);
		for (int i=0; i<pre_route->counts; i++) {
		    if (pre_route->sa[i].sin_addr.s_addr == 0) {	
			memcpy(&pre_route->sa[i], &route->sa[i], sizeof(struct sockaddr_in));
		    }
		}
		route_free(route);
		strncpy(cur, line, sizeof(cur)-1);
		break;
	    default: 
		break;
	}
    }
    /* Update the timestamp for last route */
    pre_route = (route_t *)tr_list->tail->element;
    route = route_parse(line);
    memcpy(&pre_route->time_end, &route->time_start, sizeof(struct tm));
    route_free(route);

    return tr_list;
}


void tr_data_print(list_t * tr_list, FILE * output_file) {
    if (tr_list == NULL) {
	return;
    }
    list_cell_t * list_cell;
    route_t	* route,
		* route_pop;
    int		i;

    /* g_route_num holds the length of tr_list */
    for (list_cell = tr_list->head, i=1; 
	 list_cell; 
	 list_cell = list_cell->next, i++) {
	ffprintf(stdout, output_file, "------------------------------------------------\n");
	route = (route_t *)list_cell->element;
	ffprintf(stdout, output_file, "Route Event[%d]\t", i);	
	route_print(route, output_file);
    }
    route_pop = tr_find_pop(tr_list);
    ffprintf(stdout, output_file, 
	    "================================================\n"
	    "Total route change event number:%d\n", g_route_num);
    if (route_pop != NULL) {
	char ip_s[INET_ADDRSTRLEN] = {0};
	char * tm_ptr;
	
	tm_ptr = convert_seconds(route_pop->time_interval);
	ffprintf(stdout, output_file, 
		"Most popular route:\t");
	ffprintf(stdout, output_file, "%s\t",tm_ptr);
	ffprintf(stdout, output_file, "Hops:%d\nIPs:", route_pop->counts);
	for(int i=0; i<route_pop->counts; i++) {
	    inet_ntop(AF_INET, &(route_pop->sa[i].sin_addr), ip_s, INET_ADDRSTRLEN);
	    ffprintf(stdout, output_file, "%s\t", ip_s);
	}
	ffprintf(stdout, output_file, "\n");
	free(tm_ptr);
    }
    ffprintf(stdout, output_file, "================================================\n");
}


int main(int argc, char *argv[]) {
    
    FILE   * file = NULL,
	   * output_file = NULL;
    list_t * tr_list = NULL;
    char output_name[64] = {0};

    if(argc == 2) {
	file = fopen(argv[1], "r");
	if (!file) {
	    fprintf(stderr, "ERROR: Could not open file: %s\n", argv[1]);
	    return EXIT_FAILURE;
	}
	strncpy(output_name, argv[1], sizeof(output_name)-1);
	strcat(output_name, ".out"); 
	output_file = fopen(output_name, "w");
	if (!output_file) {
	    fprintf(stderr, "ERROR: Could not create output file: %s\n", output_name);
	    return EXIT_FAILURE;
	}
    } else {
	fprintf(stderr, "ERROR: Wrong number of arguments\n"
	"usage: %s <input_file>\n", argv[0]);
	return EXIT_FAILURE;
    }

    tr_list = tr_data_read(file); 
    tr_data_print(tr_list, output_file);
    list_free(tr_list, route_free);
    fclose(file);

    return EXIT_SUCCESS;
}




