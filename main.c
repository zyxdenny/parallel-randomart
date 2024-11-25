#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MAX_FUNC_NUM      10
#define MAX_ARG_NUM       10
#define MAX_RULE_NUM      20
#define IMG_WIDTH         800
#define IMG_HEIGHT        800
#define IMG_CHANNEL_NUM   3
#define PI                3.14159
#define MAX_SYMBOL_LEN    10

typedef double (*Func)(double[MAX_ARG_NUM]);

typedef struct FuncInfo {
    Func func;
    int arity;
    char func_name[32];
} FuncInfo;

typedef struct SubRule {
    FuncInfo func_info;
    int args[MAX_ARG_NUM];
    float prob;
} SubRule;

typedef struct Rule {
    int func_num;
    SubRule sub_rules[MAX_FUNC_NUM];
} Rule;

typedef struct ExpressionNode {
    FuncInfo func_info;
    struct ExpressionNode *args[MAX_ARG_NUM];
} ExpressionNode;


ExpressionNode *buildExpressionTree(Rule *grammar, int pos, int depth);
double evaluateExpressionTree(ExpressionNode *root, double x, double y);
void freeExpressionTree(ExpressionNode *root);
int rand_with_weight(Rule rule);
void func_info_cpy(FuncInfo *dest, FuncInfo *source);
char *get_real_line(char *buffer, int buffer_size, FILE *file);
int find_symbol(char *symbol, char symbol_arr[MAX_RULE_NUM][MAX_SYMBOL_LEN + 1], int symbol_arr_size);
int find_func(char *func_name);
int parse_from_file(char *file_name, int entry_symbol_arr[3], Rule grammar[MAX_RULE_NUM]);

double add(double *nums);
double mult(double *nums);
double identity(double *nums);
double rand_norm(double *nums);
double get_x(double *nums);
double get_y(double *nums);
double sin_func(double *nums);
double neg_func(double *nums);
double sqrt_func(double *nums);
double mix(double *nums);
double tan_func(double *nums);


/* Global variables */
FuncInfo func_collection[] = {
    { get_x,       0,   "GET_X" },
    { get_y,       0,   "GET_Y" },
    { rand_norm,   0,   "RAND" },
    { identity,    1,   "ID" },
    { sin_func,    1,   "SIN" },
    { tan_func,    1,   "TAN" },
    { neg_func,    1,   "NEG" },
    { sqrt_func,   1,   "SQRT" },
    { add,         2,   "ADD" },
    { mult,        2,   "MULT" },
    { mix,         3,   "MIX" },
};

ExpressionNode *buildExpressionTree(Rule *grammar, int pos, int depth)
{
    Rule rule = grammar[pos];
    FuncInfo func_chosen;
    int func_chosen_idx;
    ExpressionNode *res = (ExpressionNode*)malloc(sizeof(ExpressionNode));

    func_chosen_idx = depth <= 0 ?
        0 : rand_with_weight(rule);
    func_chosen = rule.sub_rules[func_chosen_idx].func_info;
    
    func_info_cpy(&res->func_info, &func_chosen);

    if (depth >= 0 && (double)rand() / RAND_MAX < 0.5)
        depth -= 1;

    for (int i = 0; i < rule.sub_rules[func_chosen_idx].func_info.arity; i++) {
        int arg = rule.sub_rules[func_chosen_idx].args[i];
        res->args[i] = buildExpressionTree(grammar, arg, depth - 1);
    }

    return res;
}

double evaluateExpressionTree(ExpressionNode *root, double x, double y)
{
    double params[MAX_ARG_NUM];
    double res;

    if (root->func_info.arity == 0) {
        params[0] = x;
        params[1] = y;
        return root->func_info.func(params);
    }

    for (int i = 0; i < root->func_info.arity; i++) {
        params[i] = evaluateExpressionTree(root->args[i], x, y);
    }

    res = root->func_info.func(params);
    if (res < -1 || res > 1) {
        printf("function %s has a problem\n", root->func_info.func_name);
        exit(1);
    }
    return res;
}

void freeExpressionTree(ExpressionNode *root)
{
    for (int i = 0; i < root->func_info.arity; i++) {
        freeExpressionTree(root->args[i]);
    }

    free(root);
}

void printExpressionTree(ExpressionNode *root)
{
    printf("%s", root->func_info.func_name);
    if (root->func_info.func != get_x && root->func_info.func != get_y)
        printf("(");
    for (int i = 0; i < root->func_info.arity; i++) {
        printExpressionTree(root->args[i]);
        if (i != root->func_info.arity - 1)
            printf(", ");
    }
    if (root->func_info.func != get_x && root->func_info.func != get_y)
        printf(")");
}

