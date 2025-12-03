// Custom Multi-Profile Linux Shell
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define MAXCOM 100000  // max number of letters to be supported
#define MAXLIST 100000 // max number of commands to be supported

// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

// Colors
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

const char *HISTORY_FILE = ".custom_shell_history";

// Forward declarations
int execArgs(char **parsed);
int execArgsPiped(char **parsed, char **parsedpipe);
int processString(char *str, char **parsed, char **parsedpipe, int profile);
int coreShell(char **parsed);
int opsShell(char **parsed);
int dataShell(char **parsed);
int netShell(char **parsed);
int secShell(char **parsed);
void createFiles(void);

// Helper: trim whitespace in place
char *trimWhitespace(char *str) {
    if (str == NULL) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Greeting shell during startup
void init_shell() {
    clear();
    printf(COLOR_CYAN "\n\n  ===============================\n" COLOR_RESET);
    printf(COLOR_CYAN "      Custom Multi-Profile Shell\n" COLOR_RESET);
    printf(COLOR_CYAN "  ===============================\n" COLOR_RESET);
    char *username = getenv("USER");
    if (username) {
        printf("User: %s\n", username);
    }

    printf("Initializing shell");
    fflush(stdout);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        usleep(300000); // 0.3s
    }
    printf("\n");
    sleep(1);
    clear();
}

int isLinuxCommand(char *cmd) {
    char command[100];
    snprintf(command, sizeof(command), "which %s > /dev/null 2>&1", cmd);
    int status = system(command);
    return (status == 0);
}

// Function to take input
int takeInput(char *str) {
    char *buf;
    buf = readline(" ");
    if (buf && strlen(buf) != 0) {
        add_history(buf);

        // Save to history file
        FILE *hf = fopen(HISTORY_FILE, "a");
        if (hf != NULL) {
            fprintf(hf, "%s\n", buf);
            fclose(hf);
        }

        strcpy(str, buf);
        free(buf);
        return 0;
    } else {
        if (buf) free(buf);
        return 1;
    }
}

// Function to print Current Directory.
void printDir() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s", cwd);
}

// Generic unknown command error
void displayError() {
    printf("Unknown command. Type 'help' to see the list of commands.\n");
}

// History display
void showHistory() {
    FILE *hf = fopen(HISTORY_FILE, "r");
    if (!hf) {
        printf("No history available.\n");
        return;
    }
    char line[1024];
    int count = 1;
    while (fgets(line, sizeof(line), hf)) {
        printf("%4d  %s", count++, line);
    }
    fclose(hf);
}

// Common built-ins: cd, history, exit handled in profiles individually for custom messages.
// Here we only implement cd & history logic.
int handleCommonBuiltins(char **parsed) {
    if (parsed[0] == NULL) return 0;

    if (strcmp(parsed[0], "cd") == 0) {
        if (parsed[1] == NULL) {
            char *home = getenv("HOME");
            if (home == NULL) home = "/";
            if (chdir(home) != 0) {
                perror("cd");
            }
        } else {
            if (chdir(parsed[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(parsed[0], "history") == 0) {
        showHistory();
        return 1;
    }

    return 0;
}

// Function where a simple system command is executed
int execArgs(char **parsed) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            perror("execvp");
            exit(1);
        }
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else
            return 1;
    }
}

// Function where the piped system commands are executed
int execArgsPiped(char **parsed, char **parsedpipe) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        perror("pipe");
        return 1;
    }
    p1 = fork();
    if (p1 < 0) {
        perror("fork");
        return 1;
    }

    if (p1 == 0) {
        // Child 1
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            perror("execvp");
            exit(1);
        }
    } else {
        p2 = fork();
        if (p2 < 0) {
            perror("fork");
            return 1;
        }

        if (p2 == 0) {
            // Child 2
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                perror("execvp");
                exit(1);
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
            int status1, status2;
            waitpid(p1, &status1, 0);
            waitpid(p2, &status2, 0);
            if (WIFEXITED(status2))
                return WEXITSTATUS(status2);
            else
                return 1;
        }
    }
    return 0;
}

