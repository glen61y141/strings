/*
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

https://github.com/morenice/ahocorasick
*/

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ctype.h>
#include <limits.h>

struct aho_text_t
{
	struct aho_text_t *prev;
	struct aho_text_t *next;
	unsigned char *text;
	int id;
	int len;
};

struct aho_trie_node
{
	struct aho_trie_node *parent;
	struct aho_trie_node *child_list[256];

	unsigned int child_count;

	unsigned char text;
	char resv[3];
	int text_end;

	struct aho_text_t **output_text; /* when text_end is 1 */
	int output_count;

	struct aho_trie_node *failure_link;
	struct aho_trie_node *output_link;
};

struct aho_trie
{
	struct aho_trie_node root;
};

struct aho_queue_node
{
	struct aho_queue_node *next;
	struct aho_queue_node *prev;
	struct aho_trie_node  *data;
};

struct aho_queue
{
	struct aho_queue_node *front;
	struct aho_queue_node *rear;
	unsigned int count;
};

struct aho_match_t
{
	int type;
	int pos;
	int id;
	int len;
};

struct ahocorasick
{
	struct aho_text_t *text_list_head;
	struct aho_text_t *text_list_tail;

	struct aho_trie trie;

	void (*callback_match)(void *arg, struct aho_match_t*);
	void  *callback_arg;
};

extern void aho_create_trie(struct ahocorasick *aho);
extern unsigned int aho_add_match_text(struct ahocorasick *aho, unsigned int text_id, unsigned char *text, unsigned int len);

extern void aho_findtext(struct ahocorasick *aho, int nocase, const char *data, unsigned int data_len, void (*callback_match)(void *arg, struct aho_match_t*), void  *callback_arg);

extern void aho_clear_match_text(struct ahocorasick *aho);
extern void aho_clear_trie(struct ahocorasick *aho);

static int aho_queue_enqueue(struct aho_queue *que, struct aho_trie_node *node)
{
	struct aho_queue_node *que_node;
	que_node = (struct aho_queue_node*) malloc(sizeof(struct aho_queue_node));

	if (!que_node)
	{
		/* free memory error!! */
		return 0;
	}

	memset(que_node, 0x00, sizeof(struct aho_queue_node));
	que_node->data = node;

	if (que->count == 0)
	{
		que->rear = que_node;
		que->front = que_node;
		que->count++;
		return 1;
	}

	que_node->prev = que->rear;
	que->rear->next = que_node;
	que->rear = que_node;
	que->count++;

	return 1;
}

static int aho_queue_empty(struct aho_queue *que)
{
	return (que->count == 0) ? 1 : 0;
}

static struct aho_queue_node *aho_queue_dequeue(struct aho_queue *que)
{
	struct aho_queue_node *deque_node;
	struct aho_queue_node *after_last_node;

	if (aho_queue_empty(que) == 1)
	{
		return NULL;
	}

	if (que->count == 1)
	{
		deque_node = que->rear;
		que->front = que->rear = NULL;
		que->count--;
		return deque_node;
	}

	deque_node = que->rear;

	after_last_node = que->rear->prev;
	after_last_node->next = NULL;
	que->rear = after_last_node;
	que->count--;
	return deque_node;
}

static void aho_queue_destroy(struct aho_queue *que)
{
	struct aho_queue_node *que_node = NULL;
	while ((que_node = aho_queue_dequeue(que)) != NULL)
	{
		free(que_node);
	}
}

static void aho_queue_init(struct aho_queue *que)
{
	memset(que, 0x00, sizeof(struct aho_queue));
}

static void __aho_trie_node_init(struct aho_trie_node *node)
{
	memset(node, 0x00, sizeof(struct aho_trie_node));
	node->text_end = 0;
}

static int aho_add_trie_node(struct aho_trie *t, struct aho_text_t *text)
{
	struct aho_trie_node *travasal_node = &(t->root);
	int text_idx;

	for (text_idx = 0; text_idx < text->len; text_idx++)
	{
		unsigned char node_text = text->text[text_idx];
		int find_node = 0;
		int child_idx = 0;

		if (travasal_node->child_count == 0)
		{
			/* insert first node to child_list */
			struct aho_trie_node *child_list = (struct aho_trie_node*) malloc(sizeof(struct aho_trie_node));
			if (child_list == NULL)
			{
				return -1;
			}

			travasal_node->child_list[0] = child_list;
			travasal_node->child_count++;

			__aho_trie_node_init(travasal_node->child_list[0]);
			travasal_node->child_list[0]->text = node_text;
			travasal_node->child_list[0]->parent = travasal_node;

			travasal_node = travasal_node->child_list[0];
			continue;
		}

		if (travasal_node->child_count == 256)
		{
			return 0;
		}

		for (child_idx = 0; child_idx < travasal_node->child_count; child_idx++)
		{
			if (travasal_node->child_list[child_idx]->text == node_text)
			{
				find_node = 1;
				break;
			}
		}

		if (find_node == 1)
		{
			travasal_node = travasal_node->child_list[child_idx];
		}
		else
		{
			/* push_back to child_list */
			struct aho_trie_node *child_node = NULL;

			child_node = (struct aho_trie_node*) malloc(sizeof(struct aho_trie_node));
			if (child_node == NULL)
			{
				return -1;
			}

			travasal_node->child_list[travasal_node->child_count] = child_node;
			travasal_node->child_count++;

			__aho_trie_node_init(child_node);
			child_node->text = node_text;
			child_node->parent = travasal_node;

			travasal_node = child_node;
		}
	}

	// connect output link
	if (travasal_node)
	{
		struct aho_text_t **output_text;
		int output_count = travasal_node->output_count;

		output_text = realloc(travasal_node->output_text, sizeof(struct aho_text_t *) * (output_count + 1));;
		if (output_text == NULL)
		{
			return 0;
		}

		output_text[output_count] = text;

		travasal_node->output_text = output_text;
		travasal_node->output_count = output_count + 1;
		travasal_node->text_end = 1;
	}

	return 1;
}