int rand_with_weight(Rule rule)
{
    double rand_value = (double)rand() / RAND_MAX;
    double acc_prob = 0;
    int i = 0;

    for (; i < rule.func_num; i++) {
        acc_prob += rule.sub_rules[i].prob;
        if (rand_value < acc_prob)
            break;
    }

    return i;
}

void func_info_cpy(FuncInfo *dest, FuncInfo *source)
{
    dest->func = source->func;
    dest->arity = source->arity;
    strcpy(dest->func_name, source->func_name);
}

char *get_real_line(char *buffer, int buffer_size, FILE *file)
{
    char *res;
    char *buffer_cpy = (char*)malloc(sizeof(char) * buffer_size);
    while ((res = fgets(buffer, buffer_size, file))) {
        char *first_char;
        strcpy(buffer_cpy, buffer);
        first_char = strtok(buffer_cpy, " \t\n");
        if (first_char && first_char[0] != '#') {
            break;
        }
    }
    free(buffer_cpy);
    return res;
}

int find_symbol(char *symbol, char symbol_arr[MAX_RULE_NUM][MAX_SYMBOL_LEN + 1], int symbol_arr_size)
{
    for (int i = 0; i < symbol_arr_size; i++) {
        if (strcmp(symbol, symbol_arr[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int find_func(char *func_name)
{
    int i = 0; 
    for (; i < sizeof(func_collection) / sizeof(FuncInfo); i++) {
        if (strcmp(func_name, func_collection[i].func_name) == 0)
            return i;
    }
    return -1;
}

int parse_from_file(char *file_name, int entry_symbol_arr[3], Rule grammar[MAX_RULE_NUM])
{
    FILE *file = fopen(file_name, "r");
    char line_buffer[1024];

    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", file_name);
        return EXIT_FAILURE;
    }

    /* Get defined symbols */
    char symbol_arr[MAX_RULE_NUM][MAX_SYMBOL_LEN + 1];
    char *symbol;
    int symbol_pos;
    int symbol_arr_size = 0;

    if (!get_real_line(line_buffer, sizeof(line_buffer), file)) {
        perror("Symbol definitions expected\n");
        return EXIT_FAILURE;
    }
    symbol = strtok(line_buffer, " \t\n");
    while (symbol) {
        if (strlen(symbol) > MAX_SYMBOL_LEN) {
            fprintf(stderr, 
                    "The name \"%s\" exceeds the maximum length of %d characters\n",
                    symbol, MAX_SYMBOL_LEN);
            return EXIT_FAILURE;
        }
        strcpy(symbol_arr[symbol_arr_size++], symbol);
        symbol = strtok(NULL, " \t\n");
    }

    /* Get entry symbols */
    int entry_symbol_arr_size = 0;

    if (!get_real_line(line_buffer, sizeof(line_buffer), file)) {
        perror("Entry symbols expected\n");
        return EXIT_FAILURE;
    }
    symbol = strtok(line_buffer, " \t\n");
    while (symbol) {
        if (entry_symbol_arr_size > 3) {
            fprintf(stderr, "Too many entry symbols; 3 entry symbols are expected (RGB)\n");
            return EXIT_FAILURE;
        }
        symbol_pos = find_symbol(symbol, symbol_arr, symbol_arr_size);
        if (symbol_pos < 0) {
            fprintf(stderr, "Symbol \"%s\" is not defined\n", symbol);
            return EXIT_FAILURE;
        }
        entry_symbol_arr[entry_symbol_arr_size++] = symbol_pos;
        symbol = strtok(NULL, " \t\n");
    }
    if (entry_symbol_arr_size < 3) {
        fprintf(stderr, "3 entry symbols are expected (RGB); only got %d\n", entry_symbol_arr_size);
        return EXIT_FAILURE;
    }

    /* Get the rules */
    while (get_real_line(line_buffer, sizeof(line_buffer), file)) {
        char *arrow;
        int master_pos;
        int func_cnt = 0;

        symbol = strtok(line_buffer, " \t\n");
        symbol_pos = find_symbol(symbol, symbol_arr, symbol_arr_size);
        if (symbol_pos < 0) {
            fprintf(stderr, "Symbol \"%s\" is not defined\n", symbol);
            return EXIT_FAILURE;
        }
        master_pos = symbol_pos;
        arrow = strtok(NULL, " \t\n");
        if (strcmp(arrow, "->") != 0) {
            fprintf(stderr, "-> is expected to define a rule\n");
            return EXIT_FAILURE;
        }

        while (1) {
            char *prob;
            char *func_name;
            int arity = 0;
            int func_pos;
            prob = strtok(NULL, " \t\n");
            if (!atof(prob)) {
                fprintf(stderr, "Float for probability is expected, got \"%s\"\n", prob);
                return EXIT_FAILURE;
            }
            grammar[master_pos].sub_rules[func_cnt].prob = atof(prob);
            func_name = strtok(NULL, " \t\n");
            func_pos = find_func(func_name);
            if (func_pos < 0) {
                fprintf(stderr, "Function \"%s\" is not valid\n", func_name);
                return EXIT_FAILURE;
            }
            func_info_cpy(&grammar[master_pos].sub_rules[func_cnt].func_info, &func_collection[func_pos]);
            while (1) {
                symbol = strtok(NULL, " \t\n");
                if (!symbol || strcmp(symbol, "|") == 0)
                    break;
                symbol_pos = find_symbol(symbol, symbol_arr, symbol_arr_size);
                if (symbol_pos < 0) {
                    fprintf(stderr, "Symbol \"%s\" is not defined\n", symbol);
                }
                grammar[master_pos].sub_rules[func_cnt].args[arity++] = symbol_pos;
            }
            if (arity != grammar[master_pos].sub_rules[func_cnt].func_info.arity) {
                fprintf(stderr, "%s expect %d arguments; got %d\n", 
                        grammar[master_pos].sub_rules[func_cnt].func_info.func_name,
                        grammar[master_pos].sub_rules[func_cnt].func_info.arity,
                        arity);
                return EXIT_FAILURE;
            }
            func_cnt++;
            if (!symbol)
                break;
        }
        grammar[master_pos].func_num = func_cnt;
    }


    fclose(file);

    return EXIT_SUCCESS;
}


/* functions in expressions */
double add(double *nums)
{
    return (nums[0] + nums[1]) / 2;
}

double mult(double *nums)
{
    return nums[0] * nums[1];
}

double identity(double *nums)
{
    return nums[0];
}

double rand_norm(double *nums)
{
    /* returns a random number between -1 and 1 */
    return (double)rand() / RAND_MAX * 2 - 1;
}

double get_x(double *nums)
{
    return nums[0];
}

double get_y(double *nums)
{
    return nums[1];
}

double sin_func(double *nums)
{
    return sin(PI * nums[0]);
}

double neg_func(double *nums)
{
    return -nums[0];
}

double sqrt_func(double *nums)
{
    return sqrt((nums[0] + 1) * 2) - 1;
}

double mix(double *nums)
{
    double prop = (nums[2] + 1) / 2;
    return nums[0] * prop + nums[1] * (1 - prop);
}

double tan_func(double *nums)
{
    return 1 / (1 + exp(-tan(PI * nums[0]))) * 2 - 1;
}


int main(int argc, char **argv)
{
    int exit_code;
    int entry_symbol_arr[3] = {0};
    Rule grammar[MAX_RULE_NUM] = {0};
    if (argc != 2) {
        fprintf(stderr, "Usage: ./rart <grammar_file>\n");
        exit(1);
    }

    exit_code = parse_from_file(argv[1], entry_symbol_arr, grammar);
    if (exit_code == EXIT_FAILURE)
        exit(1);

    unsigned char img[IMG_HEIGHT * IMG_WIDTH * 3];
    srand(time(NULL));
    ExpressionNode *r_root = buildExpressionTree(grammar, entry_symbol_arr[0], 10);
    ExpressionNode *g_root = buildExpressionTree(grammar, entry_symbol_arr[1], 10);
    ExpressionNode *b_root = buildExpressionTree(grammar, entry_symbol_arr[2], 10);

    printExpressionTree(r_root);
    printf("\n");
    printExpressionTree(g_root);
    printf("\n");
    printExpressionTree(b_root);
    printf("\n");

    for (int i = 0; i < IMG_HEIGHT; i++) {
        for (int j = 0; j < IMG_WIDTH; j++) {
            int idx = (i * IMG_HEIGHT + j) * 3;
            double x_norm = i / (double)IMG_HEIGHT * 2 - 1;
            double y_norm = j / (double)IMG_WIDTH * 2 - 1;
            img[idx + 0] = (evaluateExpressionTree(r_root, x_norm, y_norm) + 1) / 2 * 255;
            img[idx + 1] = (evaluateExpressionTree(g_root, x_norm, y_norm) + 1) / 2 * 255;
            img[idx + 2] = (evaluateExpressionTree(b_root, x_norm, y_norm) + 1) / 2 * 255;
        }
    }

    if (stbi_write_png("randart.png", IMG_WIDTH, IMG_HEIGHT, 3, img, 3 * IMG_WIDTH)) {
        printf("Image saved as 'randart.png'\n");
    } else {
        fprintf(stderr, "Failed to save image\n");
    }

    freeExpressionTree(r_root);
    freeExpressionTree(g_root);
    freeExpressionTree(b_root);

    return 0;
}
