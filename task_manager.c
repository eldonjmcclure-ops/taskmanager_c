#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct task {
	int id;
    char name[33];
    char desc[101];
    bool isdone;
    int timesneeded;
    int timesdone;
    char datedue[11];
    struct task *next;
};

void createTask (struct task * task) {
    printf("\nEnter task ID: ");
    scanf("%d", &task->id);
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF) {}
	printf("\nEnter task name: ");
	if (fgets(task->name, sizeof task->name, stdin) == NULL) return;
	task->name[strcspn(task->name, "\n")] = '\0';
	printf("\nEnter task description (<100char): ");
	if (fgets(task->desc, sizeof task->desc, stdin) == NULL) return;
	task->desc[strcspn(task->desc, "\n")] = '\0';
    printf("\nEnter instance count needed for completion (int): ");
    scanf("%d", &task->timesneeded);
    printf("\nEnter a due date in mm/dd/yyyy format: ");
    scanf("%10s", &task->datedue);
    task->timesdone = 0;
    task->isdone = 0;
    task->next = NULL;
};

void completeTask (struct task * task) {
    task->timesdone + 1;
    if (task->timesdone == task->timesneeded) {
        task->isdone = 1;
    }

};

void printTaskDetails (struct task * task) {
    printf("\nTask: %s", task->name);
    printf("\nDescription: %s", task->desc);
    printf("\nCompletion: %d/%d", task->timesdone, task->timesneeded);
	printf("\nTask Due By: %s", task->datedue);
    if (task->isdone == true) {
        printf("\n--Task Completed--");
    }
    else {
        printf("\n--Task Incomplete--");
    }
};

void displayTasks (struct task *a) {
	while (a != NULL)
		{
			printTaskDetails(a);
			a = a->next;
		}
};

void printMenu(struct task *head) {
	int choice;
	do {
		printf("\nMain Menu\n");
		printf("   1. Add new task\n");
		printf("   2. Display all tasks\n");
		printf("   3. Find task\n");
		printf("   4. Complete task\n");
		printf("   0. Log out\n");
		printf("Your Choice: ");
		scanf("%d", &choice);

		//Add task
		if (choice == 1) {
			struct task *new_task = malloc(sizeof(struct task));
			createTask(new_task);
			new_task->next = NULL;
			if (head == NULL){
				head = new_task;
			}
			else {
				struct task *tail;
				tail = head;
				while (tail->next != NULL) {
					tail = tail->next;
				}
				tail->next = new_task;
			}
        }
		// Display tasks
		else if (choice == 2) {

			if (head == NULL) {
				printf("Task list is empty.\n");
			}
			else {
				displayTasks(head);
			}
		}
		// Find Task
		else if (choice == 3) {
			int search_num;
			printf("Enter the task number to find: ");
			scanf("%d", &search_num);
			struct task *tail;
			tail = head;
			while (tail->id != search_num) {
				tail = tail->next;
				if (tail->next == NULL) {
					break;
				}
			}
			if (tail->id == search_num) {
				printf("\nTask found: ");
				printTaskDetails(tail);
			}
			else {
				printf("\nNo task found with that number.");
			}

		}
		// Update Task
		else if (choice == 4) {
			int search_num;
			printf("Enter the task number to update: ");
			scanf("%d", &search_num);
			struct task *tail;
			tail = head;
			while (tail->id != search_num) {
				tail = tail->next;
			}
			if (tail->id == search_num) {
				printf("\nTask found:\n");
				printTaskDetails(tail);
				printf("\nEnter updated task information: ");
				createTask(tail);
				printf("Updated information:");
				printTaskDetails(tail);
			}
			else {
				printf("No task found with that number.\n");
			}
		}
		// Log Out
		else if (choice == 0) {
			printf("Logging out...\n");
		}
		else {
			printf("Choose an available option.\n");
		}
	} while (choice != 0);
};

int main(void) {

    struct task * head;
    head = NULL;
	printMenu(head);

    return EXIT_SUCCESS;
}
