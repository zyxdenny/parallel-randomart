#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
#define DEPTH_THRESHOLD   4

enum {
    X,
    Y,
    RAND_NUM,
};

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
    double rand_num;
} ExpressionNode;


ExpressionNode *build_expression_tree(Rule *grammar, int pos, int depth);
double evaluate_expression_tree(ExpressionNode *root, double x, double y, int depth);
double evaluate_expression_tree_parallel(ExpressionNode *root, double x, double y, int depth);
void free_expression_tree(ExpressionNode *root);
int rand_with_weight(Rule rule);
void func_info_cpy(FuncInfo *dest, FuncInfo *source);
char *get_real_line(char *buffer, int buffer_size, FILE *file);
int find_symbol(char *symbol, char symbol_arr[MAX_RULE_NUM][MAX_SYMBOL_LEN + 1], int symbol_arr_size);
int find_func(char *func_name);
int parse_from_file(char *file_name, int entry_symbol_arr[3], Rule grammar[MAX_RULE_NUM]);
void fill_image_loop_parallel(unsigned char *img, int width, int height,
        ExpressionNode *r_root, ExpressionNode *g_root, ExpressionNode *b_root,
        int threads_cnt);
void fill_image_rec_parallel(unsigned char *img, int width, int height,
        ExpressionNode *r_root, ExpressionNode *g_root, ExpressionNode *b_root,
        int threads_cnt);

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
double eight_sum(double *nums);


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
    { eight_sum,   8,   "EIGHT_SUM" },
};

ExpressionNode *build_expression_tree(Rule *grammar, int pos, int depth)
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
        res->args[i] = build_expression_tree(grammar, arg, depth - 1);
    }
    res->rand_num = (double)rand() / RAND_MAX * 2 - 1;

    return res;
}

double evaluate_expression_tree(ExpressionNode *root, double x, double y, int depth)
{
    double params[MAX_ARG_NUM];
    double res;

    if (root->func_info.arity == 0) {
        params[X] = x;
        params[Y] = y;
        params[RAND_NUM] = root->rand_num;
        return root->func_info.func(params);
    }

    for (int i = 0; i < root->func_info.arity; i++) {
        params[i] = evaluate_expression_tree(root->args[i], x, y, depth + 1);
    }

    res = root->func_info.func(params);
    return res;
}

double evaluate_expression_tree_parallel(ExpressionNode *root, double x, double y, int depth)
{
    double params[MAX_ARG_NUM];
    double res;

    if (root->func_info.arity == 0) {
        params[X] = x;
        params[Y] = y;
        params[RAND_NUM] = root->rand_num;
        return root->func_info.func(params);
    }

    if (depth < DEPTH_THRESHOLD) {
        for (int i = 0; i < root->func_info.arity; i++) {
#           pragma omp task shared(params)
            params[i] = evaluate_expression_tree(root->args[i], x, y, depth + 1);
        }

#       pragma omp taskwait
    }
    else {
        for (int i = 0; i < root->func_info.arity; i++) {
            params[i] = evaluate_expression_tree(root->args[i], x, y, depth + 1);
        }
    }

    res = root->func_info.func(params);
    return res;
}

void free_expression_tree(ExpressionNode *root)
{
    for (int i = 0; i < root->func_info.arity; i++) {
        free_expression_tree(root->args[i]);
    }

    free(root);
}

void print_expression_tree(ExpressionNode *root)
{
    printf("%s", root->func_info.func_name);
    printf("(");
    for (int i = 0; i < root->func_info.arity; i++) {
        print_expression_tree(root->args[i]);
        if (i != root->func_info.arity - 1)
            printf(", ");
    }
    printf(")");
}

int rand_with_weight(Rule rule)
{
    float rand_value = (float)rand() / (float)RAND_MAX;
    float acc_prob = 0;
    int i = 0;

    for (; i < rule.func_num; i++) {
        acc_prob += rule.sub_rules[i].prob;
        if (rand_value < acc_prob)
            return i;
    }

    if (i != 0)
        return i - 1;
    return 0;
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

void fill_image_loop_parallel(unsigned char *img, int width, int height,
        ExpressionNode *r_root, ExpressionNode *g_root, ExpressionNode *b_root,
        int threads_cnt)
{
#   pragma omp parallel for num_threads(threads_cnt) collapse(2)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = (i * width + j) * 3;
            double x_norm = (double)i / (double)height * 2 - 1;
            double y_norm = (double)j / (double)width * 2 - 1;
            img[idx + 0] = (evaluate_expression_tree(r_root, x_norm, y_norm, 0) + 1) / 2 * 255;
            img[idx + 1] = (evaluate_expression_tree(g_root, x_norm, y_norm, 0) + 1) / 2 * 255;
            img[idx + 2] = (evaluate_expression_tree(b_root, x_norm, y_norm, 0) + 1) / 2 * 255;
        }
    }
}

void fill_image_rec_parallel(unsigned char *img, int width, int height,
        ExpressionNode *r_root, ExpressionNode *g_root, ExpressionNode *b_root,
        int threads_cnt)
{
#   pragma omp parallel num_threads(threads_cnt)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = (i * width + j) * 3;
            double x_norm = (double)i / (double)height * 2 - 1;
            double y_norm = (double)j / (double)width * 2 - 1;
#           pragma omp single
            img[idx + 0] = (evaluate_expression_tree_parallel(r_root, x_norm, y_norm, 0) + 1) / 2 * 255;
#           pragma omp single
            img[idx + 1] = (evaluate_expression_tree_parallel(g_root, x_norm, y_norm, 0) + 1) / 2 * 255;
#           pragma omp single
            img[idx + 2] = (evaluate_expression_tree_parallel(b_root, x_norm, y_norm, 0) + 1) / 2 * 255;
        }
    }
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

