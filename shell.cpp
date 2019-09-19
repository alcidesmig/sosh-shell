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


char cwd[PATH_MAX];
vector<string> cwdFiles;

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

void handle(char *result)
{
    char *cmd = strtok(strdup(result), " ");

    if (!strcmp(cmd, "jobs"))
    {

    }
    else if (!strcmp(cmd, "fg"))
    {

    }
    else if (!strcmp(cmd, "bg"))
    {

    }
    else if (!strcmp(cmd, "cd"))
    {
        char *path = strtok(NULL, "\0");
        if (!chdir(path))
        {
            cwdFiles.clear();
            attCwdFiles();
            attCwd();
        }
        else cout << "Diretório " << path << " não existe \n";
    }
    else if (!strcmp(cmd, "ls"))
    {
        for (std::size_t i = 0; i < cwdFiles.size(); ++i)
        {
            cout << cwdFiles[i] << endl;
        }
    }
    else if (!strcmp(cmd, "pwd"))
    {
        cout << cwd << endl;
    }
    else if (!strcmp(cmd, "exit"))
    {
        exit(0);
    }
    else if (cmd[0] == '.' && cmd[1] == '/')
    {
        int pid = fork();
        int status;

        if (pid)
        {
            waitpid(pid, &status, WUNTRACED);
        }
        else
        {
            char *argv[] = {NULL};
            char *envp[] = {NULL};

            execve(cmd, argv, envp);
        }
    }
    else
    {
        cerr << cmd << ": comando não encontrado" << endl;
    }
}


pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

/* Make sure the shell is running interactively as the foreground job
   before proceeding. */

void
init_shell ()
{

    /* See if we are running interactively.  */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);

    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground.  */
        while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
            kill (- shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

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

        /* Linenoise functions */

        char prompt[2 * PATH_MAX];

        attCwd();
        attCwdFiles();

        linenoiseInstallWindowChangeHandler();
        linenoiseHistoryLoad(HISTORY);
        linenoiseSetCompletionCallback(completionHook);

        while (1)
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
    }
}
