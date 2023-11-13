#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_TIPOS_JOBS 4
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
    FCFS,
    PS,
    SJF,
    RR
};

typedef struct job
{

    int id,
        prioridade,
        tempo_inicio,
        tempo_fim,
        tempo_espera,

        estado,
        burst_countdown,

        cpu_burst_length,
        io_burst_length,

        quant_countdown,

        reps;

} job_t;

typedef struct no
{
    int prioridade;
    job_t *job;
    struct no *proximo;
} no_t;

no_t *rtr_queue;
job_t **jobs;
job_t *ativo = NULL;

int num_jobs = 0;
int jobs_terminados = 0;

int tempo = 1;
int tempo_ocupado_cpu = 0;
int tempo_espera_cpu = 0;

const char *nome_alg[] = {"fcfs", "ps", "sjf", "rr"};
int alg_id = -1;

void carrega_jobs(FILE *f);
job_t *define_proximo_job();
void run();
void status();
void relatorio();

void push(no_t **head, job_t *j);
job_t *pop(no_t **);
int getpri(job_t *j);

/*-------------------------------------------------------------*/

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
        run();
        fprintf(f, "└───────────────────────────────┘");
    }
    else
    {
        printf("\"%s\" algoritmo invalido\n", nome_alg[alg_id]);
        return 0;
    }
}

/*-------------------------------------------------------------*/

void run()
{

    for (;;)
    {

        if (ativo)
        {

            --ativo->burst_countdown;
            --ativo->quant_countdown;

            // if CPU burst is done, switch to IO
            // and start running a new job.
            if (ativo->burst_countdown == 0)
            {
                ativo->estado = IO;
                ativo->burst_countdown = ativo->io_burst_length;
                ativo = define_proximo_job();
            }

            // else if the current job's quantum is expired, put it
            // at the end of the RTR queue and start the next one.
            else if (ativo->quant_countdown == 0)
            {
                ativo->estado = RTR;
                push(&rtr_queue, ativo);
                ativo = define_proximo_job();
            }

            tempo_ocupado_cpu++;
        }

        // if no job is using the CPU, try to pop a new one from the queue.
        // increment the idle time if there is nothing ready to run.
        else
        {
            if (!(ativo = define_proximo_job()))
            {
                tempo_espera_cpu++;
            }
        }

        // now look at the jobs that are not using the CPU.
        for (int i = 0; i < num_jobs; i++)
        {

            job_t *j = jobs[i];

            if (j->estado == IO)
            {
                // if IO burst is done, put back into RTR queue.
                if (j->burst_countdown-- == 0)
                {
                    if (j->reps == 0)
                    {
                        j->estado = TERMINADO;
                        j->tempo_fim = tempo;
                        jobs_terminados++;
                    }
                    else
                    {
                        j->estado = RTR;
                        push(&rtr_queue, j);
                    }
                }
            }

            if (j->estado == RTR)
            {
                ++j->tempo_espera;
            }
        }

        if (jobs_terminados < num_jobs)
            status();
        else
        {
            relatorio();
            break;
        }

        ++tempo;
    }

    for (int i = 0; i < num_jobs; i++)
    {
        free(jobs[i]);
    }

    if (rtr_queue)
    {
        free(rtr_queue);
    }
}

/*-------------------------------------------------------------*/

job_t *define_proximo_job()
{

    job_t *j = NULL;
    if ((j = pop(&rtr_queue)))
    {

        j->estado = CPU;

        if (!j->tempo_inicio)
        {
            j->tempo_inicio = tempo;
        }

        if (j->burst_countdown <= 0)
        {
            j->burst_countdown = j->cpu_burst_length;
            j->reps--;
        }

        j->quant_countdown = (alg_id == RR ? RR_TAMANHO_QUANTUM : -1);
    }
    return j;
}

/*-------------------------------------------------------------*/

void push(no_t **head, job_t *j)
{

    no_t *new = malloc(sizeof(no_t));
    new->job = j;
    new->proximo = NULL;
    new->prioridade = getpri(j);

    if (*head == NULL)
    {
        *head = new;
        return;
    }

    if (new->prioridade < (*head)->prioridade)
    {
        new->proximo = *head;
        *head = new;
        return;
    }

    no_t *atual = *head;

    while (atual->proximo && new->prioridade > atual->proximo->prioridade)
    {
        atual = atual->proximo;
    }

    new->proximo = atual->proximo;
    atual->proximo = new;
}

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

int getpri(job_t *j)
{

    int p = -1;

    if (alg_id == PS)
    {
        p = j->prioridade;
    }

    if (alg_id == SJF)
    {
        p = j->cpu_burst_length * j->reps;
    }

    if (alg_id == RR)
    {
        p = tempo;
    }

    return p;
}

/*-------------------------------------------------------------*/

void status()
{
    FILE *f = fopen("log.txt", "a+");
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
        fprintf(f, "   Tempo Final:   %12d\n", jobs[i]->tempo_fim);
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

        sscanf(linha, "%d %d %d %d %d",
               &j->id,
               &j->cpu_burst_length,
               &j->io_burst_length,
               &j->reps,
               &j->prioridade);

        push(&rtr_queue, j);
        ++num_jobs;
    }

    jobs = malloc(num_jobs * sizeof(job_t));

    no_t *atual = *&rtr_queue;

    int i = 0;
    while (atual)
    {
        jobs[i++] = atual->job;
        atual = atual->proximo;
    }
}