#pragma once

typedef unsigned int acstate_t;
#define ACSM_FAIL_STATE 0xffffffff

typedef struct acsm_pattern_t {
	struct acsm_pattern_t *next;

	unsigned char *patrn;
	unsigned char *casepatrn;
	int n;
	int nocase;
	int offset;
	int depth;
	void *id;
	int iid;
}acsm_pattern;

typedef struct trans_node_s {
	acstate_t key;
	acstate_t next_state;
	struct trans_node_s *next;
}trans_node_t;

enum {
	ACF_FULL,
	ACF_SPARSE,
	ACF_BANDED,
	ACF_SPARSEBANDS
};

enum {
	FSA_TRIE,
	FSA_NFA,
	FSA_DFA
};

typedef struct acsm_struct_s{
	int max_states;
	int num_states;

	acsm_pattern *patterns;
	acstate_t *failstate;
	acsm_pattern **match_list;

	trans_node_t **transtable;
	
	acstate_t **next_state;
	int format;
	int sparse_max_row_nodes;
	int sparse_max_zcnt;

	int num_trans;
	int alphabet_size;
	int acsm_fsa;
}acsm_struct;

typedef int(*match_func_t)(void *id, int index, void *data);

acsm_struct *acsm_new(void);

void acsm_free(acsm_struct *acsm);

int acsm_add_pattern(acsm_struct *self, unsigned char *pattern, int n, 
	int nocase, int offset, int depth, void *id, int iid);

int acsm_compile(acsm_struct *self);

int acsm_search(acsm_struct *self, unsigned char **pt, int n, match_func_t, void *data, int *current_state);

int acsm_select_format(acsm_struct *self, int format);

int acsm_select_fsa(acsm_struct *self, int fsa);

void acsm_set_max_sparse_band_zeros(acsm_struct *self, int n);
void acsm_set_max_sparse_elements(acsm_struct *self, int n);
int acsm_set_alphabet_size(acsm_struct *self, int n);
void acsm_set_verbose(void);

void acsm_print_info(acsm_struct *self);

int acsm_print_detail_info(acsm_struct *self);
int acsm_print_summary_info(void);