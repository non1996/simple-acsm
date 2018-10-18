#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h> 
#include "acsm.h"

#define nullptr NULL

static int max_memory = 0;
static int num_alloc = 0;

static int s_verbose = 0;

typedef struct acsm_summary_s {
	unsigned num_states;
	unsigned num_transitions;
	acsm_struct acsm;
}acsm_summary_t;

acsm_struct * acsm_new(void) {
	return nullptr;
}

static acsm_summary_t summary;

static unsigned char xlatcase[256];
static int xlatcase_inited;

static inline void init_xlatcase() {
	int i;

	if (xlatcase_inited)
		return;

	xlatcase_inited = 1;

	for (i = 0; i < 256; ++i)
		xlatcase[i] = (unsigned char)toupper(i);
}

//	大小写转换
static inline void convert_case_ex(unsigned char *dest, unsigned char *src, int size) {
	int i;
#ifdef  XXXX
	int n;
	n = size & 0x3u;
	size >>= 2;
	for (i = 0; i < size; ++i) {
		dest[0] = xlatcase[src[0]];
		dest[1] = xlatcase[src[1]];
		dest[2] = xlatcase[src[2]];
		dest[3] = xlatcase[src[3]];
		dest += 4;
		src += 4;
	} 
	for (i = 0; i < n; i++) {
		dest[i] = xlatcase[s[i]];
	}
#else
	for (i = 0; i < size; i++) {
		dest[i] = xlatcase[src[i]];
	}
#endif //  XXXX
}

void acsm_set_verbose(void) {
	s_verbose = 1;
}

static void *ac_malloc(int n) {
	void *p;
	p = calloc(1, n);

	if (p) {
		max_memory += ((n + 0x0f) & ~0x0f) + 8;
		num_alloc++;
	}
	return p;
}

static void ac_free(void *p) {
	if (p)
		free(p);
}

//	简单队列
typedef struct qnode_s {
	int state;
	struct qnode_s *next;
}qnode;

typedef struct queue_s {
	qnode *head, *tail;
	int count;
}queue;

static void queue_init(queue *self) {
	self->head = self->tail = nullptr;
	self->count = 0;
}

static int queue_find(queue *self, int state) {
	qnode * q;
	q = self->head;
	while (q) {
		if (q->state == state) return 1;
		q = q->next;
	}
	return 0;
}

static void
queue_add(queue * self, int state) {
	qnode * q;

	if (queue_find(self, state)) return;

	if (!self->head) {
		q = self->tail = self->head = (qnode *)ac_malloc(sizeof(qnode));
		MEMASSERT(q, "queue_add");
		q->state = state;
		q->next = 0;
	}
	else {
		q = (qnode *)ac_malloc(sizeof(qnode));
		q->state = state;
		q->next = 0;
		self->tail->next = q;
		self->tail = q;
	}
	self->count++;
}

// pop_front
static int
queue_remove(queue * self) {
	int state = 0;
	qnode * q;
	if (self->head) {
		q = self->head;
		state = q->state;
		self->head = self->head->next;
		self->count--;

		if (!self->head) {
			self->tail = 0;
			self->count = 0;
		}
		ac_free(q);
	}
	return state;
}

static int
queue_count(queue * self) {
	return self->count;
}

static void
queue_free(queue * s) {
	while (queue_count(s)) {
		queue_remove(s);
	}
}

static int list_get_next_state_nfa(acsm_struct *acsm, int state, int input) {
	trans_node_t *t = acsm->transtable[state];

	while (t) {
		if (t->key == input)
			return t->next_state;
		t = t->next;
	}
	if (state == 0) return 0;
	return ACSM_FAIL_STATE;
}

static int list_get_next_state_dfa(acsm_struct *acsm, int state, int input) {
	trans_node_t *t = acsm->transtable[state];

	while (t) {
		if (t->key == input)
			return t->next_state;
		t = t->next;
	}
	return 0;
}