static int __aho_connect_link(struct aho_trie_node *p, struct aho_trie_node *q)
{
	struct aho_trie_node *pf = NULL;
	int i = 0;

	/* is root node */
	if (p->failure_link == NULL || p->parent == NULL)
	{
		q->failure_link = p;
		return 1;
	}

	pf = p->failure_link;

	for (i = 0; i < pf->child_count; i++)
	{
		/* check child node of failure link(p) */
		if (pf->child_list[i]->text == q->text)
		{
			/* connect failure link */
			q->failure_link = pf->child_list[i];

			/* connect output link */
			if (pf->child_list[i]->text_end)
			{
				q->output_link = pf->child_list[i];
			}
			else
			{
				q->output_link = pf->child_list[i]->output_link;
			}

			return 1;
		}
	}
	return 0;
}

static void aho_connect_link(struct aho_trie *t)
{
	struct aho_queue queue;
	aho_queue_init(&queue);
	aho_queue_enqueue(&queue, &(t->root));

	/* BFS access
	 *  connect failure link and output link
	 */
	while (1)
	{
		/* p :parent, q : child node */
		struct aho_queue_node *queue_node = NULL;
		struct aho_trie_node *p = NULL;
		struct aho_trie_node *q = NULL;
		int i = 0;

		queue_node = aho_queue_dequeue(&queue);
		if (queue_node == NULL)
		{
			break;
		}

		p = queue_node->data;
		free(queue_node);

		/* get child node list of p */
		for (i = 0; i < p->child_count; i++)
		{
			struct aho_trie_node *pf = p;

			aho_queue_enqueue(&queue, p->child_list[i]);
			q = p->child_list[i];

			while (__aho_connect_link(pf, q) == 0)
			{
				pf = pf->failure_link;
			}
		}
	}

	aho_queue_destroy(&queue);
}

static void aho_clean_trie_node(struct aho_trie *t)
{
	struct aho_queue queue;
	aho_queue_init(&queue);
	aho_queue_enqueue(&queue, &(t->root));

	/* BFS */
	while (1)
	{
		struct aho_queue_node *queue_node = NULL;
		struct aho_trie_node *remove_node = NULL;
		int i = 0;

		queue_node = aho_queue_dequeue(&queue);
		if (queue_node == NULL)
		{
			break;
		}

		remove_node = queue_node->data;
		free(queue_node);

		for (i = 0; i < remove_node->child_count; i++)
		{
			aho_queue_enqueue(&queue, remove_node->child_list[i]);
		}

		/* is root node */
		if (remove_node->parent == NULL)
		{
			continue;
		}

		if (remove_node->output_text)
		{
			free(remove_node->output_text);
			remove_node->output_text = NULL;
		}

		free(remove_node);
	}
}

static int __aho_find_trie_node(struct aho_trie_node **start, const unsigned char text)
{
	struct aho_trie_node *search_node = NULL;
	int i = 0;

	search_node = *start;
	if (search_node == NULL)
	{
		return 0;
	}

	for (i = 0; i < search_node->child_count; i++)
	{
		if (search_node->child_list[i]->text == text)
		{
			/* find it! move to find child node! */
			*start = search_node->child_list[i];
			return 1;
		}
	}

	/* not found */
	return 0;
}

static void aho_find_trie_node(struct aho_trie_node **start, const unsigned char text)
{
	while (__aho_find_trie_node(start, text) == 0)
	{
		/* not found!
		 * when root node stop
		 */

		if (*start == NULL || (*start)->parent == NULL)
		{
			break;
		}

		/* retry find. move failure link. */
		*start = (*start)->failure_link;
	}
}

static void aho_init_trie(struct aho_trie *t)
{
	memset(t, 0x00, sizeof(struct aho_trie));
	__aho_trie_node_init(&(t->root));
}

