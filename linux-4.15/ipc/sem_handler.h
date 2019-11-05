#ifndef SEM_HANDLER_H
#define SEM_HANDLER_H

#include <linux/sem.h>

typedef struct semarray_object {
	struct sem_array imported_sem;
	struct sem_array *local_sem;
	struct sem* mobile_sem_base;
} semarray_object_t;

int share_existing_semundo_proc_list(struct task_struct *tsk,
				     unique_id_t undo_list_id);
int create_semundo_proc_list(struct task_struct *tsk);


struct semundo_list_object;
int add_semundo_to_proc_list(struct semundo_list_object *undo_list, int semid);

void sem_handler_init(void);

#endif // SEM_HANDLER_H
