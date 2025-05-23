#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"
#include <time.h>

struct nums {
	long *increase;
	long *decrease;
	long total;
	long diff;
};

struct args {
	int thread_num;		                // application defined thread #
	long iterations;	                // number of operations
	struct nums *nums;	                // pointer to the counters (shared with other threads)
    pthread_mutex_t** mutex;            // mutex (shared with other threads), one for each unit of the array
    pthread_mutex_t* iterations_mutex;  // mutex for number of iterations left (shared with other threads)
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};


int iterations_left;
int array_size=0;

/*
void *decrease_shift(void* ptr)
{
    struct args *args = ptr;
	struct nums *n = args->nums;
    int random_number1, random_number2;

    

    while(1) {
        //variable compartida: iterations_left
        pthread_mutex_lock(args->iterations_mutex);
        iterations_left--;
        if(iterations_left < 0){
            pthread_mutex_unlock(args->iterations_mutex);
            break;
        }

        

        pthread_mutex_unlock(args->iterations_mutex);
        
        random_number1 = rand() % array_size;
        random_number2 = rand() % array_size;

        //variable compartida: decrease[random_number] e increase[random_number]
        pthread_mutex_lock(args->mutex[random_number1]);
        if(random_number1 != random_number2){
            pthread_mutex_lock(args->mutex[random_number2]);
        }


		n->decrease[random_number1]++;
		n->decrease[random_number2]--;
        

		long diff = n->total - (n->decrease[random_number1] + n->decrease[random_number2]);
	
    	if (diff != n->diff) {
			n->diff = diff;
			printf("Shift thread %d in decrease array positions %d,%d increasing %ld decreasing %ld diff %ld\n",
			       args->thread_num, random_number1, random_number2, n->decrease[random_number1], n->decrease[random_number1], diff);
		}

        pthread_mutex_unlock(args->mutex[random_number1]);
        if(random_number1 != random_number2){
            pthread_mutex_unlock(args->mutex[random_number2]);
        }
    }
    return NULL;


}
*/
void *increase_shift(void* ptr)
{
    struct args *args = ptr;
	struct nums *n = args->nums;
    int random_number1, random_number2,val1,val2;


    while(1) {
        //variable compartida: iterations_left
        pthread_mutex_lock(args->iterations_mutex);
        iterations_left--;
        if(iterations_left < 0){
            pthread_mutex_unlock(args->iterations_mutex);
            break;
        }

        

        pthread_mutex_unlock(args->iterations_mutex);
        
        random_number1 = rand() % array_size;
        random_number2 = rand() % array_size;

        while(random_number1 == random_number2){
            random_number2 = rand() % array_size;
        }

        //variable compartida: decrease[random_number] e increase[random_number]
        pthread_mutex_lock(args->mutex[random_number1]);
        if(pthread_mutex_trylock(args->mutex[random_number2])){
            pthread_mutex_unlock(args->mutex[random_number1]);
            continue;
        }
        val1=n->increase[random_number1];
        val2=n->increase[random_number2];
		
        val1--;
		val2++;
        
        n->increase[random_number1]=val1;
        n->increase[random_number2]=val2;

		long diff = n->total - (n->increase[random_number1] + n->increase[random_number2]);
	
    	if (diff != n->diff) {
			n->diff = diff;
			printf("Shift thread %d in increase array positions %d,%d increasing %ld decreasing %ld diff %ld\n",
			       args->thread_num, random_number1, random_number2, n->increase[random_number1], n->increase[random_number1], diff);
		}

        pthread_mutex_unlock(args->mutex[random_number1]);
        pthread_mutex_unlock(args->mutex[random_number2]);
        
    }
    return NULL;

    
}

// Threads run on this function
void *decrease_increase(void *ptr)
{
	struct args *args = ptr;
	struct nums *n = args->nums;
    int random_number; 

	while(1) {
        //variable compartida: iterations_left
        pthread_mutex_lock(args->iterations_mutex);
        iterations_left--;
        if(iterations_left < 0){
            pthread_mutex_unlock(args->iterations_mutex);
            break;
        }

        

        pthread_mutex_unlock(args->iterations_mutex);
        
        random_number = rand() % array_size;

        //variable compartida: decrease[random_number] e increase[random_number]
        pthread_mutex_lock(args->mutex[random_number]);


		n->decrease[random_number]--;
		n->increase[random_number]++;
        

		long diff = n->total - (n->decrease[random_number] + n->increase[random_number]);
	
    	if (diff != n->diff) {
			n->diff = diff;
			printf("Thread %d in array position %d increasing %ld decreasing %ld diff %ld\n",
			       args->thread_num, random_number, n->increase[random_number], n->decrease[random_number], diff);
		}

        pthread_mutex_unlock(args->mutex[random_number]);
    }
    return NULL;
}

