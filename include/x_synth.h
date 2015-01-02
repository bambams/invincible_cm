
#ifndef H_X_SYNTH
#define H_X_SYNTH

#define TWOPI                 (2.0 * PI)
//#define SAMPLES_PER_BUFFER    (1024)
//#define SAMPLE_FREQUENCY      (44100)
#define SAMPLE_FREQUENCY      (22050)


#define FILTERS 8

enum
{
SYNTH_BASE_SINE
};

enum
{
FILTER_SINE_ADD,
FILTER_FLANGER
//FILTER_ATTACK_LINEAR,
//FILTER_DECAY_LINEAR
};

struct filterstruct
{
	int type;
	int start_time;
	int end_time;
	float base_freq;
	float phase;
};

struct synthstruct
{
 int base_type;
 int length;
 float linear_attack; // can be zero if a more complex filter attack/decay is being used
 float linear_decay;
 float base_freq;
 float phase;
 int filters;
 struct filterstruct filter [FILTERS];

};

void set_synth(int base_type, int length, int linear_attack, int linear_decay, int base_freq, int phase);
int add_synth_filter(int type, float base_freq, float phase);
ALLEGRO_SAMPLE* synthesise(int ssi);
#define SYNTH_SAMPLES 8
#define SYNTH_SAMPLE_MAX_SIZE 20000

extern struct synthstruct synth;
extern float synth_sample [SYNTH_SAMPLES] [SYNTH_SAMPLE_MAX_SIZE];


#endif
