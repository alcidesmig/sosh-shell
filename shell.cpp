#include "shell.h"

static const char *commands[] =
{
    "jobs",
    "fg",
    "bg",
    "cd",
    "ls",
    "pwd",
    "exit",
    NULL
};


int stop_shell;
char cwd[PATH_MAX];
vector<string> cwdFiles;
vector<Job> jobs;

int verifyDirectory(char *dir)
{
    struct stat aux;
    if (stat(dir, &aux) && S_ISDIR(aux.st_mode))
    {
        return 0;
    }
    return 1;
}

void completionHook (char const *prefix, linenoiseCompletions *lc)
{
    size_t i;

    for (i = 0;  commands[i] != NULL; ++i)
    {
        if (strncmp(prefix, commands[i], strlen(prefix)) == 0)
        {
            linenoiseAddCompletion(lc, commands[i]);
        }
    }

    for (std::size_t i = 0; i < cwdFiles.size(); ++i)
    {
        const char *c_str = cwdFiles[i].c_str();
        if (strncmp(prefix, c_str, strlen(prefix)) == 0)
        {
            linenoiseAddCompletion(lc, c_str);
        }
    }
}

void attCwdFiles()
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            cwdFiles.push_back(dir->d_name);
        }
        closedir(d);
    }
}

void attCwd()
{
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, "Erro ao identificar diretório corrente.\n");
        exit(1);
    }
}

void executeProgram(char *cmd, char *result, char *argv, int background)
{
    pid_t pid = fork();
    int status;
    if (pid != 0 && !background)
    {
        printf("Esperando\n");
        waitpid(pid, &status, 0); /* Wait for process in foreground */
        printf("Finalizado\n");
    }
    else if (pid != 0 && background)
    {
        Job job; /* Process in background */
        job.pid = pid;
        job.cmd = result;
        job.active = 1;
        if (jobs.size() == 0) job.id_job = 1;
        else job.id_job = jobs[jobs.size() - 1].id_job + 1;
        jobs.push_back(job);
        return;
    }
    else if (pid < 0)
    {
        cerr << "Erro ao executar o programa" << endl;
    }
    else
    {
        char env_cmd[6] = "/bin/";

        strcat(env_cmd, cmd);

        int exec_status = execv(env_cmd, &argv);

        if(exec_status)
        {
            cerr << cmd << ": comando não encontrado" << endl;
        }
    }
}

void executeFile(char *cmd, char *argv)
{
    int pid = fork();
    int status;

    if (pid)
    {
        waitpid(pid, &status, WUNTRACED);
    }
    else if (pid < 0)
    {
        cerr << "Erro ao executar o programa" << endl;
    }
    else
    {
        execv(cmd, &argv);
    }
}

void foreground(char *fg)
{
    if(fg[0] == '%')
    {
        fg = &(fg[1]);
        int id_job;
        if(sscanf(fg, "%d", &id_job) > 0)
        {
            int i = 0;

            while(jobs[i].id_job != id_job || i++ < jobs.size());

            if(i != jobs.size())
            {
                int status;
                waitpid(jobs[i].pid, &status, 0);
            }
            else
            {
                cerr << "Job não encontrado." << endl;
            }
        }
        else
        {
            cerr << "Utilize fg %id_job." << endl;
        }
    }
    else
    {
        cerr << "Utilize fg %id_job." << endl;
    }
}

void changeDirectory(char *path)
{
    if (!chdir(path))
    {
        cwdFiles.clear();
        attCwdFiles();
        attCwd();
    }
    else cout << "Diretório " << path << " não existe \n";
}

void listFiles()
{
    for (std::size_t i = 0; i < cwdFiles.size(); ++i)
    {
        cout << cwdFiles[i] << endl;
    }
}

void printCurrentDirectory()
{
    cout << cwd << endl;
}

void listJobs()
{
    if(jobs.size() == 0) cout << "Não há tarefas no histórico" << endl;
    for (std::size_t i = 0; i < jobs.size(); ++i)
    {
        kill(jobs[i].pid, 0);
        if(errno = ESRCH)
        {
            jobs[i].active = 0;
        }
        if(jobs[i].active) cout << "Job " << jobs[i].id_job << " [" << jobs[i].pid << "]" << ": " << jobs[i].cmd << endl;
    }
}

void handle(char *result)
{
    char *cmd = strtok(strdup(result), " ");

    int background = result[strlen(result) - 1] == '&';

    if(background)
    {
        result[strlen(result) - 1] = '\0';
    }

    if (!strcmp(cmd, "jobs"))
    {
        listJobs();
    }

    else if (!strcmp(cmd, "fg"))
    {
        char *fg = strtok(NULL, "\0");
        foreground(fg);
    }
    else if (!strcmp(cmd, "bg"))
    {

    }
    else if (!strcmp(cmd, "cd"))
    {
        char *path = strtok(NULL, "\0");
        changeDirectory(path);
    }
    else if (!strcmp(cmd, "ls"))
    {
        listFiles();
    }
    else if (!strcmp(cmd, "pwd"))
    {
        printCurrentDirectory();
    }
    else if (!strcmp(cmd, "exit"))
    {
        exit(0);
    }
    else if (cmd[0] == '.' && cmd[1] == '/')
    {
        char *argv = strtok(NULL, "\0");
        executeFile(cmd, argv);
    }
    else
    {
        char *argv = strtok(NULL, "\0");
        executeProgram(cmd, result, argv, background);

    }
}



void sigHandler(int sig)
{
    printf("%d\n", sig);
}
/* Make sure the shell is running interactively as the foreground job
   before proceeding. */


pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
void
init_shell ()
{
    stop_shell = 0;
    cout << "Shell iniciado" << endl;

    /* See if we are running interactively.  */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);

    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground.  */
        while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
            kill (- shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        signal (SIGINT, sigHandler);
        signal (SIGQUIT, sigHandler);
        signal (SIGTSTP, sigHandler);
        signal (SIGTTIN, sigHandler);
        signal (SIGTTOU, sigHandler);
        signal (SIGCHLD, sigHandler);

        /* Put ourselves in our own process group.  */
        shell_pgid = getpid ();
        if (setpgid (shell_pgid, shell_pgid) < 0)
        {
            perror ("Couldn't put the shell in its own process group");
            exit (1);
        }

        /* Grab control of the terminal.  */
        tcsetpgrp (shell_terminal, shell_pgid);

        /* Save default terminal attributes for shell.  */
        tcgetattr (shell_terminal, &shell_tmodes);
    }




    /* Linenoise functions */

    char prompt[2 * PATH_MAX];

    attCwd();
    attCwdFiles();

    linenoiseInstallWindowChangeHandler();
    linenoiseHistoryLoad(HISTORY);
    linenoiseSetCompletionCallback(completionHook);

    while (!stop_shell)
    {
        memset(prompt, '\0', 2 * PATH_MAX);
        strcat(prompt, "\x1b[1;32mshell\x1b[0m:\x1b[1;34m");
        strcat(prompt, cwd);
        strcat(prompt, "\x1b[0m$ ");

        char *result = linenoise(prompt);

        if (result == NULL)
        {
            break;
        }
        else
        {
            handle(result);
        }

        if (*result == '\0')
        {
            free(result);
            break;
        }

        linenoiseHistoryAdd(result);
        free(result);
    }

    linenoiseHistorySave(HISTORY);
    linenoiseHistoryFree();
    return;
}
