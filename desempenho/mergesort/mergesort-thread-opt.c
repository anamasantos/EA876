
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
  int *vetor;
  int *scratch;
  int N;
}  thread_args, *Thread_args;


void troca (int *a, int *b) {
  int c;
  c = (*a);
  (*a) = (*b);
  (*b) = c;
#ifdef debugmode
  printf("Trocando %d e %d\n", *a, *b);
#endif
 }

void intercala(int *vetor, int *scratch, int N1, int M1, int N2, int M2) {
  int n1, n2, n3;
  n1 = 0;
  n2 = 0;
  n3 = 0;

#ifdef debugmode
  printf("Intercalando (simples)\n");
  for (int j = 0; j < N1; j++)
    printf("V1[%d] = %d\t", j, vetor[j+M1]);
  printf("\n");
  for (int j = 0; j < N2; j++)
    printf("V2[%d] = %d\t", j, vetor[j+M2]);
  printf("\n");
#endif


  /* Intercalar em area auxiliar */
  while ((n1 < N1) && (n2 < N2)) {
    if (vetor[M1+n1] < vetor[M2+n2]){

      scratch[M1+n3] = vetor[M1+n1];
      n1++;
      n3++;
    } else{
      scratch[M1+n3] = vetor[M2+n2];
      n2++;
      n3++;

      }
  }
  while (n1 < N1) {
    scratch[M1+n3] = vetor[M1+n1];
    n1++;
    n3++;
  }

  while (n2 < N2) {
    scratch[M1+n3] = vetor[M2+n2];
    n2++;
    n3++;
  }

  /* Retornar a vetores originais */
  n1=0;
  n2=0;
  n3=0;
  while (n1 < (M2+N2)) {
    vetor[n1] = scratch[n3];
    n1++;
    n3++;
  }

#ifdef debugmode
printf("Resultado: ");
for (int j=0; j<n3; j++)
  printf("[%d]=%d\t", j, scratch[j]);
printf("\n");
#endif

}

void intercalaK(int *vetor, int *scratch, const int *Ns, const int K) {

  int N_cumulativo[K];
  N_cumulativo[0] = Ns[0];
  for (int i = 1; i < K; i++) {
    N_cumulativo[i] = N_cumulativo[i-1] + Ns[i];
  }

#ifdef debugmode
  printf("Intercala K com K = %d (K/2 = %d)\n", K, K/2);
  for (int j = 0; j < K; j++)
    printf("Ns[%d] = %d - acc=%d\n", j, Ns[j], N_cumulativo[j]);
  for (int j = 0; j < N_cumulativo[K-1]; j++)
    printf("  Vetor[%d] = %d\n", j, vetor[j]);
#endif

  if (K <= 1)
    return;
  if  (K == 2) {
    intercala(vetor, scratch, Ns[0], 0, Ns[1], Ns[0] );
    return;
  }


  intercalaK(vetor, scratch, Ns, K/2);
  intercalaK(&(vetor[N_cumulativo[(K/2)-1]]), scratch, &(Ns[K/2]), K-(K/2));
  intercala(vetor, scratch, N_cumulativo[(K/2)-1], 0, N_cumulativo[K-1]-N_cumulativo[(K/2)-1], N_cumulativo[(K/2)-1]);

}



void mergesort(int *vetor, int *scratch, int N) {
  int M;

#ifdef debugmode
  printf("Mergesort com N=%d\n", N);
#endif
  if (N <= 1)
    return;

   if (N == 2) {
    if (vetor[1] < vetor[0])
      troca( &(vetor[0]), &(vetor[1]) );
    return;
  }

  M = N/2;

#ifdef debugmode
  printf("   Quebrando em %d e %d\n", M, N-M);
#endif

  mergesort(vetor, scratch, M);
  mergesort(&(vetor[M]), scratch, N-M);
  intercala(vetor, scratch, M, 0, N-M, M);
  return;
}

void* mergesort_worker(void *args) {
  thread_args* t = (thread_args*) args;
  mergesort(t->vetor, t->scratch, t->N);
}

void mergesort_threading(int *vetor, int N, const int n_threads) {
  pthread_t workers[n_threads];
  thread_args batch[n_threads];
  int Ns[n_threads];
  int thread_start;
  int thread_len;
  int *scratch = (int*) malloc(N*sizeof(int));

  thread_len = N / n_threads;

  for (int i = 0; i < n_threads; i++) {
    batch[i].N = thread_len;
    Ns[i] = thread_len;
    batch[i].vetor = &(vetor[i*thread_len]);

    batch[i].scratch=scratch;
  }
  batch[n_threads-1].N += N%n_threads;
  Ns[n_threads-1] = batch[n_threads-1].N;

  #ifdef debugmode
  int n=0;
  for (int i=0; i<n_threads; i++) {

    printf("Threads %d: %d elementos (atual=%d)\n", i, batch[i].N, batch[i].vetor[0]);
    n += batch[i].N;
  }
  printf("Total: %d elementos\n", n);
  #endif

  /* Dispara threads */
  for (int i = 0; i < (n_threads); i++) {
    pthread_create(&(workers[i]), NULL, mergesort_worker, &(batch[i]));
  }

  /* Thread join */
  for (int i = 0; i < n_threads; i++) {
    pthread_join(workers[i], NULL);
  }

  /* Merge em elementos */
  intercalaK(batch[0].vetor, scratch, Ns, n_threads);
  free(scratch);
}

int  main(int argc, char **argv) {
  clock_t start, end;

  int tam_vetor = atoi(argv[1]);
  int n_threads = atoi(argv[2]);

  int *a = (int*) malloc(sizeof(int) * tam_vetor);
  for (int i = 0; i< tam_vetor; i++) {
    a[i] = tam_vetor-i;
  }

  start = clock();
  mergesort_threading(a, tam_vetor, n_threads);
  end = clock();

#ifdef debugmode
  for  (int i=0; i<tam_vetor; i++) {
    printf("[%d] = %d\t", i, a[i]);
  }
#endif

  printf("Mergesort-thread levou %f segundos para n=%d e %d threads\n", (double)(end-start)/CLOCKS_PER_SEC, tam_vetor, n_threads);

  free(a);

  return 0;

}