// ================= Profile-specific commands =================

// File helpers from original logic, kept but theme-neutral

void createCorruptedFilesDirectory() {
    char *dirName = "corrupted_files";
    char *fileName = "corrupted_files/temp_corrupt.txt";

    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }

    FILE *file = fopen(fileName, "w");
    if (file != NULL) {
        fclose(file);
    }
}

void createBackupDirectory() {
    char *dirName = "backup_good_files";

    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }
}

void createGoodFilesDirectory() {
    char *dirName = "good_files";
    char *fileName = "good_files/base.txt";

    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }

    FILE *file = fopen(fileName, "w");
    if (file != NULL) {
        fprintf(file, "Base data file\n");
        fclose(file);
    }
}

void createHiddenFilesDirectory() {
    char *dirName = "hidden";
    char *fileName = "hidden/temp_hidden.txt";

    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }

    FILE *file = fopen(fileName, "w");
    if (file != NULL) {
        fprintf(file, "Hidden temp file\n");
        fclose(file);
    }
}

void createMainDirectory() {
    char *dirName = "main";
    char *fileName = "main/temp_main.txt";

    struct stat st = {0};
    if (stat(dirName, &st) == -1) {
        mkdir(dirName, 0700);
    }

    FILE *file = fopen(fileName, "w");
    if (file != NULL) {
        fprintf(file, "Main temp file\n");
        fclose(file);
    }
}

void createTargetFiles() {
    char *fileName1 = "target_location.txt";
    char *fileName2 = "target.txt";

    FILE *file1 = fopen(fileName1, "w");
    if (file1 != NULL) {
        fclose(file1);
    }
    FILE *file2 = fopen(fileName2, "w");
    if (file2 != NULL) {
        fprintf(file2, "Target file\n");
        fclose(file2);
    }
}

void createFiles() {
    createBackupDirectory();
    createCorruptedFilesDirectory();
    createGoodFilesDirectory();
    createHiddenFilesDirectory();
    createMainDirectory();
    createTargetFiles();
}

// ===== Core profile commands (similar to original Gryffindor) =====

void cmd_sanitize() {
    if (system("rm -rf ./corrupted_files") == 0) {
        printf("sanitize: removed temporary/corrupted files.\n");
    } else {
        printf("sanitize: no corrupted files found or removal failed.\n");
    }
}

void cmd_backup() {
    if (system("cp -r ./good_files/* ./backup_good_files 2>/dev/null") == 0) {
        printf("backup: copied good_files to backup_good_files.\n");
    } else {
        printf("backup: nothing to copy or backup failed.\n");
    }
}

void cmd_unhide() {
    DIR *dir = opendir("./hidden");
    if (dir && readdir(dir) != NULL) {
        if (system("mv ./hidden/* ./main/ 2>/dev/null") == 0) {
            printf("unhide: moved hidden files to main directory.\n");
        } else {
            printf("unhide: no hidden files moved.\n");
        }
    } else {
        printf("unhide: no files found to move.\n");
    }
    if (dir) closedir(dir);
}

void displayHelp_Core(const char *command) {
    printf("\n==== Core Profile Help ====\n");
    if (strcmp(command, "sanitize") == 0) {
        printf("sanitize: remove temporary/corrupted files.\n");
    } else if (strcmp(command, "backup") == 0) {
        printf("backup: copy ./good_files to ./backup_good_files.\n");
    } else if (strcmp(command, "unhide") == 0) {
        printf("unhide: move files from ./hidden to ./main.\n");
    } else if (strcmp(command, "all") == 0) {
        printf("Available commands:\n");
        printf("  sanitize   - remove corrupted files\n");
        printf("  backup     - backup good_files\n");
        printf("  unhide     - move hidden files to main\n");
        printf("  cd         - change directory\n");
        printf("  history    - show command history\n");
        printf("  exit       - exit shell\n");
    } else {
        printf("Unknown command '%s'. Type 'help all' for list.\n", command);
    }
    printf("===========================\n");
}

