#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int k;
    int tests_for_thread; // кол-во экспериментов для каждого потока
    int *first_win;
    int *second_win;
    int *draw;
    unsigned int seed;
} ThreadArgs;

pthread_mutex_t lock; 

void *simulate_games(void *arg) // как ведется подсчет очков
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int first_win_local = 0, second_win_local = 0, draw_local = 0;

    for (int i = 0; i < args->tests_for_thread; i++)
    {
        int sum1 = 0, sum2 = 0;

        for (int j = 0; j < args->k; j++)
        {
            sum1 += rand_r(&args->seed) % 6 + rand_r(&args->seed) % 6 + 2;
            sum2 += rand_r(&args->seed) % 6 + rand_r(&args->seed) % 6 + 2;
        }

        if (sum1 > sum2)
            first_win_local++;
        else if (sum1 < sum2)
            second_win_local++;
        else
            draw_local++;
    }
    pthread_mutex_lock(&lock);
    *(args->first_win) += first_win_local;
    *(args->second_win) += second_win_local;
    *(args->draw) += draw_local;
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(int argc, char *argv[])
{
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int k = 0, all_tests = 0, threads_count = 1;
    int round = 0, score1 = 0, score2 = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-k") == 0)
            k = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0)
            round = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s1") == 0)
            score1 = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s2") == 0)
            score2 = atoi(argv[++i]);
        else if (strcmp(argv[i], "-n") == 0)
            all_tests = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0)
            threads_count = atoi(argv[++i]);
    }

    if (k <= 0 || all_tests <= 0)
    {
        char error_msg[256];
        int len = snprintf(error_msg, sizeof(error_msg),
                           "Usage: %s -k K [-r ROUND] [-s1 SCORE1] [-s2 SCORE2] -n EXPERIMENTS [-t THREADS]\n", argv[0]);
        write(2, error_msg, len);
        return 1;
    }

    pthread_mutex_init(&lock, NULL);
    int first_win = 0, second_win = 0, draw = 0;
    pthread_t *threads = malloc(threads_count * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(threads_count * sizeof(ThreadArgs));
    int tests_for_thread = all_tests / threads_count;

    for (int i = 0; i < threads_count; i++)
    {
        thread_args[i].k = k;
        thread_args[i].tests_for_thread = tests_for_thread;
        if (i == threads_count - 1)
        {
            thread_args[i].tests_for_thread += all_tests % threads_count;
        }
        thread_args[i].first_win = &first_win;
        thread_args[i].second_win = &second_win;
        thread_args[i].draw = &draw;
        thread_args[i].seed = time(NULL) ^ getpid() ^ i;
        pthread_create(&threads[i], NULL, simulate_games, &thread_args[i]);
    }

    for (int i = 0; i < threads_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(thread_args);

     // создаем итоговые переменные, которые учитывают начальный счет
    int final_wins1 = first_win + score1;
    int final_wins2 = second_win + score2;
    // общее количество игр теперь тоже больше (эксперименты + начальный счет)
    int total_games = all_tests + score1 + score2;
    char buffer[512];
    int len;

    len = snprintf(buffer, sizeof(buffer), "All tests: %d\n", total_games);
    write(1, buffer, len);
    len = snprintf(buffer, sizeof(buffer), "Player 1 wins: %d %.2f%%\n",
                   final_wins1, (100.0 * final_wins1 / total_games));
    write(1, buffer, len);
    len = snprintf(buffer, sizeof(buffer), "Player 2 wins: %d %.2f%%\n",
                   final_wins2, (100.0 * final_wins2 / total_games));
    write(1, buffer, len);
    len = snprintf(buffer, sizeof(buffer), "Draw: %d %.2f%%\n",
                   draw, (100.0 * draw / total_games));
    write(1, buffer, len);

    pthread_mutex_destroy(&lock);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double execution_time = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    len = snprintf(buffer, sizeof(buffer), "Execution time: %.6f seconds\n", execution_time);
    write(1, buffer, len);

    return 0;
}