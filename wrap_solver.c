#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "wrap_solver.h"


#define MAX_DEPTH 100 // max depth for our recursion

struct constellation {
	unsigned int num_stars;
	bool **edges;
	int *wrap_counts;
};

constellation *create_constellation(unsigned int num_stars) {
	constellation *c = malloc(sizeof(*c));
	if (c == NULL){
		return NULL;
	}

	c->num_stars = num_stars;

	c->edges = calloc(sizeof(bool*), c->num_stars);
	if (c->edges == NULL){
		free(c);
		return NULL;
	}

	for (unsigned int i=0; i < c->num_stars; i++){
		c->edges[i] = calloc(sizeof(bool), c->num_stars);
		if (c->edges[i] == NULL) {
			destroy_constellation(c);
			return NULL;
		}
	}

	return c;
}

void destroy_constellation(constellation* c) {
	if (c->edges == NULL){
		return;
	}

	for (unsigned int i=0; i < c->num_stars; i++){
		if (c->edges[i] != NULL){
			free(c->edges[i]);
		}
	}
	free(c->edges);
	free(c);
}

const char* to_alpha(constellation *c, unsigned int star){
	assert( c != NULL);

	if (star > 25){
		char *name = NULL;
		sprintf( name, "%d - %d", star, c->wrap_counts[star-1]);
		return name;
	}
	if (star == 0){
		char *name = "start - NULL";
		return name;
	}

	char alphabet[26] = "abcdefghijklmnopqrstuvwxyz";
	for (unsigned int i=0; i < (int) sizeof(alphabet); i++){
		if (star == i+1){
			char *name = (char*)malloc(2 * sizeof(char));
			sprintf( name, "%c - %d", alphabet[i], c->wrap_counts[star-1]);
			return name;
		}
	}

	char *name = "null";
	return name;

}

void print_constellation(constellation *c) {
	printf("graph constellation{\n");
	printf("nodesep=1.0;\n");

	for (unsigned int from=0; from < c->num_stars; from++){
		for (unsigned int to=0; to < c->num_stars; to++){
			if (c->edges[from][to]){
				printf("\"%s\" -- \"%s\";\n", to_alpha(c, from), to_alpha(c, to));
			}
		}
	}
	printf("}\n");
}

bool add_edge(constellation *c, unsigned int from_star, unsigned int to_star) {
	assert(c != NULL);
	assert(from_star < c->num_stars);
	assert(to_star < c->num_stars);

	if (has_edge(c, from_star, to_star)) {
		return false;
	}

	c->edges[from_star][to_star] = true;
	return true;
}

bool has_edge(constellation *c, unsigned int from_star, unsigned int to_star) {
	assert(c != NULL);
	assert(from_star < c->num_stars);
	assert(to_star < c->num_stars);
	return c->edges[from_star][to_star];
}

bool add_wrap_counts(constellation *c, int wrap_counts[]){
	assert(c != NULL);

	c->wrap_counts = (int*)malloc(sizeof(unsigned int) * c->num_stars - 1);
	for (unsigned int i = 0; i < c->num_stars - 1; i++){
		c->wrap_counts[i] = wrap_counts[i];
	}
	return true;
}

// Subtract 1 for current visited and surrounding stars
void update_weights(constellation *c, unsigned int visited_star, int* saved_wrap_counts){
	assert(c != NULL);

	// Only update wrap counts for current star if it's not the start star 
	if (visited_star != 0){
		// (visited_star -1) because star value is decremented by 1 when considering stars with wrap counts
		saved_wrap_counts[visited_star - 1] -= 1;
	}
	// START star doesn't impact other stars
	else {
		return;
	}

	// we use this array to make sure we only updates stars once
	bool star_updated[c->num_stars - 1];
	for (unsigned int i = 0; i < (c->num_stars - 1); i++) {
    	star_updated[i] = false;
    }
	
	// loop through our edges
	for (unsigned int from=0; from < c->num_stars; from++){
		for (unsigned int to=0; to < c->num_stars; to++){
			if (c->edges[from][to]){
				// fetch each star that has a connection to the visited star
				int connected_star = (from == visited_star) ? (int)to : (to == visited_star ? (int)from : -1);

				// only update the wrap count if the star hasn't already been updated
				if (connected_star > 0 && !star_updated[connected_star - 1]){ 
					saved_wrap_counts[connected_star - 1] -= 1;
					star_updated[connected_star - 1] = true;
				}
			}
		}
	}
}