int coreShell(char **parsed) {
    if (parsed[0] == NULL) return 1;

    if (handleCommonBuiltins(parsed)) return 1;

    if (strcmp(parsed[0], "sanitize") == 0) {
        cmd_sanitize();
        return 1;
    } else if (strcmp(parsed[0], "backup") == 0) {
        cmd_backup();
        return 1;
    } else if (strcmp(parsed[0], "unhide") == 0) {
        cmd_unhide();
        return 1;
    } else if (strcmp(parsed[0], "help") == 0) {
        if (parsed[1] != NULL)
            displayHelp_Core(parsed[1]);
        else
            displayHelp_Core("all");
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("Exiting Core profile shell. Goodbye!\n");
        exit(0);
    } else {
        if (isLinuxCommand(parsed[0])) {
            return 0; // let caller exec
        } else {
            displayError();
            return 1;
        }
    }
}

// ===== Ops profile commands (similar to original Slytherin) =====

void cmd_truncate_important() {
    if (system("echo '' > ./important.txt") == 0) {
        printf("truncate_important: cleared important.txt.\n");
    } else {
        printf("truncate_important: could not modify important.txt.\n");
    }
}

void cmd_generate_corrupt() {
    createCorruptedFilesDirectory();
    if (system("touch ./corrupted_files/file{1..5}.txt") == 0) {
        printf("generate_corrupt: created sample corrupted files.\n");
    } else {
        printf("generate_corrupt: failed to create files.\n");
    }
}

void cmd_hide_main() {
    DIR *dir = opendir("./main");
    if (dir && readdir(dir) != NULL) {
        if (system("mv ./main/* ./hidden/ 2>/dev/null") == 0) {
            printf("hide_main: moved files from main to hidden.\n");
        } else {
            printf("hide_main: no files moved.\n");
        }
    } else {
        printf("hide_main: no files found.\n");
    }
    if (dir) closedir(dir);
}

void displayHelp_Ops(const char *command) {
    printf("\n==== Ops Profile Help ====\n");
    if (strcmp(command, "truncate_important") == 0) {
        printf("truncate_important: clear contents of important.txt.\n");
    } else if (strcmp(command, "generate_corrupt") == 0) {
        printf("generate_corrupt: create test corrupted files.\n");
    } else if (strcmp(command, "hide_main") == 0) {
        printf("hide_main: move files from main to hidden directory.\n");
    } else if (strcmp(command, "all") == 0) {
        printf("Available commands:\n");
        printf("  truncate_important - clear important.txt\n");
        printf("  generate_corrupt   - create corrupted_files\n");
        printf("  hide_main          - move main/* to hidden/\n");
        printf("  cd                 - change directory\n");
        printf("  history            - show command history\n");
        printf("  exit               - exit shell\n");
    } else {
        printf("Unknown command '%s'. Type 'help all' for list.\n", command);
    }
    printf("=========================\n");
}

int opsShell(char **parsed) {
    if (parsed[0] == NULL) return 1;

    if (handleCommonBuiltins(parsed)) return 1;

    if (strcmp(parsed[0], "truncate_important") == 0) {
        cmd_truncate_important();
        return 1;
    } else if (strcmp(parsed[0], "generate_corrupt") == 0) {
        cmd_generate_corrupt();
        return 1;
    } else if (strcmp(parsed[0], "hide_main") == 0) {
        cmd_hide_main();
        return 1;
    } else if (strcmp(parsed[0], "help") == 0) {
        if (parsed[1] != NULL)
            displayHelp_Ops(parsed[1]);
        else
            displayHelp_Ops("all");
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("Exiting Ops profile shell. Goodbye!\n");
        exit(0);
    } else {
        if (isLinuxCommand(parsed[0])) {
            return 0;
        } else {
            displayError();
            return 1;
        }
    }
}