static int list_put_next_state(acsm_struct *acsm, int state, int input, int next_state) {
	trans_node_t *p;
	trans_node_t *tnew;

	p = acsm->transtable[state];
	while (p) {
		if (p->key == input) {
			p->next_state = next_state;
			return 0;
		}
		p = p->next;
	}

	tnew = (trans_node_t*)ac_malloc(sizeof(trans_node_t));

	if (!tnew) return -1;

	tnew->key = input;
	tnew->next_state = next_state;
	tnew->next = 0;

	tnew->next = acsm->transtable[state];
	acsm->transtable[state] = tnew;
	acsm->num_trans++;
	return 0;
}

static int list_free_transtable(acsm_struct *acsm) {
	int i;
	trans_node_t *t, *p;

	if (!acsm->transtable)	return 0;

	for (i = 0; i < acsm->max_states; ++i) {
		t = acsm->transtable[i];

		while (t) {
			p = t->next;
			free(t);
			t = p;
			max_memory -= sizeof(trans_node_t);
		}
	}

	free(acsm->transtable);
	max_memory -= sizeof(void*) * acsm->max_states;
	acsm->transtable = nullptr;
}

static
int List_PrintTransTable(acsm_struct * acsm) {
	int i;
	trans_node_t * t;
	acsm_pattern * patrn;

	if (!acsm->transtable) return 0;

	printf("Print Transition Table- %d active states\n", acsm->num_states);

	for (i = 0; i< acsm->num_states; i++) {
		t = acsm->transtable[i];

		printf("state %3d: ", i);

		while (t) {
			if (isprint(t->key))
				printf("%3c->%-5d\t", t->key, t->next_state);
			else
				printf("%3d->%-5d\t", t->key, t->next_state);

			t = t->next;
		}

		patrn = acsm->match_list[i];

		while (patrn) {
			printf("%.*s ", patrn->n, patrn->patrn);

			patrn = patrn->next;
		}

		printf("\n");
	}
	return 0;
}

//	将trans_node_t链表转换为acstate稀疏数组full。
static int list_conv_to_full(acsm_struct * acsm, acstate_t state, acstate_t * full) {
	int tcnt = 0;
	trans_node_t * t = acsm->transtable[state];

	memset(full, 0, sizeof(acstate_t) * acsm->alphabet_size);

	if (!t) return 0;

	while (t) {
		full[t->key] = t->next_state;
		tcnt++;
		t = t->next;
	}
	return tcnt;
}

//	复制axsm_pattern结构体，但是不复制其保存的字符串
static acsm_pattern *copy_match_list_entry(acsm_pattern *px) {
	acsm_pattern *p;

	p = (acsm_pattern*)ac_malloc(sizeof(px));
	memcpy(p, px, sizeof(acsm_pattern));
	p->next = nullptr;
	return p;
}

//	将一个模式添加到终结于state的模式列表中	
static void add_match_list_entry(acsm_struct *acsm, int state, acsm_pattern *px) {
	acsm_pattern *p;
	p = copy_match_list_entry(px);
	p->next = acsm->match_list[state];
	acsm->match_list[state] = p;
}

//	对于模式p，判断要添加多少节点，并将这些节点添加到对应位置
static void add_pattern_states(acsm_struct *acsm, acsm_pattern *p) {
	int state, next, n;
	unsigned char *pattern;

	n = p->n;
	pattern = p->patrn;
	state = 0;

	for (; n > 0; pattern++, n--) {
		next = list_get_next_state_nfa(acsm, state, *pattern);
		if (next == ACSM_FAIL_STATE || next == 0) {
			break;
		}
		state = next;
	}

	for (; n > 0; pattern++, n--) {
		acsm->num_states++;
		list_put_next_state(acsm, state, *pattern, acsm->num_states);
		state = acsm->num_states;
	}

	add_match_list_entry(acsm, state, p);
}

