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

https://github.com/occasumlux/WuManber
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define NUM_BLOCK  2      //Block size
#define MIN_LEN    2      //Patterns minimum length (>= Block size)
#define MAX_LEN    512    //Patterns maximum length

struct wu_map_node
{
	struct wu_map_node *prev;
	struct wu_map_node *next;
	unsigned char *key;
	int key_len;
	int value;
	int *list;
	int list_len;
};

struct wu_map
{
	struct wu_map_node **row_head;
	int num_entries;
};

struct wu_text
{
	int id;
	int str_len;
	unsigned char *str;
};

struct wu_match
{
	int id;
};

struct wu_info
{
	struct wu_map shift_table;
	struct wu_map aux_shift_table;
	struct wu_map hash_table;

	struct wu_text *patterns;
	int pattern_count;

	int min_len;
	int nocase;
};


extern int wu_init(struct wu_info *wu);
extern int wu_add_text(struct wu_info *wu, int id, int str_len, unsigned char *str);
extern int wu_complier(struct wu_info *wu);
extern int wu_search(struct wu_info *wu, int length, unsigned char *text, void (*callback_match)(void *arg, struct wu_match*), void  *callback_arg);
extern void wu_dump(struct wu_info *wu);
extern void wu_free(struct wu_info *wu);

static int map_get_row_id(struct wu_map *p_table, unsigned char *key, int key_len)
{
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int c = 0;
	int i;

	for (i = 0; i < key_len; i++)
	{
		c = c * a + (key[i]);
		a *= b;
	}

	return ((c) * 0x61C88647) >> (32 - 7);	/* row: 128 */
}

static void map_dump(struct wu_map *p_table)
{
	struct wu_map_node *p_node;
	unsigned int row_id;
	int i;

	for (row_id = 0; row_id < 128; row_id++)
	{
		if (p_table->row_head[row_id] == NULL)
		{
			continue;
		}

		p_node = p_table->row_head[row_id];
		while (p_node)
		{
			printf("kv: ");

			for (i = 0; i < p_node->key_len; i++)
			{
				printf("%c", p_node->key[i]);
			}

			printf(" -> %d\n", p_node->value);

			p_node = p_node->next;
		}
	}
}

static void map_cleanup(struct wu_map *p_table)
{
	struct wu_map_node *p_node;
	unsigned int row_id;

	if (p_table->row_head)
	{
		for (row_id = 0; row_id < 128; row_id++)
		{
			if (p_table->row_head[row_id] == NULL)
			{
				continue;
			}

			p_node = p_table->row_head[row_id];
			while (p_node)
			{
				if (p_node->next)
				{
					p_node->next->prev = p_node->prev;
				}

				if (p_node->prev)
				{
					p_node->prev->next = p_node->next;
				}
				else
				{
					p_table->row_head[row_id] = p_node->next;
				}

				if (p_node->key)
				{
					free(p_node->key);
				}

				if (p_node->list)
				{
					free(p_node->list);
				}

				free(p_node);

				p_table->num_entries --;

				p_node = p_table->row_head[row_id];
			}
		}

		free(p_table->row_head);
		p_table->row_head = NULL;
	}
}

static int map_init(struct wu_map *p_table)
{
	memset(p_table, 0, sizeof(struct wu_map));
	p_table->num_entries = 0;

	p_table->row_head = (struct wu_map_node **) malloc(128 * sizeof(struct wu_map_node *));
	if (!p_table->row_head)
	{
		return -1;
	}

	memset(p_table->row_head, 0, 128 * sizeof(struct wu_map_node *));

	return 0;
}

static struct wu_map_node *map_find(struct wu_map *p_table, unsigned char *key, int key_len)
{
	struct wu_map_node *p_node;
	int row_id;

	row_id = map_get_row_id(p_table, key, key_len);

