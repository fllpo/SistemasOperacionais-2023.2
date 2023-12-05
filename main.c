#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_TIPOS_JOBS 5
#define RR_TAMANHO_QUANTUM 10

enum estados
{
    RTR,
    CPU,
    IO,
    TERMINADO
};

enum algoritmos
{
    SJF,
    RR,
    NOSSO
};

typedef struct job
{

    int id,
        prioridade,
        tempo_inicio,
        tempo_fim,
        tempo_espera,

        estado,
        rajada_contagem,

        tamanho_rajada_cpu,
        tamanho_rajada_io,

        quantum_contagem,

        repeticoes;

} job_t;

typedef struct no
{
    int prioridade;
    job_t *job;
    struct no *proximo;
} no_t;

no_t *fila_rtr;
job_t **jobs;
job_t *ativo = NULL;

int num_jobs = 0;
int jobs_terminados = 0;

int tempo = 1;
int tempo_ocupado_cpu = 0;
int tempo_espera_cpu = 0;

const char *nome_alg[] = {"fcfs", "ps", "sjf", "rr", "nosso"};
int alg_id = -1;

void carrega_jobs(FILE *f);
job_t *define_proximo_job();
void executar();
void status();
void relatorio();

void push(no_t **head, job_t *j);
job_t *pop(no_t **);
int getprioridade(job_t *j);

int main(int argc, char *argv[])
{

    for (int i = 0; i < NUM_TIPOS_JOBS; i++)
    {
        if (!strcmp(argv[1], nome_alg[i]))
        {
            alg_id = i;
        }
    }

    FILE *f = fopen("processos.txt", "r");
    carrega_jobs(f);

    fclose(f);

    if (alg_id != -1)
    {
        executar();
        fprintf(f, "└───────────────────────────────┘");
    }
    else
    {
        printf("algoritmo invalido\n");
        return 0;
    }
}

void executar()
{

    while (1)
    {
        if (ativo)
        {

            ativo->rajada_contagem--;
            ativo->quantum_contagem--;

            // se a rajada de cpu terminou, troca para io e inicia novo job
            if (ativo->rajada_contagem == 0)
            {
                ativo->estado = IO;
                ativo->rajada_contagem = ativo->tamanho_rajada_io;
                ativo = define_proximo_job();
            }

            // senao se o quantum do job atual terminou, coloca no fim da fila e inicia o proximo
            else if (ativo->quantum_contagem == 0)
            {
                ativo->estado = RTR;
                push(&fila_rtr, ativo);
                ativo = define_proximo_job();
            }

            tempo_ocupado_cpu++;
        }

        // se nenhum job usa o cpu, define o proximo job e incrementa tempo de espera
        else
        {
            if (!(ativo = define_proximo_job()))
            {
                tempo_espera_cpu++;
            }
        }

        // jobs que nao estao usando cpu
        for (int i = 0; i < num_jobs; i++)
        {

            job_t *j = jobs[i];

            if (j->estado == IO)
            {
                // se a rajada io terminou, coloca na fila
                if (j->rajada_contagem-- == 0)
                {
                    if (j->repeticoes == 0)
                    {
                        j->estado = TERMINADO;
                        j->tempo_fim = tempo;
                        jobs_terminados++;
                    }
                    else
                    {
                        j->estado = RTR;
                        push(&fila_rtr, j);
                    }
                }
            }

            if (j->estado == RTR)
            {
                j->tempo_espera++;
            }
        }

        if (jobs_terminados < num_jobs)
            status();
        else
            break;

        tempo++;
    }

    relatorio();
    for (int i = 0; i < num_jobs; i++)
    {
        free(jobs[i]);
    }

    if (fila_rtr)
    {
        free(fila_rtr);
    }
}

job_t *define_proximo_job()
{

    job_t *j = NULL;
    if ((j = pop(&fila_rtr)))
    {

        j->estado = CPU;

        if (!j->tempo_inicio)
        {
            j->tempo_inicio = tempo;
        }

        if (j->rajada_contagem <= 0)
        {
            j->rajada_contagem = j->tamanho_rajada_cpu;
            j->repeticoes--;
        }

        j->quantum_contagem = (alg_id == RR ? RR_TAMANHO_QUANTUM : -1);
    }
    return j;
}

