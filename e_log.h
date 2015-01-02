
#ifndef H_E_LOG
#define H_E_LOG

void init_log(int w_pixels, int h_pixels);
void display_log(int x1, int y1);
void reset_log(void);

void write_to_log(const char* str);
void start_log_line(int mcol);
void finish_log_line(void);

//void write_to_log(const char* str, int source, int source_line);
void write_line_to_log(char* str, int mcol);
//void start_log_line(int source, int source_line);
//void finish_log_line(void);

void write_number_to_log(int num);

#endif