// ===== Data profile commands (similar to original Hufflepuff) =====

void cmd_mkdata() {
    const char *dir_name = "./main";
    char file_name[128];
    FILE *file;

    srand(time(NULL));
    snprintf(file_name, sizeof(file_name), "%s/data_%d.txt", dir_name, rand());

    file = fopen(file_name, "w");
    if (file == NULL) {
        perror("mkdata");
        return;
    }
    fprintf(file, "Data file generated by Data profile.\n");
    fclose(file);
    printf("mkdata: created %s\n", file_name);
}

void cmd_motivate() {
    printf("motivate: Keep going. Small consistent progress beats perfection.\n");
}

void cmd_tips() {
    printf("tips: File management best practices:\n");
    printf("  - Keep directories organized by project/type.\n");
    printf("  - Use clear filenames and dates.\n");
    printf("  - Backup important data regularly.\n");
}

void displayHelp_Data(const char *command) {
    printf("\n==== Data Profile Help ====\n");
    if (strcmp(command, "mkdata") == 0) {
        printf("mkdata: create a new data text file in ./main.\n");
    } else if (strcmp(command, "motivate") == 0) {
        printf("motivate: print a motivational message.\n");
    } else if (strcmp(command, "tips") == 0) {
        printf("tips: show file management tips.\n");
    } else if (strcmp(command, "all") == 0) {
        printf("Available commands:\n");
        printf("  mkdata    - create sample data file\n");
        printf("  motivate  - print motivational message\n");
        printf("  tips      - show file tips\n");
        printf("  cd        - change directory\n");
        printf("  history   - show command history\n");
        printf("  exit      - exit shell\n");
    } else {
        printf("Unknown command '%s'. Type 'help all' for list.\n", command);
    }
    printf("==========================\n");
}

int dataShell(char **parsed) {
    if (parsed[0] == NULL) return 1;

    if (handleCommonBuiltins(parsed)) return 1;

    if (strcmp(parsed[0], "mkdata") == 0) {
        cmd_mkdata();
        return 1;
    } else if (strcmp(parsed[0], "motivate") == 0) {
        cmd_motivate();
        return 1;
    } else if (strcmp(parsed[0], "tips") == 0) {
        cmd_tips();
        return 1;
    } else if (strcmp(parsed[0], "help") == 0) {
        if (parsed[1] != NULL)
            displayHelp_Data(parsed[1]);
        else
            displayHelp_Data("all");
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("Exiting Data profile shell. Goodbye!\n");
        exit(0);
    } else {
        if (isLinuxCommand(parsed[0])) {
            return 0;
        } else {
            displayError();
            return 1;
        }
    }
}

// ===== Net profile commands (similar to original Ravenclaw) =====

void cmd_net_quote() {
    const char *wisdoms[] = {
        "Networks are built on small, reliable links.",
        "Debugging is like solving a mystery; logs are your clues.",
        "A good script today beats a perfect script tomorrow.",
        "Measure first, optimize later."
    };
    int numWisdoms = sizeof(wisdoms) / sizeof(wisdoms[0]);
    srand(time(0));
    printf("\nQuote: %s\n", wisdoms[rand() % numWisdoms]);
}

void cmd_net_quiz() {
    const char *riddles[] = {
        "I connect machines but have no moving parts. What am I?",
        "I identify a device in a network uniquely. What am I?"
    };
    const char *answers[] = {"network cable", "ip address"};
    int numRiddles = sizeof(riddles) / sizeof(riddles[0]);

    srand(time(0));
    int index = rand() % numRiddles;

    char userAnswer[100];
    printf("\nRiddle: %s\nYour Answer: ", riddles[index]);
    fflush(stdout);
    if (fgets(userAnswer, sizeof(userAnswer), stdin) == NULL) {
        printf("\nNo answer provided.\n");
        return;
    }
    userAnswer[strcspn(userAnswer, "\n")] = 0;

    for (char *p = userAnswer; *p; p++) *p = (char)tolower((unsigned char)*p);

    if (strcmp(userAnswer, answers[index]) == 0) {
        printf("\nCorrect!\n");
    } else {
        printf("\nIncorrect. The correct answer is: %s\n", answers[index]);
    }
}