// find all stars that have an edge to current stars
unsigned int *get_connected_stars(constellation *c, unsigned int star, int *size){
	unsigned int *stars = NULL;
	int count = 0;
	for (unsigned int to = 0; to < c->num_stars; to++){
		for (unsigned int from = 0; from < c->num_stars; from++){
			if (c->edges[to][from]){
				if (to == star){
					unsigned int *new_stars = realloc(stars, (unsigned long)(count + 1) * sizeof(unsigned int));
					stars = new_stars;
					stars[count] = from;
					count++;
				}
				if (from == star){
					unsigned int *new_stars = realloc(stars, (unsigned long)(count + 1) * sizeof(unsigned int));
					stars = new_stars;
					stars[count] = to;
					count++;
				}
			}
		}
	}

	*size = count;
	return stars;
}

// Check solution sequence for validity through various heuristics
int validate_solution_sequence(constellation *c, unsigned int* solution_sequence, int length){
	// Sequence can't have two consecutive star visits: e.g., 2,4,2,4. There is an 
	// exception for the start star: e.g., 0, 2, 0, 2
	for (int i = 0; i < length; i++){

		// // Popular star principle: If the star with the most edges already has more than one edge
		// // activated, then the subsequent step cannot go towards an exit star
		// // FAITH: LOW
		// if (is_popular_star(c, solution_sequence[i])){
		// 	int edge_count = 0
		// 	for (int j = 0; j < i; j++){
		// 		if (solution_sequence[j] == solution_sequence[i]){
		// 			edge_count++;
		// 		}
		// 	}
		// 	if (edge_count > 1){
		// 		if(is_exit_star(c, solution_sequence[i+1])){
		// 			return -1;
		// 		}
		// 	}
		// }


		if (i >= 3){
			unsigned int fourth_step = (unsigned int)solution_sequence[i];
			unsigned int third_step = (unsigned int)solution_sequence[i-1];
			unsigned int second_step = (unsigned int)solution_sequence[i-2];
			unsigned int first_step = (unsigned int)solution_sequence[i-3];

			if (fourth_step == second_step && third_step == first_step){
				if (i - 3 != 0 && second_step != 0){
					return -1;
				}
			}

			// Sequence can't "hook" a star that has an edge already included in the sequence
			// e.g.: 3, 0, .... , 2, 0, 3, 0 ...
			// 	* The sequence is "hooking" star 3 here, even though it was visited before
			// FAITH: HIGH
			if (fourth_step == second_step){
				// int connected_stars_size;
				// // retrieving the connected stars for the hooked star
				// unsigned int *connected_stars = get_connected_stars(c, third_step, &connected_stars_size);
				// int edge_count = 0;
				// for (int j = 0; j < connected_stars_size; j++){
				// 	if(connected_stars[j] == fourth_step){
				// 		edge_count++;
				// 	}
				// }
				// if (edge_count > 1){
				// 	return -1;
				// }

				for (int j = 0; j < (i - 4); j++){
					if (solution_sequence[j] == third_step){
						return -1;
					}
				}
			}

			// Triangle principle: In the beginning, if the fourth and first step are the same, and the third step's
			// edges are exhausted, then this is an invalid path
			// FAITH: MEDIUM
			if ( i==3 ){
				if (fourth_step == first_step){
					int connected_stars_size;
					unsigned int *connected_stars = get_connected_stars(c, third_step, &connected_stars_size);
					int edge_count = 0;
					for (int x = 0; x < connected_stars_size; x++){
						for (int y=0; y < length; y++){
							if (solution_sequence[y] == connected_stars[x]){
								edge_count++;
							}
						}
					}
					if (edge_count >= connected_stars_size){
						return -1;
					}
				}
			}

		}
	}
	return 0;
}

