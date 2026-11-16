#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_SIZE 33
#define DESC_SIZE 101
#define DATE_SIZE 11
#define FILE_NAME_SIZE 260
#define LINE_SIZE 512

struct task {
    int id;
    char name[NAME_SIZE];
    char desc[DESC_SIZE];
    bool isdone;
    int timesneeded;
    int timesdone;
    char datedue[DATE_SIZE];
    struct task *next;
};

/* Option A: head and tail are passed by address so this function can update them. */
static void appendTask(struct task **head, struct task **tail, struct task *task) {
    task->next = NULL;
    if (*head == NULL) *head = task;
    else (*tail)->next = task;
    *tail = task;
}

/* Returns 1 for success, 0 for EOF, and -1 when the input was too long. */
static int inputLine(const char *prompt, char *buffer, size_t size) {
    int nextChar;
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    if (strchr(buffer, '\n') == NULL) {
        /* A full-size value may have left only the newline in stdin. */
        nextChar = getchar();
        if (nextChar != '\n' && nextChar != EOF) {
            while ((nextChar = getchar()) != '\n' && nextChar != EOF) { }
            printf("Error: input is too long. Maximum length is %zu characters. Please try again.\n",
                   size - 1);
            return -1;
        }
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

static bool inputInt(const char *prompt, int *value) {
    char buffer[64], *end;
    long number;
    do {
        int result = inputLine(prompt, buffer, sizeof buffer);
        if (result == 0) return false;
        if (result < 0) continue;
        errno = 0;
        number = strtol(buffer, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (buffer[0] != '\0' && *end == '\0' && errno != ERANGE &&
            number >= INT_MIN && number <= INT_MAX) {
            *value = (int)number;
            return true;
        }
        printf("Please enter a valid integer.\n");
    } while (true);
}

static bool inputText(const char *prompt, char *value, size_t size) {
    for (;;) {
        int result = inputLine(prompt, value, size);
        if (result == 0) return false;
        if (result < 0) continue;
        if (strchr(value, '|') != NULL) {
            printf("Error: the character '|' is reserved as a field separator. Please try again.\n");
            continue;
        }
        return true;
    }
}

static bool positiveInt(const char *prompt, int *value) {
    if (!inputInt(prompt, value)) return false;
    if (*value <= 0) {
        printf("Please enter a value greater than zero.\n");
        return false;
    }
    return true;
}

static bool nonnegativeInt(const char *prompt, int *value) {
    if (!inputInt(prompt, value)) return false;
    if (*value < 0) {
        printf("Please enter zero or greater.\n");
        return false;
    }
    return true;
}

static void trim(char *text) {
    char *start = text, *end;
    while (isspace((unsigned char)*start)) start++;
    if (start != text) memmove(text, start, strlen(start) + 1);
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
}

static bool parseInt(const char *text, int *value) {
    char *end;
    long number;
    errno = 0;
    number = strtol(text, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (*text == '\0' || *end != '\0' || errno == ERANGE ||
        number < INT_MIN || number > INT_MAX) return false;
    *value = (int)number;
    return true;
}

static bool copyField(char *destination, size_t size, const char *source) {
    if (strlen(source) >= size) return false;
    strcpy(destination, source);
    return true;
}

static bool parseTask(char *line, struct task *task) {
    char *fields[7], *field;
    int count = 0, isdone;
    line[strcspn(line, "\r\n")] = '\0';
    trim(line);
    if (*line == '\0') return false;
    field = strtok(line, "|");
    while (field != NULL && count < 7) {
        trim(field);
        fields[count++] = field;
        field = strtok(NULL, "|");
    }
    if (field != NULL || count != 7 || !parseInt(fields[0], &task->id) ||
        !copyField(task->name, sizeof task->name, fields[1]) ||
        !copyField(task->desc, sizeof task->desc, fields[2]) ||
        !parseInt(fields[3], &isdone) ||
        !parseInt(fields[4], &task->timesneeded) ||
        !parseInt(fields[5], &task->timesdone) ||
        !copyField(task->datedue, sizeof task->datedue, fields[6])) return false;
    if ((isdone != 0 && isdone != 1) || task->id < 1 ||
        task->timesneeded < 1 || task->timesdone < 0 ||
        task->timesdone > task->timesneeded) return false;
    task->isdone = isdone != 0;
    task->next = NULL;
    return true;
}

static bool readFromFile(const char *filename, struct task **head,
                         struct task **tail, int *highestId) {
    FILE *file = fopen(filename, "r");
    char line[LINE_SIZE];
    int lineNumber = 0;
    if (file == NULL) {
        if (errno == ENOENT) return true;
        perror("Could not open task file for reading");
        return false;
    }
    while (fgets(line, sizeof line, file) != NULL) {
        struct task *task;
        lineNumber++;
        task = malloc(sizeof *task);
        if (task == NULL) {
            fprintf(stderr, "Out of memory while loading tasks.\n");
            fclose(file);
            return false;
        }
        if (!parseTask(line, task)) {
            fprintf(stderr, "Skipping invalid task record on line %d.\n", lineNumber);
            free(task);
            continue;
        }
        appendTask(head, tail, task);
        if (task->id > *highestId) *highestId = task->id;
    }
    if (ferror(file)) {
        perror("Error while reading task file");
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
}

static bool writeToFile(const char *filename, const struct task *head) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Could not open task file for writing");
        return false;
    }
    while (head != NULL) {
        if (fprintf(file, "%d | %s | %s | %d | %d | %d | %s\n",
                    head->id, head->name, head->desc, head->isdone ? 1 : 0,
                    head->timesneeded, head->timesdone, head->datedue) < 0) {
            perror("Error while writing task file");
            fclose(file);
            return false;
        }
        head = head->next;
    }
    if (fclose(file) != 0) {
        perror("Error while closing task file");
        return false;
    }
    return true;
}

static bool createTask(struct task *task, int id) {
    task->id = id;
    printf("\nCreating task #%d\n", id);
    if (!inputText("Enter task name: ", task->name, sizeof task->name) ||
        !inputText("Enter task description (<100 characters): ", task->desc,
                   sizeof task->desc) ||
        !positiveInt("Enter instance count needed for completion: ",
                     &task->timesneeded) ||
        !inputText("Enter a due date in mm/dd/yyyy format: ", task->datedue,
                   sizeof task->datedue)) return false;
    task->timesdone = 0;
    task->isdone = false;
    task->next = NULL;
    return true;
}

static void printTaskDetails(const struct task *task) {
    printf("\nTask #%d: %s\nDescription: %s\nCompletion: %d/%d\nTask Due By: %s\n",
           task->id, task->name, task->desc, task->timesdone,
           task->timesneeded, task->datedue);
    printf("--Task %s--\n", task->isdone ? "Completed" : "Incomplete");
}

static void displayTasks(const struct task *head) {
    while (head != NULL) {
        printTaskDetails(head);
        head = head->next;
    }
}

static struct task *findTask(struct task *head, int id) {
    while (head != NULL && head->id != id) head = head->next;
    return head;
}

static void completeTask(struct task *task) {
    if (task->timesdone < task->timesneeded) task->timesdone++;
    if (task->timesdone >= task->timesneeded) task->isdone = true;
}

static bool updateTask(struct task *task) {
    char name[NAME_SIZE], desc[DESC_SIZE], date[DATE_SIZE];
    int needed, done;
    printf("\nUpdating task #%d.\n", task->id);
    if (!inputText("Enter task name: ", name, sizeof name) ||
        !inputText("Enter task description (<100 characters): ", desc,
                   sizeof desc) ||
        !positiveInt("Enter instance count needed for completion: ", &needed) ||
        !nonnegativeInt("Enter times completed: ", &done) ||
        !inputText("Enter a due date in mm/dd/yyyy format: ", date,
                   sizeof date)) return false;
    if (done > needed) {
        printf("Times completed cannot exceed times needed. Update canceled.\n");
        return false;
    }
    strcpy(task->name, name);
    strcpy(task->desc, desc);
    strcpy(task->datedue, date);
    task->timesneeded = needed;
    task->timesdone = done;
    task->isdone = done >= needed;
    return true;
}

static void freeTasks(struct task **head, struct task **tail) {
    while (*head != NULL) {
        struct task *old = *head;
        *head = old->next;
        free(old);
    }
    *tail = NULL;
}

static void menu(struct task **head, struct task **tail, const char *filename,
                 int *highestId) {
    int choice;
    do {
        printf("\nMain Menu\n1. Add new task\n2. Display all tasks\n3. Find task\n4. Complete task\n5. Update task\n0. Log out\n");
        if (!inputInt("Your choice: ", &choice)) return;
        if (choice == 1) {
            struct task *task = malloc(sizeof *task);
            if (task == NULL) {
                fprintf(stderr, "Unable to allocate memory for the new task.\n");
                continue;
            }
            if (createTask(task, *highestId + 1)) {
                (*highestId)++;
                appendTask(head, tail, task);
                if (!writeToFile(filename, *head))
                    printf("Warning: task added in memory but not saved.\n");
            } else {
                free(task);
                printf("Task creation canceled.\n");
            }
        } else if (choice == 2) {
            if (*head == NULL) printf("Task list is empty.\n");
            else displayTasks(*head);
        } else if (choice == 3 || choice == 4 || choice == 5) {
            int id;
            const char *prompt = choice == 3 ? "Enter the task number to find: " :
                                 choice == 4 ? "Enter the task number to complete: " :
                                               "Enter the task number to update: ";
            if (!inputInt(prompt, &id)) return;
            struct task *task = findTask(*head, id);
            if (task == NULL) {
                printf("No task found with that number.\n");
            } else if (choice == 3) {
                printTaskDetails(task);
            } else if (choice == 4) {
                completeTask(task);
                if (!writeToFile(filename, *head))
                    printf("Warning: task completed in memory but not saved.\n");
                printTaskDetails(task);
            } else if (updateTask(task)) {
                if (!writeToFile(filename, *head))
                    printf("Warning: task updated in memory but not saved.\n");
                printTaskDetails(task);
            }
        } else if (choice == 0) {
            printf("Saving tasks and logging out...\n");
            if (!writeToFile(filename, *head)) printf("Warning: tasks could not be saved.\n");
        } else {
            printf("Choose an available option.\n");
        }
    } while (choice != 0);
}

int main(void) {
    struct task *head = NULL, *tail = NULL;
    char filename[FILE_NAME_SIZE];
    int highestId = 0;
    for (;;) {
        int result = inputLine("Enter the task filename: ", filename, sizeof filename);
        if (result == 0) {
            fprintf(stderr, "A task filename is required.\n");
            return EXIT_FAILURE;
        }
        if (result < 0) continue;
        if (filename[0] == '\0') {
            printf("Error: a task filename is required. Please try again.\n");
            continue;
        }
        break;
    }
    if (!readFromFile(filename, &head, &tail, &highestId)) {
        freeTasks(&head, &tail);
        return EXIT_FAILURE;
    }
    menu(&head, &tail, filename, &highestId);
    freeTasks(&head, &tail);
    return EXIT_SUCCESS;
}
