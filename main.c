#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_FUNC_NUM 10
#define MAX_ARG_NUM  10
#define MAX_RULE_NUM 20

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

int randWithWeight(Rule rule);
ExpressionNode *buildExpressionTree(Rule grammar[MAX_RULE_NUM], int pos, int depth);
double evaluateExpressionTree(ExpressionNode *root, int x, int y);
void freeExpressionTree(ExpressionNode *root);

double add(double nums[MAX_ARG_NUM]);
double mult(double nums[MAX_ARG_NUM]);
double identity(double nums[MAX_ARG_NUM]);
double randNorm(double nums[MAX_ARG_NUM]);
double getX(double nums[MAX_ARG_NUM]);
double getY(double nums[MAX_ARG_NUM]);


ExpressionNode *buildExpressionTree(Rule grammar[MAX_RULE_NUM], int pos, int depth)
{
    Rule rule = grammar[pos];
    Func func_chosen;
    int func_chosen_idx;
    ExpressionNode *res = (ExpressionNode*)malloc(sizeof(ExpressionNode));

    func_chosen_idx = depth <= 0 ?
        0 : randWithWeight(rule);
    func_chosen = rule.funcs[func_chosen_idx];
    
    res->func = func_chosen;
    res->arg_num = rule.arg_num[func_chosen_idx];

    if (rule.is_terminal[func_chosen_idx]) {
        res->args[0] = NULL;
        return res;
    }
    
    for (int i = 0; i < rule.arg_num[func_chosen_idx]; i++) {
        int arg_rule_pos = rule.rule_args[func_chosen_idx][i];
        res->args[i] = buildExpressionTree(grammar, arg_rule_pos, depth - 1);
    }

    return res;
}

double evaluateExpressionTree(ExpressionNode *root, int x, int y)
{
    double params[MAX_ARG_NUM];

    if (!root->args[0]) {
        params[0] = x;
        params[1] = y;
        return root->func(params);
    }

    for (int i = 0; i < root->arg_num; i++) {
        params[i] = evaluateExpressionTree(root->args[i], x, y);
    }

    return root->func(params);
}

void freeExpressionTree(ExpressionNode *root)
{
    if (!root->args[0]) {
        free(root);
        return;
    }

    for (int i = 0; i < root->arg_num; i++) {
        freeExpressionTree(root->args[i]);
    }

    return free(root);
}

int randWithWeight(Rule rule)
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


/* functions in expressions */
double add(double nums[MAX_ARG_NUM])
{
    return (nums[0] + nums[1]) / 2;
}

double mult(double nums[MAX_ARG_NUM])
{
    return nums[0] * nums[1];
}

double identity(double nums[MAX_ARG_NUM])
{
    return nums[0];
}

double randNorm(double nums[MAX_ARG_NUM])
{
    /* returns a random number between -1 and 1 */
    return (double)rand() / RAND_MAX * 2 - 1;
}

double getX(double nums[MAX_ARG_NUM])
{
    return nums[0];
}

double getY(double nums[MAX_ARG_NUM])
{
    return nums[1];
}


int main(void)
{
    double r, g, b;
    Rule grammar[MAX_RULE_NUM] = {
        [0] = {
            .func_num = 3,
            .arg_num = {1, 2, 2},
            .is_terminal = {0, 0, 0},
            .funcs = {identity, add, mult},
            .rule_args = {{1}, {0, 0}, {0, 0}},
            .prob = {1.0/4.0, 3.0/8.0, 3.0/8.0},
        },
        [1] = {
            .func_num = 3,
            .arg_num = {0, 0, 0},
            .is_terminal = {1, 1, 1},
            .funcs = {randNorm, getX, getY},
            .rule_args = {{0}},
            .prob = {59.0/100.0, 40.0/100.0, 1.0/100.0},
        },
    };

    srand(time(NULL));
    ExpressionNode *r_root = buildExpressionTree(grammar, 0, 15);
    ExpressionNode *g_root = buildExpressionTree(grammar, 0, 15);
    ExpressionNode *b_root = buildExpressionTree(grammar, 0, 15);

    r = evaluateExpressionTree(r_root, 1, 2);
    g = evaluateExpressionTree(g_root, 1, 2);
    b = evaluateExpressionTree(b_root, 1, 2);

    freeExpressionTree(r_root);
    freeExpressionTree(g_root);
    freeExpressionTree(b_root);

    printf("RGB is: %f, %f, %f\n", r, g, b);

    return 0;
}