// start opt.num_threads threads running on decrease_incresase
struct thread_info *start_threads(struct options opt, struct nums *nums)
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads\n", opt.num_threads);
    //Alocamos memoria para as threads dinamicas
    threads = malloc(sizeof(struct thread_info) * opt.num_threads*2);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

      
    //Alocamos espacio para un mutex por cada unidad de tamaño do array
    pthread_mutex_t** mutex = malloc(sizeof(pthread_mutex_t*)* array_size);

    for(int i=0; i<array_size*2; i++){
        mutex[i] = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mutex[i], NULL);
        if(mutex[i] == NULL) {
            printf("Not enough memory for mutex\n");
            exit(1);
        }

        printf("\nInitialized mutex %d\n", i);
    }

    //Creamos un mutex para o numero de iteracions globais
    pthread_mutex_t* iterations_mutex = malloc(sizeof(pthread_mutex_t));

    if(iterations_mutex == NULL) {
        printf("Not enough memory for mutex\n");
        exit(1);
    }

    pthread_mutex_init(iterations_mutex, NULL);
    printf("\nInitialized iterations mutex \n");


    // Create num_thread threads running decrease_increase, and another num_thread threads running increase_shift
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->thread_num = i;
        threads[i].args->nums       = nums;
        threads[i].args->iterations = opt.iterations;
        threads[i].args->mutex      = mutex;
        threads[i].args->iterations_mutex = iterations_mutex;

        if (0 != pthread_create(&threads[i].id, NULL, decrease_increase, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }

    }

    for (i = opt.num_threads; i < opt.num_threads*2; i++) {
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->thread_num = i;
        threads[i].args->nums       = nums;
        threads[i].args->iterations = opt.iterations;
        threads[i].args->mutex      = mutex;
        threads[i].args->iterations_mutex = iterations_mutex;


        if (0 != pthread_create(&threads[i].id, NULL, increase_shift, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
        
    }

    printf("\n decrease_increase threads created \n");

    return threads;
}

/*
void print_totals(struct nums *nums)
{
	printf("Final: increasing %ld decreasing %ld diff %ld\n",
	       nums->increase, nums->decrease, nums->total - (nums->decrease + nums->increase));
}
*/

void print_totals(struct nums *nums)
{
    long sum_increasing=0, sum_decreasing=0;

    for(int i=0; i<array_size; i++){
	    printf("Final: increasing %ld decreasing %ld diff %ld\n",
	        nums->increase[i], nums->decrease[i], nums->total - (nums->decrease[i] + nums->increase[i]));

        sum_increasing += nums->increase[i];
        sum_decreasing += nums->decrease[i];

    }

    printf("Final: increasing %ld decreasing %ld diff %ld total %ld\n",
	        sum_increasing, sum_decreasing, nums->total - (sum_decreasing + sum_increasing), sum_decreasing + sum_increasing);


}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct nums *nums, struct thread_info *threads) {
    // Wait for the threads to finish
    for (int i = 0; i < opt.num_threads*2; i++){
        pthread_join(threads[i].id, NULL);
    }
        

    print_totals(nums);

    for(int i=0; i<array_size*2; i++){
        if(&threads[i].args->mutex[i] != NULL){

            pthread_mutex_destroy(threads[i].args->mutex[i]);
            printf("\nMutex for array position %d destroyed\n", i);
            
            free(threads[i].args->mutex[i]);
            printf("\nMemory for the mutex freed\n");

        }

    }
    

    if(&threads[0].args->iterations_mutex != NULL){
        pthread_mutex_destroy(threads[0].args->iterations_mutex);
        printf("\nIterations mutex destroyed\n");
        
        free(threads[0].args->iterations_mutex);
        printf("\nMemory for the iterations mutex freed\n");

    }
    
    
    for (int i = 0; i < opt.num_threads; i++)
        free(threads[i].args);

    free(threads);
    // (A memoria do stack liberaa o propio programa)

    free(nums->increase);
    free(nums->decrease);
}

int main (int argc, char **argv)
{
    //Generate seed for rng
    srand(time(NULL));

    struct options opt;
    struct nums nums;
    struct thread_info *thrs;


    // Default values for the options
    opt.num_threads  = 4;
    opt.iterations   = 100000;
    opt.size         = 10;

    read_options(argc, argv, &opt);

    nums.total = opt.iterations;
    nums.increase = malloc(sizeof(long) * opt.size);
    nums.decrease = malloc(sizeof(long) * opt.size);


    for(int i=0; i<opt.size; i++){
        nums.increase[i] = 0;
        nums.decrease[i] = nums.total;
    }

    nums.diff = 0;
    iterations_left= opt.iterations;
    array_size = opt.size;
    thrs = start_threads(opt, &nums);    
    wait(opt, &nums, thrs);

    return 0;
}