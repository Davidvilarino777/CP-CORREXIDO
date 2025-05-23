#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"
#include <pthread.h>

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0


struct thread_args{
    queue entrada; //cola de entrada in_q
    queue salida; //cola de salida out_q
};

struct thread_info{
    pthread_t id; //id de thread
    struct thread_args *args; //argumentos del thread
};
// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void worker(void *p) {
    struct thread_args *threads = p;
    queue in= threads->entrada;
    queue out=threads->salida;
    chunk ch, res;
    
    while(q_elements(in)>0) {
        
        ch = q_remove(in);
        //if (q_elements(in)==0)
           // q_terminada(in);
        res = zcompress(ch);
        free_chunk(ch);
        q_insert(out, res);
    }
}

struct thread_info *start_threads(struct options opt, queue entrada, queue salida)
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    for (i=0; i<opt.num_threads; i++){
        threads[i].args = malloc(sizeof(struct thread_args));

        threads[i].args->entrada = entrada;
        threads[i].args->salida = salida;

        if (0 != pthread_create(&threads[i].id, NULL, (void *)worker, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    
    return threads;
}

void wait (struct options opt, struct thread_info *threads)
{
    int i;
    
    for (i = 0; i < opt.num_threads; i++){
        pthread_join(threads[i].id,NULL);
    }
    for (i = 0; i < opt.num_threads; i++){
        free(threads[i].args);
    }
    free(threads);
}
// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks, i, offset;
    char comp_file[256];
    struct stat st;
    archive ar;
    queue in, out;
    chunk ch;
    struct thread_info *threads;

    if((fd=open(opt.file, O_RDONLY))==-1) {
        printf("Cannot open %s\n", opt.file);
        exit(0);
    }

    fstat(fd, &st);
    chunks = st.st_size/opt.size+(st.st_size % opt.size ? 1:0);

    if(opt.out_file) {
        strncpy(comp_file,opt.out_file,255);
    } else {
        strncpy(comp_file, opt.file, 255);
        strncat(comp_file, ".ch", 255);
    }

    ar = create_archive_file(comp_file);

    in  = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    // read input file and send chunks to the in queue
    for(i=0; i<chunks; i++) {
        ch = alloc_chunk(opt.size);

        offset=lseek(fd, 0, SEEK_CUR);

        ch->size   = read(fd, ch->data, opt.size);
        ch->num    = i;
        ch->offset = offset;

        q_insert(in, ch);
    }

    // compression of chunks from in to out
    threads = start_threads(opt,in,out);

    // send chunks to the output archive file
    for(i=0; i<chunks; i++) {
        ch = q_remove(out);

        add_chunk(ar, ch);
        free_chunk(ch);
    }

    wait(opt,threads);
    printf("\n");
    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);
}


// Decompress file taking chunks of size opt.size from the input file

void decomp(struct options opt) {
    int fd, i;
    char uncomp_file[256];
    archive ar;
    chunk ch, res;

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Cannot open archive file\n");
        exit(0);
    };

    if(opt.out_file) {
        strncpy(uncomp_file, opt.out_file, 255);
    } else {
        strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
        uncomp_file[strlen(opt.file)-3] = '\0';
    }

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Cannot create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    }

    for(i=0; i<chunks(ar); i++) {
        ch = get_chunk(ar, i);

        res = zdecompress(ch);
        free_chunk(ch);

        lseek(fd, res->offset, SEEK_SET);
        write(fd, res->data, res->size);
        free_chunk(res);
    }

    close_archive_file(ar);
    close(fd);
}

int main(int argc, char *argv[]) {
    struct options opt;

    opt.compress    = COMPRESS;
    opt.num_threads = 3;
    opt.size        = CHUNK_SIZE;
    opt.queue_size  = QUEUE_SIZE;
    opt.out_file    = NULL;

    read_options(argc, argv, &opt);

    if(opt.compress == COMPRESS) comp(opt);
    else decomp(opt);
}