static void aho_destroy_trie(struct aho_trie *t)
{
	aho_clean_trie_node(t);
}

static void aho_match_handler(int type, int pos, struct aho_trie_node *result, void (*callback_match)(void *arg, struct aho_match_t*), void  *callback_arg)
{
	struct aho_match_t match;
	int i;

	for (i = 0; i < result->output_count; i++)
	{
		match.type = type;
		match.pos = pos;

		match.id  = result->output_text[i]->id;
		match.len = result->output_text[i]->len;

		if (callback_match)
		{
			callback_match(callback_arg, &match);
		}
	}
}

unsigned int aho_add_match_text(struct ahocorasick *aho, unsigned int text_id, unsigned char *text, unsigned int len)
{
	struct aho_text_t *a_text = NULL;

	a_text = (struct aho_text_t*) malloc(sizeof(struct aho_text_t));
	if (!a_text)
	{
		goto lack_free_mem;
	}

	memset(a_text, 0, sizeof(struct aho_text_t));

	a_text->text = (unsigned char*) malloc(sizeof(unsigned char) * len);
	if (!a_text->text)
	{
		goto lack_free_mem;
	}

	a_text->id = text_id;
	memcpy(a_text->text, text, len);

	a_text->len = len;
	a_text->prev = NULL;
	a_text->next = NULL;

	if (aho->text_list_head == NULL)
	{
		aho->text_list_head = a_text;
		aho->text_list_tail = a_text;
	}
	else
	{
		aho->text_list_tail->next = a_text;
		a_text->prev = aho->text_list_tail;
		aho->text_list_tail = a_text;
	}

	return 0;

lack_free_mem:
	return -1;
}

void aho_findtext(struct ahocorasick *aho, int nocase, const char *data, unsigned int data_len, void (*callback_match)(void *arg, struct aho_match_t*), void  *callback_arg)
{
	int i = 0;
	struct aho_trie_node *travasal_node = NULL;

	travasal_node = &(aho->trie.root);

	for (i = 0; i < data_len; i++)
	{
		struct aho_trie_node *node;
		char c;

		if (nocase)
		{
			c = toupper(data[i]);
		}
		else
		{
			c = data[i];
		}

		aho_find_trie_node(&travasal_node, c);

		if (travasal_node == NULL)
		{
			break;
		}

		/* match case1: find text end! */
		if (travasal_node->text_end)
		{
			aho_match_handler(1, i + 1, travasal_node, callback_match, callback_arg);
		}

		node = travasal_node->output_link;

		/* match case2: exist output_link */
		if (node && node->text_end)
		{
			aho_match_handler(2, i + 1, node, callback_match, callback_arg);
		}
	}
}

void aho_create_trie(struct ahocorasick *aho)
{
	struct aho_text_t *iter = NULL;
	aho_init_trie(&(aho->trie));

	for (iter = aho->text_list_head; iter != NULL; iter = iter->next)
	{
		aho_add_trie_node(&(aho->trie), iter);
	}

	aho_connect_link(&(aho->trie));
}

void aho_clear_match_text(struct ahocorasick *aho)
{
	struct aho_text_t *curr = aho->text_list_head;
	struct aho_text_t *next;

	while (curr != NULL)
	{
		next = curr->next;

		free(curr->text);
		free(curr);

		curr = next;
	}

	aho->text_list_head = NULL;
	aho->text_list_tail = NULL;
}

void aho_clear_trie(struct ahocorasick *aho)
{
	aho_destroy_trie(&aho->trie);
}

void callback_match_total(void *arg, struct aho_match_t *m)
{
	int *match_total = (int*)arg;

	printf("match type %d, id: %d, at pos %d\n", m->type, m->id, m->pos);

	(*match_total)++;
}

int main(int argc, const char *argv[])
{
	struct ahocorasick aho;
	int match_total = 0;

	memset(&aho, 0x00, sizeof(struct ahocorasick));

	aho_add_match_text(&aho, 1, (void *) "ab",    2);
	aho_add_match_text(&aho, 2, (void *) "ab",    2);
	aho_add_match_text(&aho, 3, (void *) "bc",    2);
	aho_add_match_text(&aho, 4, (void *) "bc",    2);
	aho_add_match_text(&aho, 5, (void *) "cd",    2);
	aho_add_match_text(&aho, 6, (void *) "ef",    2);
	aho_add_match_text(&aho, 7, (void *) "gh",    2);
	aho_add_match_text(&aho, 8, (void *) "abc",   3);
	aho_add_match_text(&aho, 9, (void *) "abcd",  4);

	aho_create_trie(&aho);

	aho_findtext(&aho, 0, "abcdefgh", strlen("abcdefgh"), callback_match_total, &match_total);
	printf("totoal match: %u\n", match_total);

	aho_clear_match_text(&aho);
	aho_clear_trie(&aho);

	return 0;
}