//	构建fail指针
static void build_nfa(acsm_struct *acsm) {
	int r, s, i;
	queue q, *queue = &q;
	acstate_t *fail_state = acsm->failstate;
	acsm_pattern **match_list = acsm->match_list;
	acsm_pattern *mlist, *px;

	queue_init(queue);

	//	根节点的子节点的fail指针均指向根节点
	for (i = 0; i < acsm->alphabet_size; ++i) {
		s = list_get_next_state_dfa(acsm, 0, i);
		if (s) {
			queue_add(queue, s);
			fail_state[s] = 0;
		}
	}

	//	宽搜，对后续节点构建fail指针
	while (queue_count(queue) > 0) {
		r = queue_remove(queue);
		for (i = 0; i < acsm->alphabet_size; ++i) {
			int fs, next;
			s = list_get_next_state_nfa(acsm, r, i);

			if (s != ACSM_FAIL_STATE) {
				queue_add(queue, s);
				fs = fail_state[r];

				//	沿着父节点的fail向上找
				while ((next = list_get_next_state_nfa(acsm, fs, i)) == ACSM_FAIL_STATE)
					fs = fail_state[fs];

				fail_state[s] = next;

				for (mlist = match_list[next]; mlist; mlist = mlist->next) {
					px = copy_match_list_entry(mlist);
					px->next = match_list[s];
					match_list[s] = px;
				}
			}	
		}
	}
	queue_free(queue);
}

static void convert_nfa_to_dfa(acsm_struct *acsm) {
	int i, r, s, c_failstate;
	queue q, *queue = &q;
	acstate_t *failstate = acsm->failstate;

	queue_init(queue);

	for (i = 0; i < acsm->alphabet_size; ++i) {
		s = list_get_next_state_nfa(acsm, 0, i);
		if (s != 0) {	//	&& s != ACSM_FAIL_STATE
			queue_add(queue, s);
		}
	}

	while (queue_count(queue) > 0) {
		r = queue_remove(queue);
		for (i = 0; i < acsm->alphabet_size; ++i) {
			s = list_get_next_state_nfa(acsm, r, i);
			if (s != ACSM_FAIL_STATE && s != 0)
				queue_add(queue, s);
			else {
				c_failstate = list_get_next_state_nfa(acsm, failstate[r], i);
				if (c_failstate != 0 && c_failstate != ACSM_FAIL_STATE)
					list_put_next_state(acsm, r, i, c_failstate);
			}
		}
	}
	queue_free(queue);
}

//	将一组行列表生成满数组格式
static int conv_list_to_full(acsm_struct *acsm) {
	int k;
	acstate_t *p;
	acstate_t **next_state = acsm->next_state;

	for (k = 0; k < acsm->max_states; ++k) {
		p = (acstate_t*)ac_malloc(sizeof(acstate_t) * (acsm->alphabet_size + 2));

		list_conv_to_full(acsm, (acstate_t)k, p + 2); 
		p[0] = ACF_FULL;
		p[1] = 0;

		next_state[k] = p;
	}
	return 0;
}

//	将DFA存储从列表转换为稀疏行存储
static int conv_full_dfa_to_sparse(acsm_struct *acsm) {
	int cnt, m, k, i;
	acstate_t *p, state, maxstates = 0;
	acstate_t **next_state = acsm->next_state;
	acstate_t full[MAX_ALPHABET_SIZE];

	for (k = 0; k < acsm->max_states; k++) {
		cnt = 0;
		list_conv_to_full(acsm, (acstate_t)k, full);
		for (i = 0; i < acsm->alphabet_size; ++i) {
			state = full[i];
			if (state != 0 && state != ACSM_FAIL_STATE)
				cnt++;
		}

		if (cnt > 0) maxstates++;

		//	当根节点或者某个节点后续跳转节点较多时，使用满数组存储跳转条件，
		//	next_state = state:full[2 + input_key]，查找效率为O(1)
		if (k == 0 || cnt > acsm->sparse_max_row_nodes) {
			p = (acstate_t*)ac_malloc(sizeof(acstate_t) * (acsm->alphabet_size + 2));

			p[0] = ACF_FULL;
			p[1] = 0;
			memcpy(&p[2], full, acsm->alphabet_size * sizeof(acstate_t));
		}
		//	否则使用稀疏数组，查找效率O(n)。
		else {
			p = (acstate_t *)AC_MALLOC(sizeof(acstate_t)*(3 + 2 * cnt));
			
			m = 0;
			p[m++] = ACF_SPARSE;
			p[m++] = 0;
			p[m++] = cnt;

			for (i = 0; i < acsm->alphabet_size; ++i) {
				state = full[i];
				if (state != 0 && state != ACSM_FAIL_STATE) {
					p[m++] = i;
					p[m++] = state;
				}
			}
		}
		next_state[k] = p;
	}
}

