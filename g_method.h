
#ifndef H_G_METHOD
#define H_G_METHOD

void active_method_pass_each_tick(void);

void active_method_pass_after_execution(struct procstruct* pr);

s16b call_method(struct procstruct* pr, struct programstruct* cl, int m, struct methodstruct* methods, int* instr_left);

enum
{
METHOD_COST_CAT_NONE,
METHOD_COST_CAT_MIN,
METHOD_COST_CAT_LOW,
METHOD_COST_CAT_MED,
METHOD_COST_CAT_HIGH,
METHOD_COST_CAT_ULTRA,

METHOD_COST_CATEGORIES
};


struct method_costsstruct
{
 int base_cost [METHOD_COST_CATEGORIES];
 int upkeep_cost [METHOD_COST_CATEGORIES];
 int extension_cost [METHOD_COST_CATEGORIES];
 int extension_upkeep_cost [METHOD_COST_CATEGORIES];
};


#endif
