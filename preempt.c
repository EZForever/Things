//gcc $ME -o $ME.out && ./$ME.out
//gcc -g $ME -o $ME.out && gdb ./$ME.out

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <ucontext.h>

#define STACKSZ (4 * 1024)
#define TIMESLICE 500

ucontext_t *contexts = NULL;
ucontext_t *curr_context = NULL;
ucontext_t main_context;

void append_context(void (*entry)(void)) {
	ucontext_t *new_context = (ucontext_t *)malloc(sizeof(ucontext_t));
	getcontext(new_context);
	
	new_context->uc_link = contexts;
	
	new_context->uc_stack.ss_sp = malloc(STACKSZ);
	new_context->uc_stack.ss_size = STACKSZ;
	new_context->uc_stack.ss_flags = 0;
	
	makecontext(new_context, entry, 0);
	
	contexts = new_context;
}

void destroy_context(void) {
	ucontext_t *todel_context = contexts;
	contexts = contexts->uc_link;
	
	free(todel_context->uc_stack.ss_sp);
	free(todel_context);
}

void isr_sigalrm(int sig) {
	ucontext_t *prev_context = curr_context;
	if(!prev_context)
		prev_context = contexts;
	
	ucontext_t *next_context = prev_context->uc_link;
	if(!next_context)
		next_context = contexts;
	
	//ualarm(TIMESLICE, 0);
	
	curr_context = next_context;
	swapcontext(prev_context, next_context);
}

void isr_sigint(int sig) {
	ualarm(0, 0);
	
	setcontext(&main_context);
}

void thread_a(void) {
	while(1)
		fputc('A', stderr);
}

void thread_b(void) {
	while(1)
		fputc('B', stderr);
}

int main() {
	append_context(thread_a);
	append_context(thread_b);
	
	signal(SIGALRM, isr_sigalrm);
	signal(SIGINT, isr_sigint);
	
	//ualarm(TIMESLICE, 0);
	ualarm(TIMESLICE, TIMESLICE);
	
	swapcontext(&main_context, contexts);
	
	while(contexts)
		destroy_context();
	
	getchar();
	
	return 0;
}