double get_x(double *nums)
{
    return nums[X];
}

double get_y(double *nums)
{
    return nums[Y];
}

double rand_norm(double *nums)
{
    return nums[RAND_NUM];
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

double eight_sum(double *nums)
{
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += nums[i];
    }
    return sum;
}


int main(int argc, char **argv)
{
    // Parse command line
    char *grammar_file = NULL;
    char *output_file = "out.png";
    int width = IMG_WIDTH, height = IMG_HEIGHT, threads_cnt = 1, depth = 5;
    int flag_cmp = 0;
    int flag_print = 0;
    int flag_rec_parallel = 0;

    // Ensure at least GRAMMAR_FILE is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s GRAMMAR_FILE [-o OUTPUT_FILE] [-w WIDTH] [-h HEIGHT] [-t NUM_THREADS] [-d DEPTH] [-c] [-p] [-r]\n", argv[0]);
        return 1;
    }

    // GRAMMAR_FILE is the first argument
    grammar_file = argv[1];

    // Parse optional arguments using getopt
    int opt;
    while ((opt = getopt(argc - 1, argv + 1, "o:w:h:t:d:cpr")) != -1) {
        switch (opt) {
        case 'o':
            output_file = optarg;
            break;
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 't':
            threads_cnt = atoi(optarg);
            break;
        case 'd':
            depth = atoi(optarg);
            break;
        case 'c':
            flag_cmp = 1;
            break;
        case 'p':
            flag_print = 1;
            break;
        case 'r':
            flag_rec_parallel = 1;
            break;
        default: // Invalid option
            fprintf(stderr,
                    "Usage: %s GRAMMAR_FILE [-o OUTPUT_FILE] [-w WIDTH] [-h HEIGHT] [-d DEPTH] [-t NUM_THREADS] [-c] [-p] [-r]\n",
                    argv[0]);
            return 1;
        }
    }

    int exit_code;
    int entry_symbol_arr[3] = {0};
    Rule grammar[MAX_RULE_NUM] = {0};
    exit_code = parse_from_file(grammar_file, entry_symbol_arr, grammar);
    if (exit_code == EXIT_FAILURE)
        return 1;

    unsigned char *img = (unsigned char*)malloc(sizeof(unsigned char) * height * width * 3);
    srand(time(NULL));
    ExpressionNode *r_root = build_expression_tree(grammar, entry_symbol_arr[0], depth);
    ExpressionNode *g_root = build_expression_tree(grammar, entry_symbol_arr[1], depth);
    ExpressionNode *b_root = build_expression_tree(grammar, entry_symbol_arr[2], depth);

    if (flag_print) {
        printf("R channel:\n");
        print_expression_tree(r_root);
        printf("\n\n");
        printf("G channel:\n");
        print_expression_tree(g_root);
        printf("\n\n");
        printf("B channel:\n");
        print_expression_tree(b_root);
        printf("\n\n");
    }


    double tstart, tstop, ttaken;
    tstart = omp_get_wtime();
    if (flag_rec_parallel) {
        printf("Recursion parallel algorithm is chosen\n\n");
        fill_image_rec_parallel(img, width, height, r_root, g_root, b_root, threads_cnt);
    }
    else {
        printf("Loop parallel algorithm is chosen\n\n");
        fill_image_loop_parallel(img, width, height, r_root, g_root, b_root, threads_cnt);
    }
    tstop = omp_get_wtime();
    ttaken = tstop - tstart;
    printf("Time taken for generating the image with %d threads is: %.4f\n", threads_cnt, ttaken);

    if (flag_cmp) {
        struct timespec tstart_1, tstop_1;
        double ttaken_1;
        clock_gettime(CLOCK_MONOTONIC, &tstart_1);
        fill_image_loop_parallel(img, width, height, r_root, g_root, b_root, 1);
        clock_gettime(CLOCK_MONOTONIC, &tstop_1);
        ttaken_1 = (tstop_1.tv_sec - tstart_1.tv_sec) + 
            (tstop_1.tv_nsec - tstart_1.tv_nsec) / 1e9;
        printf("Time taken for generating the image sequentially is: %.4f\n", ttaken_1);
        printf("Speedup: %.4f    Efficiency: %.4f\n\n", ttaken_1 / ttaken, ttaken_1 / ttaken / threads_cnt);

        // if (stbi_write_png("comp.png", width, height, 3, img, 3 * width)) {
        //     printf("Image saved as comp.png\n");
        // } else {
        //     fprintf(stderr, "Failed to save image\n");
        //     return 1;
        // }
    }

    if (stbi_write_png(output_file, width, height, 3, img, 3 * width)) {
        printf("Image saved as %s\n", output_file);
    } else {
        fprintf(stderr, "Failed to save image\n");
        return 1;
    }

    free(img);
    free_expression_tree(r_root);
    free_expression_tree(g_root);
    free_expression_tree(b_root);

    return 0;
}