	p_node = p_table->row_head[row_id];
	while (p_node != NULL)
	{
		if (p_node->key_len == key_len &&
			memcmp(p_node->key, key, key_len) == 0)
		{
			break;
		}
		p_node = p_node->next;
	}

	if (p_node)
	{
		return p_node;
	}
	else
	{
		return NULL;
	}
}

static int map_set(struct wu_map *p_table, void *key, int key_len, int value)
{
	struct wu_map_node *p_node;
	int row_id;

	row_id = map_get_row_id(p_table, key, key_len);

	p_node = p_table->row_head[row_id];
	while (p_node != NULL)
	{
		if (p_node->key_len == key_len &&
			memcmp(p_node->key, key, key_len) == 0)
		{
			break;
		}
		p_node = p_node->next;
	}

	if (p_node != NULL)
	{
		p_node->value = value;
	}
	else
	{
		p_node = (struct wu_map_node *) malloc(sizeof(struct wu_map_node));
		if (p_node == NULL)
		{
			return -1;
		}

		memset(p_node, 0, sizeof(struct wu_map_node));

		p_node->key = malloc(key_len);
		if (p_node->key == NULL)
		{
			free(p_node);
			return -1;
		}

		memcpy(p_node->key, key, key_len);
		p_node->key_len = key_len;

		p_node->value = value;

		p_node->prev = NULL;
		p_node->next = p_table->row_head[row_id];

		if (p_table->row_head[row_id])
		{
			p_table->row_head[row_id]->prev = p_node;
		}

		p_table->row_head[row_id] = p_node;
		p_table->num_entries ++;
	}

	return 0;
}

static int map_add(struct wu_map *p_table, unsigned char *key, int key_len, int value)
{
	struct wu_map_node *p_node;
	int row_id;
	int *p_list;

	row_id = map_get_row_id(p_table, key, key_len);

	p_node = p_table->row_head[row_id];
	while (p_node != NULL)
	{
		if (p_node->key_len == key_len &&
			memcmp(p_node->key, key, key_len) == 0)
		{
			break;
		}
		p_node = p_node->next;
	}

	if (p_node != NULL)
	{
		p_list = realloc(p_node->list, sizeof(int) * (p_node->list_len + 1));
		if (p_list == NULL)
		{
			return -1;
		}

		p_list[p_node->list_len] = value;
		p_node->list = p_list;
		p_node->list_len += 1;
	}
	else
	{

		p_node = (struct wu_map_node *) malloc(sizeof(struct wu_map_node));
		if (p_node == NULL)
		{
			return -1;
		}

		memset(p_node, 0, sizeof(struct wu_map_node));

		p_node->key = malloc(key_len);
		if (p_node->key == NULL)
		{
			free(p_node);
			return -1;
		}

		p_node->list = malloc(sizeof(int));
		if (p_node->list == NULL)
		{
			free(p_node->key);
			free(p_node);
			return -1;
		}

		memcpy(p_node->key, key, key_len);
		p_node->key_len = key_len;

		p_node->list[0] = value;
		p_node->list_len = 1;

		p_node->prev = NULL;
		p_node->next = p_table->row_head[row_id];

		if (p_table->row_head[row_id])
		{
			p_table->row_head[row_id]->prev = p_node;
		}

		p_table->row_head[row_id] = p_node;
		p_table->num_entries ++;
	}

	return 0;
}

static int wu_substr(int nocase, unsigned char *dest, unsigned char *src, int src_len, int x, int y)
{
	int i;
	int j;

	if (x < 0 || y < 0)
	{
		return -1;
	}

	for (i = x, j = 0; j < y && i < src_len; i++, j++)
	{
		if (nocase)
		{
			*(dest + j) = toupper(*(src + i));
		}
		else
		{
			*(dest + j) = *(src + i);
		}
	}

	return j;
}

