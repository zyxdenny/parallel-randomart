#include <stdio.h>
#include <stdlib.h>

#define MAX_FUNC_NUM 10
#define MAX_ARG_NUM  10

typedef double (*Func)(double[MAX_ARG_NUM]);

typedef struct Rule {
    int func_num;
    int arg_num[MAX_FUNC_NUM];
    int is_terminal[MAX_FUNC_NUM];
    Func funcs[MAX_FUNC_NUM];                          /* An array of functions */
    int rule_args[MAX_FUNC_NUM][MAX_ARG_NUM];          /* Rows: each function. Columns: rule arguments of the function */
    double prob[MAX_FUNC_NUM];                         /* The probability for each function */
} Rule;

typedef struct ExpressionNode {
    Func func;
    int arg_num;
    struct ExpressionNode *args[MAX_ARG_NUM];
} ExpressionNode;

int rand_with_weight(Rule rule);
ExpressionNode *generateExpressionTree(Rule *grammar, int pos, int depth);
double add(double nums[MAX_ARG_NUM]);
double mult(double nums[MAX_ARG_NUM]);


ExpressionNode *generateExpressionTree(Rule *grammar, int pos, int depth)
{
    Rule rule = grammar[pos];
    Func func_chosen;
    int func_chosen_idx;
    ExpressionNode *res = (ExpressionNode*)malloc(sizeof(ExpressionNode));

    func_chosen_idx = depth <= 0 ?
        0 : rand_with_weight(rule);
    func_chosen = rule.funcs[func_chosen_idx];
    
    res->func = func_chosen;
    res->arg_num = rule.arg_num[func_chosen_idx];

    if (rule.is_terminal[func_chosen_idx]) {
        res->args[0] = NULL;
        return res;
    }
    
    for (int i = 0; i < rule.arg_num[func_chosen_idx]; i++) {
        int arg_rule_pos = rule.rule_args[func_chosen_idx][i];
        res->args[i] = generateExpressionTree(grammar, arg_rule_pos, depth - 1);
    }

    return res;
}

int rand_with_weight(Rule rule)
{
    double rand_value = (double)rand() / RAND_MAX;
    double acc_prob = 0;
    int i = 0;

    for (; i < rule.func_num; i++) {
        acc_prob += rule.prob[i];
        if (rand_value < acc_prob)
            break;
    }

    return i;
}


double add(double nums[MAX_ARG_NUM])
{
    return (nums[0] + nums[1]) / 2;
}

double mult(double nums[MAX_ARG_NUM])
{
    return nums[0] * nums[1];
}


int main(void)
{
    printf("Hello World!\n");

    return 0;
}
