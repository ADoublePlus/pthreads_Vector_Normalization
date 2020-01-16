#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

// Auxiliary data structure for working with pthreads
struct thread_operation_data
{
    int vectorSize;
    float *vector; // Operand 1
    float op2; // Operand 2
};

// Prints the vector to the screen
void print_vector(float *vector, int vectorSize)
{
    printf("[ ");

    for (int i = 0; i < vectorSize; i++)
    {
        printf("%f", vector[i]);
    }

    printf("] \n");
}

// Attach and split pthreads
void join_division_pthreads(pthread_t threads[], const int nThreads)
{
    for (int i = 0; i < nThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

// Divide each element of the vector by the module of the same and save it in the same standardized vector
void *division(void *arg)
{
    thread_operation_data data_in = *((thread_operation_data *) arg);

    for (int i = 0; i < data_in.vectorSize; i++)
    {
        data_in.vector[i] = data_in.vector[i] / data_in.op2;
    }

    pthread_exit(NULL);
}

// Divide the vector into as many equal parts as pthreads
// Each pthread is assigned a part of the vector to work with: Appy the division function with the operand module as its denominator
// If the vector can't be divided into equal parts, the first pthread gets left with the rest of the work
void create_division_pthreads(float *vector, const int vectorSize, pthread_t threads[], thread_operation_data tasks[], const int nThreads, const float module)
{
    //printf("Creating division pthreads...\n");

    const int range = (vectorSize / nThreads);
    const int rest = (vectorSize % nThreads);

    for (int i = 0; i < nThreads; i++)
    {
        tasks[i].vectorSize = i == 0 ? range + rest : range;
        tasks[i].op2 = module;
        tasks[i].vector = (i == 0 ? &vector[(tasks[i].vectorSize * i)] : &vector[(tasks[i].vectorSize * i) + rest]);

        pthread_create(&threads[i], NULL, division, &tasks[i]);
    }
}

// Helper for all pthreads
// Adds the results of the different pthreads and returns them
float join_power_pthreads(pthread_t threads[], const int nThreads, float* sum)
{
    //printf("Joining power pthreads\n");

    *sum = 0;

    for (int i = 0; i < nThreads; i++)
    {
        float *result = 0;
        pthread_join(threads[i], ((void **) &result));
        *sum += *result;
        free(result);
    }

    return sum;
}

// To the given vector segment, apply the power function with the given operand
// Add up all the powers and return them
void *power(void *arg)
{
    thread_operation_data data = *((thread_operation_data *) arg);

    float sum = 0;

    for (int i = 0; i < data.vectorSize; i++)
    {
        sum += pow(data.vector[i], data.op2);
    }

    float * result = (float*) malloc(sizeof(float));
    *result = sum;
    return (void *) result;
}

// Divide the vector into as many equal parts as pthreads
// Each pthread is assigned a part of the vector to work with: Apply the power function with an exponent such as operand2
// If the vector can't be divided into equal parts, the first pthread gets left with the rest of the work
void create_power_pthreads(float *vector, const int vectorSize, pthread_t threads[], thread_operation_data tasks[], const int nThreads, const int exponent)
{
    //printf("Creating power pthreads...\n");

    const int range = (vectorSize / nThreads);
    const int rest = (vectorSize % nThreads);

    for (int i = 0; i < nThreads; i++)
    {
        tasks[i].vectorSize = (i == 0 ? range + rest : range);
        tasks[i].op2 = exponent;
        tasks[i].vector = (i == 0 ? &vector[(tasks[i].vectorSize * i)] : &vector[(tasks[i].vectorSize * i) + rest]);

        pthread_create(&threads[i], NULL, power, &tasks[i]);
    }
}

// Normalize a 'vectorSize' size vector in parallel with nThreads
void norm_vector_pthreads(float *vector, const int vectorSize, const int nThreads)
{
    pthread_t threads[nThreads];
    thread_operation_data tasks[nThreads];
    float *sum = (float *) malloc(sizeof(float));

    create_power_pthreads(vector, vectorSize, threads, tasks, nThreads, 2);
    join_power_pthreads(threads, nThreads, sum);
    create_division_pthreads(vector, vectorSize, threads, tasks, nThreads, sqrt(*sum));
    join_division_pthreads(threads, nThreads);

    free(sum);
}

// Normalize a size vector sequentially
void norm_vector_seq(float *vector, int size)
{
    float module = 0;

    for (long i = 0; i < size; i++)
    {
        module += pow(vector[i], 2);
    }

    module = sqrt(module);

    for (long i = 0; i < size; i++)
    {
        vector[i] = vector[i] / module;
    }
}

// Returns a floating point number between a min and max value
float random_float(const int min, const int max)
{
    float random = min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
    return random;
}

// Generate a random vector of numbers ranging from 0 to vectorSize
// Interval difference of 1
// There is only one number this 0 and 1, only one between 1 and 2, etc
void generate_random_vector(float* vector, const int vectorSize)
{
    srand(time(NULL));

    for (int i = 0; i < vectorSize; i++)
    {
        vector[i] = random_float(i, i + 1);
    }

    for (int i = vectorSize - 1; i > 0; i--)
    {
        int random = rand() % i;

        float new_value = vector[i];
        vector[i] = vector[random];
        vector[random] = new_value;
    }
}

// Returns the time difference in nanoseconds between start and end
timespec diff(timespec &start, timespec &end)
{
    timespec temp;

    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else 
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return temp;
}

// Example of execution with a vector of size 10 and two pthreads 
int main(int argc, char **argv)
{
    timespec start, end, elapsed;
    int executions;
    int vectorSize = atoi(argv[1]);
    int nThreads = atoi(argv[2]);

    float* vector = (float *) malloc(vectorSize * sizeof(float));

    printf("Starting...\tSize = %d\tnThreads = %d\n", vectorSize, nThreads);

    generate_random_vector(vector, vectorSize);
    //printf("Random vector: \t"); 
    //print_vector(vector, vectorSize);

    clock_gettime(CLOCK_REALTIME, &start);
    norm_vector_pthreads(vector, vectorSize, nThreads);

    //printf("Result vector: \t");
    //print_vector(vector, vectorSize);

    free(vector);

    clock_gettime(CLOCK_REALTIME, &end);

    elapsed = diff(start, end);
    double time_sec = (double) elapsed.tv_sec;
    double time_nsec = (double) elapsed.tv_nsec;
    double time_msec = (time_sec * 1000) + (time_nsec / 1000000);

    printf("Time: \t %f msecs \n", time_msec);
    printf("Exit\n");
    exit(EXIT_SUCCESS);
}