int wu_search(struct wu_info *wu, int length, unsigned char *text, void (*callback_match)(void *arg, struct wu_match*), void  *callback_arg)
{
	struct wu_map *shift_table = &wu->shift_table;
	struct wu_map *aux_shift_table = &wu->aux_shift_table;
	struct wu_map *hash_table = &wu->hash_table;

	struct wu_map_node *p_node;
	struct wu_text *pattern;

	unsigned char block[MAX_LEN];
	int block_len;

	int char_index;
	int char_pos;
	int shift_value;

	int pattern_iter;
	int *p_list;

	struct wu_match match;
	char c;
	int i;

	if (wu->pattern_count == 0 || wu->min_len > length)
	{
		return -1;
	}

	for (char_index = wu->min_len - NUM_BLOCK; char_index < length; ++char_index)
	{
		block_len = wu_substr(wu->nocase, block, text, length, char_index, NUM_BLOCK);
		if (block_len < 0)
		{
			return -1;
		}

		p_node = map_find(shift_table, block, block_len);

		if (p_node)
		{
			shift_value = p_node->value;

			if (shift_value == 0)
			{
				p_node = map_find(hash_table, block, block_len);

				if (p_node)
				{
					p_list = p_node->list;

					for (pattern_iter = 0; pattern_iter < p_node->list_len; pattern_iter ++)
					{
						char_pos = char_index - wu->min_len + NUM_BLOCK;
						pattern = &wu->patterns[p_list[pattern_iter]];

						if ((char_pos + pattern->str_len) <= length)
						{
							for (i = 0; i < pattern->str_len; ++i)
							{
								if (wu->nocase)
								{
									c = toupper(text[char_pos + i]);
								}
								else
								{
									c = text[char_pos + i];
								}

								if (pattern->str[i] != c)
								{
									break;
								}
							}

							if (i == pattern->str_len)
							{
								if (callback_match)
								{
									match.id = pattern->id;
									callback_match(callback_arg, &match);
								}
							}
						}
					}
				}

				p_node = map_find(aux_shift_table, block, block_len);

				if (p_node)
				{
					char_index += p_node->value - 1;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				char_index += shift_value - 1;
			}
		}
		else
		{
			char_index += wu->min_len - NUM_BLOCK;
		}
	}

	return 0;
}

int wu_complier(struct wu_info *wu)
{
	struct wu_map *shift_table = &wu->shift_table;
	struct wu_map *aux_shift_table = &wu->aux_shift_table;
	struct wu_map *hash_table = &wu->hash_table;

	struct wu_map_node *p_node;

	unsigned char block[MAX_LEN];
	int block_len;

	int char_index;
	int block_pos;
	int last_shift;

	int ret;
	int i;

	for (i = 0; i < wu->pattern_count; i++)
	{
		unsigned char *pattern_iter = wu->patterns[i].str;
		unsigned char *pattern_end  = wu->patterns[i].str + wu->patterns[i].str_len;

		for (char_index = 0; char_index < wu->min_len - NUM_BLOCK + 1; ++char_index)
		{
			/* pattern has been normalized */
			block_len = wu_substr(0, block, pattern_iter, pattern_end - pattern_iter, char_index, NUM_BLOCK);
			if (block_len < 0)
			{
				return -1;
			}

			block_pos = wu->min_len - char_index - NUM_BLOCK;
			last_shift = (block_pos == 0) ? wu->min_len - NUM_BLOCK + 1 : block_pos;

			p_node = map_find(shift_table, block, block_len);

			if (p_node)
			{
				if (block_pos < p_node->value)
				{
					p_node->value = block_pos;
				}
			}
			else
			{
				ret = map_set(shift_table, block, block_len, block_pos);
				if (ret < 0)
				{
					printf("set shift_table error\n");
					return -1;
				}
			}

			if (block_pos == 0)
			{
				ret = map_add(hash_table, block, block_len, i);;
				if (ret < 0)
				{
					printf("add hash_table error\n");
					return -1;
				}

				if (last_shift != 0)
				{
					p_node = map_find(aux_shift_table, block, block_len);

					if (p_node)
					{
						if (last_shift < p_node->value)
						{
							p_node->value = last_shift;
						}
					}
					else
					{
						ret = map_set(aux_shift_table, block, block_len, last_shift);

						if (ret < 0)
						{
							printf("set aux_shift_table error\n");
							return -1;
						}
					}
				}
			}
		}
	}

	return 0;
}

int wu_add_text(struct wu_info *wu, int id, int str_len, unsigned char *str)
{
	struct wu_text *patterns;
	int pattern_index = wu->pattern_count;

	unsigned char *tmp;
	int i;

	if (str_len < MIN_LEN || str_len > MAX_LEN)
	{
		return -1;
	}

	tmp = malloc(str_len);
	if (tmp == NULL)
	{
		printf("allocate pattern error\n");
		return -1;
	}

	patterns = realloc(wu->patterns, sizeof(struct wu_text) * (pattern_index + 1));
	if (patterns == NULL)
	{
		printf("allocate pattern list error\n");
		free(tmp);
		return -1;
	}

	memcpy(tmp, str, str_len);

	if (wu->nocase)
	{
		for (i = 0; i < str_len; i ++)
		{
			tmp[i] = toupper(tmp[i]);
		}
	}

	if (wu->min_len > str_len)
	{
		wu->min_len = str_len;
	}

	patterns[pattern_index].str = tmp;
	patterns[pattern_index].id = id;
	patterns[pattern_index].str_len = str_len;

	wu->patterns = patterns;
	wu->pattern_count ++;

	return 0;
}

void wu_dump(struct wu_info *wu)
{
	int i;

	if (wu->patterns)
	{
		for (i = 0; i < wu->pattern_count; i++)
		{
			printf("id %d strlen %d\n", wu->patterns[i].id, wu->patterns[i].str_len);
		}
	}

	printf("shift_table:\n");
	map_dump(&wu->shift_table);

	printf("aux_shift_table:\n");
	map_dump(&wu->aux_shift_table);
}

void wu_free(struct wu_info *wu)
{
	int i;

	map_cleanup(&wu->shift_table);
	map_cleanup(&wu->aux_shift_table);
	map_cleanup(&wu->hash_table);

	if (wu->patterns)
	{
		for (i = 0; i < wu->pattern_count; i++)
		{
			if (wu->patterns[i].str)
			{
				free(wu->patterns[i].str);
			}
		}

		free(wu->patterns);
	}
}

int wu_init(struct wu_info *wu)
{
	int ret;

	memset(wu, 0, sizeof(struct wu_info));

	ret = map_init(&wu->shift_table);
	if (ret != 0)
	{
		return -1;
	}

	ret = map_init(&wu->aux_shift_table);
	if (ret != 0)
	{
		return -1;
	}

	ret = map_init(&wu->hash_table);
	if (ret != 0)
	{
		return -1;
	}

	wu->min_len = MAX_LEN + 1;

	return 0;
}

void wu_callback(void *arg, struct wu_match *match)
{
	int *match_total = (int*)arg;

	printf("match id: %d\n", match->id);

	(*match_total)++;
}

int main(void)
{
	int match_total = 0;
	struct wu_info wu = {0};

	wu_init(&wu);

	/* text will do upper case in nocase mode */
	wu.nocase = 1;

	wu_add_text(&wu, 1, 2, (void *) "AB");
	wu_add_text(&wu, 2, 2, (void *) "ab");
	wu_add_text(&wu, 3, 2, (void *) "CD");
	wu_add_text(&wu, 4, 4, (void *) "abcd");

	wu_complier(&wu);

	/* dump info */
	wu_dump(&wu);

	wu_search(&wu, 4, (void *) "AbCd", wu_callback, &match_total);

	wu_free(&wu);

	return 0;
}