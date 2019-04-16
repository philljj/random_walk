#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RAND_BUF_LEN (6)
#define MAX_RAND_NUM (65535)

static char     prog_name[] = "run_walk";
static size_t   avoid = 0;
static size_t   sleep_interval = 0;
static size_t   quiet = 0;
static uint16_t rand_buf[RAND_BUF_LEN];
static int      urand_fd = 0;
static size_t   pos_ = 0;

typedef enum {
    RW_GOOD = 0,
    RW_COLL = 1,
    RW_DONE = 2
} avoid_t;

static void    print_usage_and_die(void) __attribute__((noreturn));
static size_t  get_len(const char * arg);
static void    init_rand(void);
static size_t  safer_rand(const size_t min, const size_t max);
static void    print_board(const char * board, const size_t len);
static avoid_t get_avoid(const char * board, const size_t len,
                         const size_t i, const size_t j,
                         const size_t dir);



int
main(int    argc,
     char * argv[])
{
    if (argc < 2) { print_usage_and_die(); }

    int    opt = 0;
    size_t len = 0;

    while ((opt = getopt(argc, argv, "n:s:aq")) != -1) {
        switch (opt) {
        case 'a':
            avoid = 1;
            break;

        case 's':
            sleep_interval = get_len(optarg);
            break;

        case 'q':
            quiet = 1;
            break;

        case 'n':
            len = get_len(optarg);
            break;

        case '?':
        default:
            print_usage_and_die();
        }
    }

    if (len >= 100) {
        fprintf(stderr, "error: number is too large! Must be < 100\n");
        print_usage_and_die();
    }

    if (len < 2) {
        fprintf(stderr, "error: need a length > 2\n");
        print_usage_and_die();
    }

    init_rand();

    char * board = malloc(len * len);

    memset(board, ' ', len * len);

    size_t i = safer_rand(0, len - 1);
    size_t j = safer_rand(0, len - 1);

    board[i * len + j] = 'M';

    if (!quiet) {
        print_board(board, len);
    }

    size_t done = 0;

    for (;;) {
        size_t dir = safer_rand(0, 3);

        if (avoid) {
            switch (get_avoid(board, len, i, j, dir)) {
            case RW_GOOD:
                break;
            case RW_COLL:
                continue;
            case RW_DONE:
                done = 1;
                break;
            }
        }

        if (done) { break; }

        board[i * len + j] = '.';

        switch (dir) {
        case 0:
            if (i) { --i; }
            else { continue; }
            break;
        case 1:
            if (j) { --j; }
            else { continue; }
            break;
        case 2:
            if (i < len - 1) { ++i; }
            else { continue; }
            break;
        case 3:
            if (j < len - 1) { ++j; }
            else { continue; }
            break;
        default:
            fprintf(stderr, "error: don't know about dir = %zu\n", dir);

        }

        board[i * len + j] = 'M';

        if (sleep_interval) {
            usleep(sleep_interval);
        }

        if (!quiet) {
            print_board(board, len);
        }
    }

    print_board(board, len);
}



static void
print_usage_and_die(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s -n <len> -b <born> -l <low> -h <high> -dq", prog_name);
    fprintf(stderr, "\noptions:\n");
    fprintf(stderr, "  n: the length of square board. 2-99\n");
    fprintf(stderr, "  b: the born number in BnSij rule\n");
    fprintf(stderr, "  l: the low survival number in BnSij rule\n");
    fprintf(stderr, "  h: the high survival number in BnSij rule\n");
    fprintf(stderr, "  s: sleep interval\n");
    fprintf(stderr, "  q: quiet mode. Enable to reduce print output\n");
    fprintf(stderr, "  d: debug mode (prints function entry)\n");

    exit(EXIT_FAILURE);
}



static size_t
get_len(const char * arg)
{
    const char * n = arg;

    while (*arg) {
        if (!isdigit(*arg)) {
            fprintf(stderr, "error: %c of %s is not a number\n",
                    *arg, arg);
            print_usage_and_die();
        }

        ++arg;
    }

    return atoi(n);
}



void
init_rand(void)
{
    urand_fd = open("/dev/urandom", O_RDONLY);

    if (urand_fd <= 0) {
        fprintf(stderr, "error: failed to open %s\n", "/dev/urandom");
        exit(1);
    }

    ssize_t n = read(urand_fd, rand_buf, sizeof(rand_buf));

    if (n != sizeof(rand_buf)) {
        fprintf(stderr, "error: read returned %zu bytes, expected %zu\n",
                n, sizeof(rand_buf));
        exit(1);
    }

    pos_ = 0;

    return;
}



size_t
safer_rand(const size_t min,
           const size_t max)
{
    if (!urand_fd) {
        fprintf(stderr, "error: do not call safer_rand before init!\n");
        exit(1);
    }

    if (max <= min) {
        fprintf(stderr, "error: invalid arguments: min=%zu, max=%zu\n",
                         min, max);
        exit(1);
    }

    size_t range = (max + 1) - min;
    size_t shift = MAX_RAND_NUM % range;

    if (pos_ > RAND_BUF_LEN - 1) {
        ssize_t n = read(urand_fd, rand_buf, sizeof(rand_buf));

        if (n != sizeof(rand_buf)) {
            fprintf(stderr, "error: read returned %zu bytes, expected %zu\n",
                    n, sizeof(rand_buf));
            exit(1);
        }

        pos_ = 0;
    }

    size_t result = ((rand_buf[pos_] - shift) % range) + min;

    pos_++;

    return result;
}



static void
print_board(const char * board, const size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        for (size_t j = 0; j < len; ++j) {
            if (board[i * len + j] == 'M') {
                printf("\e[1;35m%c\e[0m ", board[i * len + j]);
            }
            else {
                printf("%c ", board[i * len + j]);
            }
        }

        printf("\n");
    }

    return;
}




static avoid_t
get_avoid(const char * board,
          const size_t len,
          const size_t i,
          const size_t j,
          const size_t dir)
{
    size_t dir_coll[4] = {0, 0, 0, 0};
    size_t dir_wall[4] = {0, 0, 0, 0};

    if (i) {
        dir_wall[0] = 1;

        if (board[(i - 1) * len + j] == ' ') {
            dir_coll[0] = 1;
        }
    }

    if (j) {
        dir_wall[1] = 1;

        if (board[i * len + j - 1] == ' ') {
            dir_coll[1] = 1;
        }
    }

    if (i < len - 1) {
        dir_wall[2] = 1;

        if (board[(i + 1) * len + j] == ' ') {
            dir_coll[2] = 1;
        }
    }

    if (j < len - 1) {
        dir_wall[3] = 1;

        if (board[i * len + j + 1] == ' ') {
            dir_coll[3] = 1;
        }
    }

    if (!avoid) {
        if (dir_wall[dir]) {
            return RW_GOOD;
        }

        return RW_COLL;
    }

    if (dir_coll[dir]) {
        return RW_GOOD;
    }

    for (size_t l = 0; l < 4; ++l) {
        if (dir_coll[l]) {
            return RW_COLL;
        }
    }

    return RW_DONE;
}
