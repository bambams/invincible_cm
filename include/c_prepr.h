
#ifndef H_C_PREPR
#define H_C_PREPR

int preprocess(struct sourcestruct* source, struct scodestruct *scode, const char* defined);

int load_source_file(const char* file_path, struct sourcestruct* target_source, int src_file_index, int preprocessing);
int load_binary_file(const char* file_path, struct bcodestruct* bcode, int src_file_index, int preprocessing);
void convert_bcode_to_asm_source_initialiser(struct bcodestruct* bcode, char text [SOURCE_TEXT_LINES] [SOURCE_TEXT_LINE_LENGTH]);

// max length of a preprocessor token
#define PTOKEN_LENGTH 32

#define NUMTOKENS 700

// numtokens are built-in defined numbers
struct numtokenstruct
{
 char name [PTOKEN_LENGTH];
 int value;
};


#endif