//	将DFA存储从列表转换为带状数组存储
static int conv_full_dfa_to_banded(acsm_struct *acsm) {
	int first = -1, last;
	acstate_t *p, state, full[MAX_ALPHABET_SIZE];
	acstate_t **nextstate = acsm->next_state;
	int cnt, m, k, i;

	for (k = 0; k < acsm->max_states; ++k) {
		cnt = 0;
		list_conv_to_full(acsm, (acstate_t)k, full);

		first = -1;
		last = -2;

		for (i = 0; i < acsm->alphabet_size; ++i) {
			state = full[i];
			if (state != 0 && state != ACSM_FAIL_STATE) {
				if (first < 0) 
					first = i;
				last = i;
			}
		}

		cnt = last - first + 1;

		p = (acstate_t*)ac_malloc(sizeof(acstate_t) * (4 + cnt));

		m = 0;
		p[m++] = ACF_BANDED;
		p[m++] = 0;
		p[m++] = cnt;
		p[m++] = first;

		for (i = first; i <= last; ++i)
			p[m++] = full[i];

		nextstate[k] = p;
	}
	return 0;
}

int acsm_select_format(acsm_struct * acsm, int m) {
	switch (m) {
	case ACF_FULL:
	case ACF_SPARSE:
	case ACF_BANDED:
	case ACF_SPARSEBANDS:
		acsm->format = m;
		break;
	default:
		return -1;
	}

	return 0;
}

int acsm_select_fsa(acsm_struct *acsm, int m) {
	switch (m) {
	case FSA_TRIE:
	case FSA_NFA:
	case FSA_DFA:
		acsm->acsm_fsa = m;
	default:
		return -1;
	}
}

int acsm_set_alphabet_size(acsm_struct *acsm, int n) {
	if (n <= MAX_ALPHABET_SIZE) {
		acsm->alphabet_size = n;
	}
	else {
		return -1;
	}
	return 0;
}

acsm_struct *acsm_new() {
	acsm_struct *p;
	init_xlatcase();
	p = (acsm_struct*)ac_malloc(sizeof(acsm_struct));

	memset(p, 0, sizeof(acsm_struct));

	p->acsm_fsa = FSA_DFA;
	p->format = ACF_FULL;
	p->alphabet_size = 256;
	p->sparse_max_row_nodes = 256;
	p->sparse_max_zcnt = 10;
	return p;
}

int acsm_add_pattern(acsm_struct * self, unsigned char * pattern, int n, int nocase, int offset, int depth, void * id, int iid) {
	acsm_pattern *plist;

	plist = (acsm_pattern*)ac_malloc(sizeof(acsm_pattern));
	
	plist->patrn = (unsigned char*)ac_malloc(n);

	convert_case_ex(plist->patrn, pattern, n);

	plist->casepatrn = (unsigned char *)AC_MALLOC(n);

	memcpy(plist->casepatrn, pattern, n);

	plist->n = n;
	plist->nocase = nocase;
	plist->offset = offset;
	plist->depth = depth;
	plist->id = id;
	plist->iid = iid;

	plist->next = self->patterns;
	self->patterns = plist;

	return 0;
}

static void acsm_update_match_states(acsm_struct *acsm) {
	acstate_t state;
	acstate_t **nextstate = acsm->next_state;
	acsm_pattern **match_list = acsm->match_list;

	for (state = 0; state < acsm->num_states; ++state) {
		if (match_list[state])
			nextstate[state][1] = 1;
		else
			nextstate[state][1] = 0;
	}
}

