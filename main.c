#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct job
{
    int id,
        prioridade,
        tempo_início,
        tempo_fim,
        tempo_espera,
        tempo_cpu,
        tempo_io,
        estado;

} job_t;

typedef struct nó
{
    int prioridade;
    job_t *job;
    struct nó *próximo;
} nó_t;

int num_jobs = 0, jobs_terminados = 0;

job_t **jobs, *ativo = NULL;

void carrega_jobs(FILE *a)
{
    char linha[128];

    for (int i = 1; fgets(linha, 128, a) != NULL; i++)
    {
        job_t *j = calloc(1, sizeof(job_t));
        j->id = i;

        sscanf(linha, "%d", &j->prioridade);
        printf("PID: %d | Prioridade: %d | \n", j->id, j->prioridade);
    }
    jobs = malloc(num_jobs * sizeof(job_t));
}

void push(nó_t **cabeça, job_t *j)
{

    nó_t *novo = malloc(sizeof(nó_t));
    novo->job = j;
    novo->próximo = NULL;
    novo->prioridade = j->prioridade;

    if (*cabeça == NULL)
    {
        *cabeça = novo;
        return;
    }

    if (novo->prioridade < (*cabeça)->prioridade)
    {
        novo->próximo = *cabeça;
        *cabeça = novo;
        return;
    }

    nó_t *atual = *cabeça;

    while (atual->próximo && novo->prioridade > atual->próximo->prioridade)
    {
        atual = atual->próximo;
    }

    novo->próximo = atual->próximo;
    atual->próximo = novo;
}

job_t *pop(nó_t **cabeça)
{

    nó_t *atual = *cabeça;

    if (!atual)
    {
        return NULL;
    }

    job_t *j = atual->job;
    *cabeça = (*cabeça)->próximo;
    free(atual);

    return j;
}

int main()
{
    FILE *a = fopen("processos.txt", "r");

    carrega_jobs(a);
    fclose(a);

    return 0;
}