// DFS algorithm in order to find a sequence that produces a zero-graph
void depth_first_search(constellation *c, int **saved_wrap_counts, unsigned int current_star, 
	unsigned int **solved_list_array, int step,  /* int *end_search_flag, */ unsigned int valid_exit_stars[], int exit_stars_num, bool validate){
	// checking if graph is solved or invalid
	unsigned int zero_count = 0;
	unsigned int neg_flag = 0;

	// our flag to know when we have just ended a search
	static bool end_search_flag = false;

	// check wrap counts for validity or solution
	for (unsigned int i = 0; i < c->num_stars - 1; i++){

		// Accumulate the pillars with value 0
		if (saved_wrap_counts[step][i] == 0){
			zero_count++;
		}
		// A pillar has a negative value, this sequence is invalid
		if (saved_wrap_counts[step][i] < 0){
			neg_flag = 1;
		}
	}

	// check for --validate flag
	if (validate){ 
		if (validate_solution_sequence(c, *solved_list_array, step)){
			// Invalid Sequence, Ending this search
			goto end_search;

		}
	}

	// Each pillar has a value of 0, we have a solution candidate
	if (zero_count == c->num_stars - 1){

		// check for --validate flag
		if (validate){

			// Ensure that the solution sequence ends with a Valid Exit Star
			// Valid Exit Star is defined as a star whose orientation allows to exit
			// the level through the doorway.
			bool valid_flag = false;
			for (int j = 0; j < exit_stars_num; j++){
				if ( (*solved_list_array)[step-1] == valid_exit_stars[j]) {
					valid_flag = true;
				}
			}

			// Invalid sequence, ending this search..
			if (!valid_flag){
				// Ensure we're restoring properly
				goto end_search;
			}
		}

		// Printing out the solution
		printf("Solution Sequence: ");
		for (int k = 0; k < step; k++){
			printf("%d", (*solved_list_array)[k]);

			if(k < step - 1) {
				printf(" -> ");
			}
		}
		printf("\n");
		goto end_search;
	}

	// Invalid graph, ending this search...
	if (neg_flag) {
		for (unsigned int m = 0; m < c->num_stars; m++) {
             saved_wrap_counts[step][m] = saved_wrap_counts[step-1][m]; // Ensure you restore properly
        }
		goto end_search;

	}

	// gather the connected stars to the current star
	int connected_stars_size;
	unsigned int *connected_stars = get_connected_stars(c, current_star, &connected_stars_size);

	// Loop through each connected star, thus producing all possible sequences in the graph
	for (int p = 0; p < connected_stars_size; p++){

		// We just came from a terminated sequence, so make sure we decrement the step
		if (end_search_flag) step--;
		
		// we hit max_depth, break out of the loop
		if (step + 1 >= MAX_DEPTH) { 
			printf("Error: Max recursion depth reached.\n");
			break;
		}

		saved_wrap_counts[step+1] = (int *)malloc(c->num_stars * sizeof(int));
		for (unsigned int q = 0; q < c->num_stars; q++) {
		 	saved_wrap_counts[step+1][q] = saved_wrap_counts[step][q];
		}

		update_weights(c, connected_stars[p], saved_wrap_counts[step + 1]);

		unsigned int *temp_array = (unsigned int *)realloc(*solved_list_array, (unsigned long)(step + 1) * sizeof(unsigned int));
		if (temp_array == NULL) {
			printf("Error: failed to realloc for our solved_list_array\n");
			return;
		} else {
			*solved_list_array = temp_array;
			(*solved_list_array)[step] = connected_stars[p]; 
		}

		step++;
		end_search_flag = false;

		depth_first_search(c, saved_wrap_counts, connected_stars[p], solved_list_array, step, /* end_search_flag, */valid_exit_stars, exit_stars_num, validate);
	}

	free(connected_stars);

// Wrapping up the search by restoring the wrap counts and setting our flag
end_search:
	// rolling back the most recent step in this search to continue other searches
	for (unsigned int x = 0; x < c->num_stars; x++) {
         saved_wrap_counts[step][x] = saved_wrap_counts[step-1][x];
    }
	end_search_flag = true;
	return;
}


// Prepare for the recursive depth-first search algorithm 
void prepare_dfs(constellation *c, unsigned int valid_exit_stars[], int exit_stars_num, bool validate){
	
	// A 2-D array that will hold the state of the constellation's wrap count after each traversal
	int **saved_wrap_counts = (int **)malloc(MAX_DEPTH * sizeof (int *));
	if (!saved_wrap_counts) {
		printf("Error: Memory allocation failed for saved_wrap_counts.\n");
		return;
	}

	saved_wrap_counts[0] = (int *)malloc((((c->num_stars) - 1)) * sizeof(int));
	if (!saved_wrap_counts[0]) {
		printf("Error: Memory allocation failed for saved_wrap_counts[0].\n");
		free(saved_wrap_counts);
		return;
	}

	unsigned int *solved_list_array = (unsigned int *)malloc(1 * sizeof(int));
	if (!solved_list_array) {
		printf("Error: Memory allocation failed for solved_list_array.\n");
		free(saved_wrap_counts[0]);
		free(saved_wrap_counts);
		return;
	}

	for (unsigned int i = 0; i < c->num_stars; i++) {
		saved_wrap_counts[0][i] = c->wrap_counts[i];
	}
	depth_first_search(c, saved_wrap_counts, 0, &solved_list_array, 0, valid_exit_stars, exit_stars_num, validate);

	for (int j = 0; j < MAX_DEPTH; j++) {
		if (saved_wrap_counts[j]){
			free(saved_wrap_counts[j]);
		}
	}

	free(saved_wrap_counts);
	free(solved_list_array);
}