void cmd_find_target() {
    printf("Scanning for target.txt...\n");
    if (system("find . -name 'target.txt' > target_location.txt") == 0) {
        printf("Search complete! Check target_location.txt for results.\n");
    } else {
        printf("No target file found.\n");
    }
}

void displayHelp_Net(const char *command) {
    printf("\n==== Net Profile Help ====\n");
    if (strcmp(command, "netquote") == 0) {
        printf("netquote: print a random technical quote.\n");
    } else if (strcmp(command, "netquiz") == 0) {
        printf("netquiz: answer a simple riddle.\n");
    } else if (strcmp(command, "find_target") == 0) {
        printf("find_target: search filesystem for 'target.txt'.\n");
    } else if (strcmp(command, "all") == 0) {
        printf("Available commands:\n");
        printf("  netquote    - show a technical quote\n");
        printf("  netquiz     - answer a riddle\n");
        printf("  find_target - search for target.txt\n");
        printf("  cd          - change directory\n");
        printf("  history     - show command history\n");
        printf("  exit        - exit shell\n");
    } else {
        printf("Unknown command '%s'. Type 'help all' for list.\n", command);
    }
    printf("=========================\n");
}

int netShell(char **parsed) {
    if (parsed[0] == NULL) return 1;

    if (handleCommonBuiltins(parsed)) return 1;

    if (strcmp(parsed[0], "netquote") == 0) {
        cmd_net_quote();
        return 1;
    } else if (strcmp(parsed[0], "netquiz") == 0) {
        cmd_net_quiz();
        return 1;
    } else if (strcmp(parsed[0], "find_target") == 0) {
        cmd_find_target();
        return 1;
    } else if (strcmp(parsed[0], "help") == 0) {
        if (parsed[1] != NULL)
            displayHelp_Net(parsed[1]);
        else
            displayHelp_Net("all");
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("Exiting Net profile shell. Goodbye!\n");
        exit(0);
    } else {
        if (isLinuxCommand(parsed[0])) {
            return 0;
        } else {
            displayError();
            return 1;
        }
    }
}

// ===== Sec profile commands (new 5th profile) =====

void cmd_scan_temp() {
    printf("scan_temp: listing main, hidden, corrupted_files (if present):\n");
    system("ls -R main hidden corrupted_files 2>/dev/null");
}

void cmd_secure_backup() {
    printf("secure_backup: creating archive backup_good_files.tar.gz (if backup_good_files exists)...\n");
    int status = system("tar -czf backup_good_files.tar.gz backup_good_files 2>/dev/null");
    if (status == 0) {
        printf("secure_backup: archive created.\n");
    } else {
        printf("secure_backup: archive creation failed.\n");
    }
}

void cmd_clean_temp() {
    printf("clean_temp: removing temporary *_temp.txt files in current directory.\n");
    int status = system("rm -f *_temp.txt 2>/dev/null");
    if (status == 0) {
        printf("clean_temp: cleanup attempted.\n");
    } else {
        printf("clean_temp: cleanup may have failed or no files.\n");
    }
}

void displayHelp_Sec(const char *command) {
    printf("\n==== Sec Profile Help ====\n");
    if (strcmp(command, "scan_temp") == 0) {
        printf("scan_temp: list files in main, hidden, corrupted_files.\n");
    } else if (strcmp(command, "secure_backup") == 0) {
        printf("secure_backup: create tar.gz archive of backup_good_files.\n");
    } else if (strcmp(command, "clean_temp") == 0) {
        printf("clean_temp: remove *_temp.txt files in current directory.\n");
    } else if (strcmp(command, "all") == 0) {
        printf("Available commands:\n");
        printf("  scan_temp      - list key directories\n");
        printf("  secure_backup  - archive backup_good_files\n");
        printf("  clean_temp     - remove temporary temp files\n");
        printf("  cd             - change directory\n");
        printf("  history        - show command history\n");
        printf("  exit           - exit shell\n");
    } else {
        printf("Unknown command '%s'. Type 'help all' for list.\n", command);
    }
    printf("=========================\n");
}