void push(no_t **head, job_t *j)
{

    no_t *novo = malloc(sizeof(no_t));
    novo->job = j;
    novo->proximo = NULL;
    novo->prioridade = getprioridade(j);

    if (*head == NULL)
    {
        *head = novo;
        return;
    }

    if (novo->prioridade < (*head)->prioridade)
    {
        novo->proximo = *head;
        *head = novo;
        return;
    }

    no_t *atual = *head;

    while (atual->proximo && novo->prioridade > atual->proximo->prioridade)
    {
        atual = atual->proximo;
    }

    novo->proximo = atual->proximo;
    atual->proximo = novo;
}

job_t *pop(no_t **head)
{

    no_t *atual = *head;

    if (!atual)
    {
        return NULL;
    }

    job_t *j = atual->job;
    *head = (*head)->proximo;
    free(atual);

    return j;
}

int getprioridade(job_t *j)
{

    int p = -1;

    if (alg_id == SJF)
    {
        p = j->tamanho_rajada_cpu * j->repeticoes;
    }

    if (alg_id == RR)
    {
        p = tempo;
    }

    if (alg_id == NOSSO)
    {
        p = (tempo * j->repeticoes);
    }

    return p;
}

void status()
{
    FILE *f = fopen("log.txt", "a");
    char *iostring = malloc(16 * sizeof(char));
    int c = 0;

    for (int i = 0; i < num_jobs; i++)
    {
        if (jobs[i]->estado == IO)
        {
            c += sprintf(&iostring[c], "%d ", jobs[i]->id);
        }
    }

    if (!c)
    {
        strcpy(iostring, "xx");
    }

    if (ativo)
        fprintf(f, "│  %4d %9d %9s     │\n", tempo, ativo->id, iostring);
    else
        fprintf(f, "│  %4d %9s %9s     │\n", tempo, "xx", iostring);

    free(iostring);
}

void relatorio()
{
    FILE *f = fopen("log.txt", "w");

    int turn_time = 0;

    for (int i = 0; i < num_jobs; i++)
    {

        fprintf(f, "   ID do Processo: %11d\n", jobs[i]->id);
        fprintf(f, "   Tempo de Início: %10d\n", jobs[i]->tempo_inicio);
        fprintf(f, "   Tempo Final:   %12d\n", jobs[i]->tempo_fim - 1);
        fprintf(f, "   Tempo em Espera:  %9d\n", jobs[i]->tempo_espera);

        turn_time += jobs[i]->tempo_fim;
        fprintf(f, "─────────────────────────────────\n");
    }

    fprintf(f, "   Turnaround Médio: %9d\n", turn_time / num_jobs);
    fprintf(f, "   Tempo Ocupado da CPU: %5d\n", tempo_ocupado_cpu);
    fprintf(f, "   Tempo de Espera da CPU: %3d\n\n", tempo_espera_cpu);

    fprintf(f, "┌───────────────────────────────┐\n");
    fprintf(f, "│   Tempo  :   CPU   :    IO    │\n");
    fprintf(f, "├───────────────────────────────┤\n");

    fclose(f);
}

void carrega_jobs(FILE *f)
{

    char linha[128];

    while (fgets(linha, 128, f))
    {

        job_t *j = calloc(1, sizeof(job_t));

        sscanf(linha, "%d %d %d %d",
               &j->id,
               &j->tamanho_rajada_cpu,
               &j->tamanho_rajada_io,
               &j->repeticoes);

        push(&fila_rtr, j);
        num_jobs++;
    }

    jobs = malloc(num_jobs * sizeof(job_t));

    no_t *atual = *&fila_rtr;

    int i = 0;
    while (atual)
    {
        jobs[i++] = atual->job;
        atual = atual->proximo;
    }
}
