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

void handle(char *result)
{
    char *cmd = strtok(strdup(result), " ");

    int background = result[strlen(result) - 1] == '&';

    if(background) {
        result[strlen(result) - 1] = '\0';
    }

    if (!strcmp(cmd, "jobs"))
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

    else if (!strcmp(cmd, "fg"))
    {
        char *fg = strtok(NULL, "\0");
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
        else if (pid < 0)
        {
            cerr << "Erro ao executar o programa" << endl;
        }
        else
        {
            char *argv = strtok(NULL, "\0");
            execv(cmd, &argv);
        }
    }
    else
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
            char *argv = strtok(NULL, "\0");

            char env_cmd[6] = "/bin/";

            strcat(env_cmd, cmd);

            int exec_status = execv(env_cmd, &argv);

            if(exec_status)
            {
                cerr << cmd << ": comando não encontrado" << endl;
            }
            stop_shell = 1;
        }
    }
}


/* Make sure the shell is running interactively as the foreground job
   before proceeding. */

void
init_shell ()
{
    stop_shell = 0;
    cout << "Shell iniciado" << endl;

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