int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Usage: %s <level> [--validate | --print]\n", argv[0]);
		return -1;
	}

	int level = atoi(argv[1]);

	if (level < 1 || level > 3){ 
		printf("Error: Level must be between 1-3\n");
		return 1;
	}

	bool validate = false;
	bool print = false;
	if (argc >= 3 && strcmp(argv[2], "--validate") == 0){
		validate = true;
	}
	else if (argc >= 3 && strcmp(argv[2], "--print") == 0){
		print = true;
	}

	switch(level) {

		// Level 1
		case 1:  {
			constellation *level_1 = create_constellation(10);
			int level_1_wrap_counts[] = {1, 4, 3, 5, 6, 7, 5, 6, 4};
			unsigned int level_1_valid_exit_stars[] = {3, 4, 5, 7};
			int level_1_exit_stars_num = 4;
			add_wrap_counts(level_1, level_1_wrap_counts);
			add_edge(level_1, 0, 1);
			add_edge(level_1, 0, 4);
			add_edge(level_1, 1, 2);
			add_edge(level_1, 2, 3);
			add_edge(level_1, 2, 4);
			add_edge(level_1, 3, 4);
			add_edge(level_1, 4, 5);
			add_edge(level_1, 4, 6);
			add_edge(level_1, 5, 6);
			add_edge(level_1, 5, 7);
			add_edge(level_1, 6, 8);
			add_edge(level_1, 6, 9);
			add_edge(level_1, 7, 8);
			add_edge(level_1, 8, 9);
			if (print) {
				print_constellation(level_1);
			} else {
				prepare_dfs(level_1, level_1_valid_exit_stars, level_1_exit_stars_num, validate);
			}
			destroy_constellation(level_1);
			break;
		}

		// Level 2
		case 2: {
			constellation *level_2 = create_constellation(10);
			int level_2_wrap_counts[] = {5, 7, 7, 7, 8, 7, 6, 5, 4};
			unsigned int level_2_valid_exit_stars[] = {0, 1, 3, 7, 9};
			int level_2_exit_stars_num = 5;
			add_wrap_counts(level_2, level_2_wrap_counts);
			add_edge(level_2, 0, 2);
			add_edge(level_2, 0, 7);
			add_edge(level_2, 1, 3);
			add_edge(level_2, 1, 2);
			add_edge(level_2, 2, 3);
			add_edge(level_2, 3, 4);
			add_edge(level_2, 4, 5);
			add_edge(level_2, 2, 5);
			add_edge(level_2, 5, 7);
			add_edge(level_2, 5, 6);
			add_edge(level_2, 4, 6);
			add_edge(level_2, 6, 8);
			add_edge(level_2, 7, 8);
			add_edge(level_2, 7, 9);
			add_edge(level_2, 8, 9);
			if (print){
				print_constellation(level_2);
			} else {
				prepare_dfs(level_2, level_2_valid_exit_stars, level_2_exit_stars_num, validate);
			}
			destroy_constellation(level_2);
			break;
		}

		// Level 3
		case 3: {
			constellation *level_3 = create_constellation(10);
			int level_3_wrap_counts[] = {4, 6, 6, 5, 5, 5, 3, 5, 5};
			unsigned int level_3_valid_exit_stars[] = {4, 5, 6};
			int level_3_exit_stars_num = 3;
			add_wrap_counts(level_3, level_3_wrap_counts);
			add_edge(level_3, 0, 1);
			add_edge(level_3, 0, 2);
			add_edge(level_3, 0, 3);
			add_edge(level_3, 0, 9);
			add_edge(level_3, 1, 2);
			add_edge(level_3, 1, 8);
			add_edge(level_3, 2, 3);
			add_edge(level_3, 3, 4);
			add_edge(level_3, 4, 5);
			add_edge(level_3, 5, 6);
			add_edge(level_3, 5, 9);
			add_edge(level_3, 6, 7);
			add_edge(level_3, 6, 9);
			add_edge(level_3, 7, 8);
			add_edge(level_3, 8, 9);
			if (print){
				print_constellation(level_3);
			} else {
				prepare_dfs(level_3, level_3_valid_exit_stars, level_3_exit_stars_num, validate);
			}
			destroy_constellation(level_3);
			break;
		}
		default: 
			break;
	}
}