int acsm_compile(acsm_struct *acsm) {
	int k;
	acsm_pattern *plist;

	for (plist = acsm->patterns; plist != nullptr; plist = plist->next)
		acsm->max_states += plist->n * 2;

	acsm->max_states++;

	acsm->transtable = (trans_node_t**)AC_MALLOC(sizeof(trans_node_t*) * acsm->max_states);
	memset(acsm->transtable, 0, sizeof(trans_node_t*) * acsm->max_states);

	acsm->failstate = (acstate_t*)AC_MALLOC(sizeof(acstate_t) * acsm->max_states);
	memset(acsm->failstate, 0, sizeof(acstate_t) * acsm->max_states);

	acsm->match_list = (acsm_pattern**)AC_MALLOC(sizeof(acsm_pattern*) * acsm->max_states);
	memset(acsm->match_list, 0, sizeof(acsm_pattern*) * acsm->max_states);

	acsm->next_state = (acstate_t**)AC_MALLOC(acsm->max_states * sizeof(acstate_t*));

	for (k = 0; k < acsm->max_states; ++k)
		acsm->next_state[k] = (acstate_t*)0;

	acsm->num_states = 0;

	for (plist = acsm->patterns; plist != nullptr; plist = plist->next)
		add_pattern_states(acsm, plist);

	acsm->num_states++;

	if (acsm->acsm_fsa == FSA_DFA || acsm->acsm_fsa == FSA_NFA)
		build_nfa(acsm);

	if (acsm->acsm_fsa == FSA_DFA)
		convert_nfa_to_dfa(acsm);

	if (acsm->format == ACF_SPARSE) {
		if (conv_full_dfa_to_sparse(acsm))
			return -1;
	}
	else if (acsm->format == ACF_BANDED) {
		if (conv_full_dfa_to_banded(acsm))
			return -1;
	}
	else if (acsm->format == ACF_SPARSEBANDS) {
		if (conv_full_dfa_to_sparsebands(acsm))
			return -1;
	}
	else if (acsm->format == ACF_FULL) {
		if (conv_list_to_full(acsm))
			return -1;
	}
		
	acsm_update_match_states(acsm);

	list_free_transtable(acsm);

	summary.num_states += acsm->num_states;
	summary.num_transitions += acsm->num_trans;
}

static inline int acsmSearchSparseDFA_Banded(acsm_struct *acsm, unsigned char **ptx, int n,
	int(*match)(void *id, int index, void *data), void *data, int *current_state) {

	acstate_t state;
	unsigned char *Tend, *T, *Tx;
	int sindex;
	int index;
	acstate_t **nextstate = acsm->next_state;
	acsm_pattern **matchlist = acsm->match_list;
	acsm_pattern *mlist;
	acstate_t *ps;
	int nfound = 0;

	Tx = *ptx;
	T = Tx;
	Tend = T + n;

	if (!current_state)
		return 0;

	state = *current_state;

	for (; T < Tend; ++T) {
		ps = nextstate[state];
		sindex = xlatcase[T[0]];

		/* test if this state has any matching patterns */
		if (ps[1]) {
			for (mlist = matchlist[state]; mlist != nullptr; mlist = mlist->next) {
				index = T - mlist->n - Tx;
				if (mlist->nocase) {
					index = T - mlist->n - Tx;
					if (mlist->nocase) {
						nfound++;
						if (match(mlist->id, index, data) > 0) {
							*current_state = state;
							*ptx = T;
							return nfound;
						}
					}
				}
				else {
					if (memcmp(mlist->casepatrn, Tx + index, mlist->n) == 0) {
						nfound++;
						if (match(mlist->id, index, data) > 0) {
							*current_state = state;
							*ptx = T;
							return nfound;
						}
					}
				}
			}
		}

		if (sindex <   ps[3])  state = 0;
		else if (sindex >= (ps[3] + ps[2]))  state = 0;
		else                                  state = ps[4u + sindex - ps[3]];
	}

	/* Check the last state for a pattern match */
	for (mlist = matchlist[state];
		mlist != NULL;
		mlist = mlist->next) {
		index = T - mlist->n - Tx;

		if (mlist->nocase) {
			nfound++;
			if (match(mlist->id, index, data)>0) {
				*current_state = state;
				*ptx = T;
				return nfound;
			}
		}
		else {
			if (memcmp(mlist->casepatrn, Tx + index, mlist->n) == 0) {
				nfound++;
				if (match(mlist->id, index, data)>0) {
					*current_state = state;
					*ptx = T;
					return nfound;
				}
			}
		}
	}

	*ptx = T;
	return nfound;
}