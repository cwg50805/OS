#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define GET_TIME 333
#define PRINTK 334

#define CHILD_CPU	1
#define PARENT_CPU	0

#define FIFO	1
#define RR		2
#define SJF		3
#define PSJF	4

/* Last context switch time for RR scheduling */
static int t_last;

/* Current unit time */
static int ntime;

/* Index of running process. -1 if no process running */
static int running;

/* Number of finish Process */
static int finish_cnt;

/* Running one unit time */
void UNIT_T()				
{						
	volatile unsigned long i;		
	for (i = 0; i < 1000000UL; i++);
}						

struct process {
	char name[32];
	int t_ready;
	int t_exec;
	pid_t pid;
	long t_start;
};

int cmp(const void *a, const void *b)
{
	return ((struct process *)a)->t_ready - ((struct process *)b)->t_ready;
}


int fifo(struct process *proc, int nproc, int policy)
{
	/* Non-preemptive */
	if (running != -1 )
		return running;

	int ret = -1;

	for(int i = 0; i < nproc; i++) {
			if(proc[i].pid == -1 || proc[i].t_exec == 0)
				continue;
			if(ret == -1 || proc[i].t_ready < proc[ret].t_ready)
				ret = i;
	}
	return ret;
}

int rr(struct process *proc, int nproc, int policy)
{
	int ret = -1;

	if (running == -1) {
		for (int i = 0; i < nproc; i++) {
			if (proc[i].pid != -1 && proc[i].t_exec > 0){
				ret = i;
				break;
			}
		}
	}
	else if ((ntime - t_last) % 500 == 0)  {
		ret = (running + 1) % nproc;
		while (proc[ret].pid == -1 || proc[ret].t_exec == 0)
			ret = (ret + 1) % nproc;
	}
	else
		ret = running;

	return ret;
}

int sjf(struct process *proc, int nproc, int policy)
{
	/* Non-preemptive */
	if (running != -1 )
		return running;

	int ret = -1;

	for (int i = 0; i < nproc; i++) {
			if (proc[i].pid == -1 || proc[i].t_exec == 0)
				continue;
			if (ret == -1 || proc[i].t_exec < proc[ret].t_exec)
				ret = i;
	}
	return ret;
}

int psjf(struct process *proc, int nproc, int policy)
{
	int ret = -1;

	for (int i = 0; i < nproc; i++) {
			if (proc[i].pid == -1 || proc[i].t_exec == 0)
				continue;
			if (ret == -1 || proc[i].t_exec < proc[ret].t_exec)
				ret = i;
	}
	return ret;
}

int proc_assign_cpu(int pid, int core)
{
	if (core > sizeof(cpu_set_t)) {
		fprintf(stderr, "Core index error.");
		return -1;
	}

	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
		
	if (sched_setaffinity(pid, sizeof(mask), &mask) < 0) {
		perror("sched_setaffinity");
		exit(1);
	}

	return 0;
}

int proc_wakeup(int pid)
{
	struct sched_param param;
	
	param.sched_priority = 99;

	int ret = sched_setscheduler(pid, SCHED_FIFO, &param);
	
	if (ret < 0) {
		perror("sched_setscheduler");
		return -1;
	}

	return ret;
}

int proc_block(int pid)
{
	struct sched_param param;

	param.sched_priority = 1;

	int ret = sched_setscheduler(pid, SCHED_FIFO, &param);
	
	if (ret < 0) {
		perror("sched_setscheduler");
		return -1;
	}

	return ret;
}

int proc_exec(struct process proc)
{
	int pid = fork();

	if (pid < 0) {
		perror("fork");
		return -1;
	}
	if (pid > 0){
		/* Assign child to another core prevent from interupted by parant */
		proc_assign_cpu(pid, CHILD_CPU);
		proc_block(pid);
		return pid;
	}
	
	if (pid == 0) {
		for (int i = 0; i < proc.t_exec; i++) {
			UNIT_T();
		}
		exit(0);
	}
	return pid;
}


int scheduling(struct process *proc, int nproc, int policy)
{
	/* Sort processes by ready time */
	qsort(proc, nproc, sizeof(struct process), cmp);

	/* Initial pid = -1 imply not ready */
	for (int i = 0; i < nproc; i++)
		proc[i].pid = -1;

	/* Set single core prevent from preemption */
	proc_assign_cpu(getpid(), PARENT_CPU);
	
	/* Set high priority to scheduler */
	proc_wakeup(getpid());

	/* Initial scheduler */
	ntime = 0;
	running = -1;
	finish_cnt = 0;
	
	while(1) {
		if (running != -1 && proc[running].t_exec == 0) {

			long start_sec,  end_sec;
			start_sec = proc[running].t_start;
			waitpid(proc[running].pid, NULL, 0);
			end_sec = syscall(GET_TIME);
			syscall(PRINTK, proc[running].pid, start_sec, end_sec);
			printf("%s %d\n", proc[running].name, proc[running].pid);
			fflush(stdout);
			running = -1;
			finish_cnt++;

			if (finish_cnt == nproc)
				break;
		}

		/* Check if process ready and execute */
		for (int i = 0; i < nproc; i++) {
			if (proc[i].t_ready == ntime) {
				proc[i].pid = proc_exec(proc[i]);
			}
		}

		int next;
		/* Select next running  process */
		switch (policy)
		{
		case 1:
			next = fifo(proc, nproc, policy);
			break;
		case 2:
			next = rr(proc, nproc, policy);
			break;
		case 3:
			next = sjf(proc, nproc, policy);
			break;
		case 4:
			next = psjf(proc, nproc, policy);
			break;
		default:
			break;
		}

		if (next != -1) {
			if (running != next) {
				if(proc[next].t_start == -1) proc[next].t_start = syscall(GET_TIME);
				proc_wakeup(proc[next].pid);
				proc_block(proc[running].pid);
				running = next;
				t_last = ntime;
			}
		}
		/* Run an unit of time */
		UNIT_T();
		if (running != -1)
			proc[running].t_exec--;
		ntime++;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	char sched_policy[256];
	int policy;
	int nproc;
	struct process *proc;
	scanf("%s%d", sched_policy, &nproc);
	
	proc = (struct process *)malloc(nproc * sizeof(struct process));

	for (int i = 0; i < nproc; i++) {
		scanf("%s%d%d", proc[i].name,
			&proc[i].t_ready, &proc[i].t_exec);
		proc[i].t_start = -1;
	}

	
	if (strcmp(sched_policy, "FIFO") == 0) {
		policy = FIFO;
	}
	else if (strcmp(sched_policy, "RR") == 0) {
		policy = RR;
	}
	else if (strcmp(sched_policy, "SJF") == 0) {
		policy = SJF;
	}
	else if (strcmp(sched_policy, "PSJF") == 0) {
		policy = PSJF;
	}
	else {
		fprintf(stderr, "Invalid policy: %s", sched_policy);
		exit(0);
	}

	scheduling(proc, nproc, policy);
	exit(0);
}

