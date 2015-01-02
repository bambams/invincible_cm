
#ifndef H_C_CTOKEN
#define H_C_CTOKEN

int read_next(struct ctokenstruct* ctoken);

//int get_next_ctoken(struct scodestruct* scode, int* scode_pos, struct ctokenstruct* ctoken, struct identifierstruct* identifier);

int c_get_next_char_from_scode(void);
int check_end_of_scode(void);

int check_next(int ctoken_type, int ctoken_subtype);
int peek_next(struct ctokenstruct* ctoken);
int peek_check_next(int ctoken_type, int ctoken_subtype);

int ctoken_compare_text(struct ctokenstruct* ctoken, const char *comp_str);

int read_next_expression_value(struct ctokenstruct* ctoken);
int peek_next_expression_value(struct ctokenstruct* ctoken);
int attempt_array_resolve(int id_index);
int read_literal_numbers(struct ctokenstruct* ctoken);

int new_c_identifier(struct ctokenstruct* ctoken, int type);
void print_identifier_list(void);

#endif