int secShell(char **parsed) {
    if (parsed[0] == NULL) return 1;

    if (handleCommonBuiltins(parsed)) return 1;

    if (strcmp(parsed[0], "scan_temp") == 0) {
        cmd_scan_temp();
        return 1;
    } else if (strcmp(parsed[0], "secure_backup") == 0) {
        cmd_secure_backup();
        return 1;
    } else if (strcmp(parsed[0], "clean_temp") == 0) {
        cmd_clean_temp();
        return 1;
    } else if (strcmp(parsed[0], "help") == 0) {
        if (parsed[1] != NULL)
            displayHelp_Sec(parsed[1]);
        else
            displayHelp_Sec("all");
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("Exiting Sec profile shell. Goodbye!\n");
        exit(0);
    } else {
        if (isLinuxCommand(parsed[0])) {
            return 0;
        } else {
            displayError();
            return 1;
        }
    }
}

// ===== Parsing helpers =====

// function for finding pipe
int parsePipe(char *str, char **strpiped) {
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }

    if (strpiped[1] == NULL)
        return 0; // no pipe
    else
        return 1;
}

// function for parsing command words
void parseSpace(char *str, char **parsed) {
    int i;
    for (i = 0; i < MAXLIST; i++) {
        parsed[i] = strsep(&str, " ");
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

// Main command processing: choose profile shell, decide if external command needed
int processString(char *str, char **parsed, char **parsedpipe, int profile) {
    char *strpiped[2];
    int piped = 0;

    piped = parsePipe(str, strpiped);

    if (piped) {
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);
    } else {
        parseSpace(str, parsed);
    }

    int handled = 0;

    if (profile == 0) {
        handled = coreShell(parsed);
    } else if (profile == 1) {
        handled = opsShell(parsed);
    } else if (profile == 2) {
        handled = dataShell(parsed);
    } else if (profile == 3) {
        handled = netShell(parsed);
    } else {
        handled = secShell(parsed);
    }

    if (handled) {
        return 0;
    } else {
        return 1 + piped; // 1 for simple, 2 for piped
    }
}

// Run a single command (with optional pipe), return status code
int runSingleCommand(char *commandStr, int profile) {
    char *parsedArgs[MAXLIST];
    char *parsedArgsPiped[MAXLIST];
    int execFlag = processString(commandStr, parsedArgs, parsedArgsPiped, profile);

    if (execFlag == 0) {
        // handled by built-in/profile
        return 0;
    } else if (execFlag == 1) {
        return execArgs(parsedArgs);
    } else if (execFlag == 2) {
        return execArgsPiped(parsedArgs, parsedArgsPiped);
    }
    return 1;
}

// Split on "&&": left and right are pointers inside input
int splitAnd(char *input, char **left, char **right) {
    char *pos = strstr(input, "&&");
    if (!pos) return 0;

    *pos = '\0';
    pos += 2;

    char *leftTrim = trimWhitespace(input);
    char *rightTrim = trimWhitespace(pos);

    if (leftTrim == NULL || rightTrim == NULL || strlen(leftTrim) == 0 || strlen(rightTrim) == 0)
        return 0;

    *left = leftTrim;
    *right = rightTrim;
    return 1;
}

// Mapping profile index to name and color
const char *profileName(int p) {
    switch (p) {
        case 0: return "Core";
        case 1: return "Ops";
        case 2: return "Data";
        case 3: return "Net";
        case 4: return "Sec";
        default: return "Unknown";
    }
}

const char *profileColor(int p) {
    switch (p) {
        case 0: return COLOR_RED;
        case 1: return COLOR_GREEN;
        case 2: return COLOR_YELLOW;
        case 3: return COLOR_BLUE;
        case 4: return COLOR_MAGENTA;
        default: return COLOR_RESET;
    }
}

// Shell loop
int shell_cmds(int profile) {
    char inputString[MAXCOM];
    printf("%sProfile selected: %s%s\n",
           profileColor(profile), profileName(profile), COLOR_RESET);

    // Short summary of commands per profile
    if (profile == 0) {
        printf("Commands: sanitize, backup, unhide, cd, history, help, exit\n");
    } else if (profile == 1) {
        printf("Commands: truncate_important, generate_corrupt, hide_main, cd, history, help, exit\n");
    } else if (profile == 2) {
        printf("Commands: mkdata, motivate, tips, cd, history, help, exit\n");
    } else if (profile == 3) {
        printf("Commands: netquote, netquiz, find_target, cd, history, help, exit\n");
    } else {
        printf("Commands: scan_temp, secure_backup, clean_temp, cd, history, help, exit\n");
    }

    while (1) {
        const char *color = profileColor(profile);
        const char *name = profileName(profile);

        printf("%s%s>%s ", color, name, COLOR_RESET);
        fflush(stdout);

        if (takeInput(inputString))
            continue;

        char *left = NULL;
        char *right = NULL;
        int hasAnd = splitAnd(inputString, &left, &right);

        if (hasAnd) {
            int status = runSingleCommand(left, profile);
            if (status == 0) {
                runSingleCommand(right, profile);
            } else {
                // first failed, skip second
            }
        } else {
            runSingleCommand(inputString, profile);
        }
    }
    return 0;
}

// Simple profile selection instead of sorting hat
const char *selectProfile() {
    int core = 0, ops = 0, data = 0, net = 0, sec = 0;
    char answer[16];

    printf("Profile selection wizard (answer yes/no):\n");

    printf("Q1: Do you like cleaning, organizing and maintaining systems? ");
    fflush(stdout);
    if (fgets(answer, sizeof(answer), stdin) && strncmp(answer, "yes", 3) == 0) core++;

    printf("Q2: Do you enjoy automation, deployment and operations? ");
    fflush(stdout);
    if (fgets(answer, sizeof(answer), stdin) && strncmp(answer, "yes", 3) == 0) ops++;

    printf("Q3: Do you like working with data and logs? ");
    fflush(stdout);
    if (fgets(answer, sizeof(answer), stdin) && strncmp(answer, "yes", 3) == 0) data++;

    printf("Q4: Are you interested in networking and connectivity? ");
    fflush(stdout);
    if (fgets(answer, sizeof(answer), stdin) && strncmp(answer, "yes", 3) == 0) net++;

    printf("Q5: Are you interested in security and monitoring? ");
    fflush(stdout);
    if (fgets(answer, sizeof(answer), stdin) && strncmp(answer, "yes", 3) == 0) sec++;

    int maxScore = core;
    const char *profile = "Core";
    if (ops > maxScore) { maxScore = ops; profile = "Ops"; }
    if (data > maxScore) { maxScore = data; profile = "Data"; }
    if (net > maxScore) { maxScore = net; profile = "Net"; }
    if (sec > maxScore) { maxScore = sec; profile = "Sec"; }

    printf("\nSelected profile: %s\n\n", profile);
    return profile;
}

int main() {
    init_shell();
    createFiles();

    const char *profile = selectProfile();

    if (strcmp(profile, "Core") == 0) shell_cmds(0);
    else if (strcmp(profile, "Ops") == 0) shell_cmds(1);
    else if (strcmp(profile, "Data") == 0) shell_cmds(2);
    else if (strcmp(profile, "Net") == 0) shell_cmds(3);
    else if (strcmp(profile, "Sec") == 0) shell_cmds(4);
    else shell_cmds(0);

    return 0;
}

