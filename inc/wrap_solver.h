#ifndef WRAP_SOLVER_H
#define WRAP_SOLVER_H

#include <stdbool.h>

typedef struct constellation constellation;


constellation *create_constellation(unsigned int num_stars);
void destroy_constellation(constellation* c);
const char* to_alpha(constellation *c, unsigned int node);
void print_constellation(constellation *c);
bool add_edge(constellation *c, unsigned int from_star, unsigned int to_star);
bool add_wrap_counts(constellation *c, int wrap_counts[]);
void update_weights(constellation *c, unsigned int visited_star, int* saved_wrap_counts);
bool has_edge(constellation *c, unsigned int from_star, unsigned int to_star);
unsigned int *get_connected_stars(constellation *c, unsigned int node, int *size);
int validate_solution_sequence(constellation *c, unsigned int* solution_sequence, int length);
void depth_first_search(constellation *c, int **saved_wrap_counts, unsigned int current_star, 
	unsigned int **solved_list_array, int step,  /* int *end_search_flag, */ unsigned int valid_exit_stars[], int exit_stars_num, bool validate);
	void prepare_dfs(constellation *c, unsigned int valid_exit_stars[], int exit_stars_num, bool validate);

#endif
