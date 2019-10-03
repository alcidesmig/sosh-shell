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
pid_t last_executed = -1;

int isInString(char *str, char value) // Função que verifica a presença de um caracter em uma string
{
    for(int i = 0; i < strlen(str); i++)
    {
        if(str[i] == value) return 1;
    }
    return 0;
}

void completionHook (char const *prefix, linenoiseCompletions *lc) // Auto-complete do linenoise
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

void attCwdFiles() // Atualizar arquivos do diretório
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

void attCwd() // Identificar diretório atual
{
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, "Erro ao identificar diretório corrente.\n");
        exit(1);
    }
}

void addJob(pid_t pid, char *result, int active, int stopped) // Adiciona um Job na lista de jobs
{
    Job job;
    job.pid = pid;
    job.cmd = result;
    job.active = active;
    job.stopped = stopped;
    if (jobs.size() == 0) job.id_job = 1;
    else job.id_job = jobs[jobs.size() - 1].id_job + 1;
    jobs.push_back(job);
}

void verifySetStopped(int status, pid_t pid) // Verifica se um job está parado e seta ele como parado caso positivo
{
    if(WIFSTOPPED(status))
    {
        int it = 0;
        while(it < jobs.size() && jobs[it].pid != pid) it++;
        if(it != jobs.size())
        {
            jobs[it].stopped = 1;
        }
    }
}

void executeProgram(char *cmd, char *result, char *argv, int background, int out, char *outFile, int in, char *inFile) // Executa um programa
{
    if( access( cmd, F_OK ) != -1 )
    {
        pid_t pid = fork();
        int status;
        if (pid != 0 && !background) // Se for o pai e não background
        {
            last_executed = pid;
            addJob(pid, result, 1, 0);
            waitpid(pid, &status, WUNTRACED); /* Wait for process in foreground */
            verifySetStopped(status, pid);
            last_executed = -1;
            return;
        }
        else if (pid != 0 && background) // Se for o pai e background
        {
            addJob(pid, result, 1, 0);
            return;
        }
        else if (pid < 0)
        {
            cerr << "Erro ao fazer o fork do programa" << endl;
        }
        else
        {
            if (in) // Redirecionamento de output >
            {
                int fd0 = open(inFile, O_RDONLY);
                if(fd0 == NULL)
                {
                    cerr << inFile << ": arquivo não encontrado" << endl;
                    return;
                }
                dup2(fd0, STDIN_FILENO);
                close(fd0);
            }

            if (out) // Redirecionamento de input <
            {
                int fd1 = creat(outFile, 0644) ;
                dup2(fd1, STDOUT_FILENO);
                close(fd1);
            }

            char *args[] = { cmd, argv, NULL }; // Argumentos para o programa
            int exec_status = execv(cmd, args); // Executa o programa
            cerr << cmd << ": erro ao executar o programa." << endl;

        }
    }
    else
    {
        cerr << cmd << ": programa não encontrado." << endl;
    }
}

void executeFile(char *cmd, char *argv, char *result, int background) // Executa um arquivo chamado com / ou ./
{
    int out = 0, in = 0;
    char *inFile, * outFile;
    if(isInString(result, '<')) // Verifica se é para redirecionar o input e identifica o arquivo para utilizar
    {
        in = 1;
        inFile = strtok(strdup(result), ">");
        inFile = strtok(NULL, "\0");
        while(inFile[0] == ' ') inFile = &inFile[1];
        argv = strtok(result, ">");
    }
    if(isInString(result, '>')) // Verifica se é para redirecionar o output e identifica o arquivo para criar e gravar
    {
        out = 1;
        outFile = strtok(strdup(result), ">");
        outFile = strtok(NULL, "\0");
        while(outFile[0] == ' ') outFile = &outFile[1];
        argv = strtok(result, ">");
    }

    executeProgram(cmd, result, argv, background, out, outFile, in, inFile);

}


void background_(char *bg) // Envia um processo para execução em background
{
    if(bg[0] == '%')
    {
       
        bg = &(bg[1]);
        int id_job;
        if(sscanf(bg, "%d", &id_job) > 0)
        {   
            int i = 0;

            while(jobs[i].id_job != id_job && i++ < jobs.size());

            if(i != jobs.size())
            {
                printf("Continuar\n");
                kill(id_job, 18); //SIGCONT;
                jobs[i].stopped = 0;
            }
            else
            {
                cerr << "Job não encontrado." << endl;
            }
        }
        else
        {
            cerr << "Utilize bg %id_job." << endl;
        }
    }
    else
    {
        cerr << "Utilize bg %id_job." << endl;
    }
}


void foreground(char *fg) // Envia um processo para execução em foreground
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

void changeDirectory(char *path) // cd
{
    if (!chdir(path))
    {
        cwdFiles.clear();
        attCwdFiles();
        attCwd();
    }
    else cout << "Diretório " << path << " não existe \n";
}

void listFiles() // ls
{
    for (std::size_t i = 0; i < cwdFiles.size(); ++i)
    {
        cout << cwdFiles[i] << "\t";
    }
    cout << endl;
}

void printCurrentDirectory() // pwd
{
    cout << cwd << endl;
}

void listJobs() // Listagem dos jobs
{
    if(jobs.size() == 0) cout << "Não há tarefas no histórico" << endl;
    for (std::size_t i = 0; i < jobs.size(); ++i)
    {
        int x = 0;
        pid_t status = waitpid(jobs[i].pid, &x, WNOHANG);
        if(status == jobs[i].pid || status == -1) {
            jobs[i].active = 0;
        }
        string stopped = jobs[i].stopped ? "Stopped" : "Running" ;
        if(jobs[i].active) cout << "Job " << jobs[i].id_job  << " [" << stopped << " at " << jobs[i].pid << "]" << ": " << jobs[i].cmd << endl;
    }
}


void handle(char *result) // Identificação e execução de comandos
{


    int background = result[strlen(result) - 2] == '&';

    if(background)
    {
        strtok(result, "\n");
        strtok(result, "&");
        if(result[strlen(result) - 1] == ' ') result[strlen(result) - 1] = '\0';
    }
    else
    {
        strtok(result, "\n");
    }

    char *cmd;
    if(isInString(result, ' '))
    {
        cmd = strtok(strdup(result), " ");
    }
    else
    {
        cmd = strtok(strdup(result), "\n");
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
        char *bg = strtok(NULL, "\0");
        background_(bg);
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
    else if ((cmd[0] == '.' && cmd[1] == '/') || cmd[0] == '/')
    {
        char *argv = strtok(NULL, "\0");
        executeFile(cmd, argv, result, background);
    }
    else
    {
        char *restOfString = strtok(NULL, "\0");
        int out = 0, in = 0;
        char *inFile, * outFile, * argv = restOfString;
        if(restOfString)
        {
            if(isInString(restOfString, '<')) // Verifica se é para redirecionar o input e identifica o arquivo para utilizar
            {
                in = 1;
                inFile = strtok(strdup(restOfString), ">");
                inFile = strtok(NULL, "\0");
                while(inFile[0] == ' ') inFile = &inFile[1];
                argv = strtok(restOfString, ">");
            }
            if(isInString(restOfString, '>')) // Verifica se é para redirecionar o output e identifica o arquivo para criar e gravar
            {
                out = 1;
                outFile = strtok(strdup(restOfString), ">");
                outFile = strtok(NULL, "\0");
                while(outFile[0] == ' ') outFile = &outFile[1];
                argv = strtok(restOfString, ">");
            }
        }
        char env_cmd[50] = "/bin/";
        strcat(env_cmd, cmd);
        executeProgram(env_cmd, result, argv, background, out, outFile, in, inFile);
    }
}



void sigHandler(int sig_num) // Handler de sinais
{
    signal(SIGINT, SIG_IGN);
    signal (SIGTSTP, sigHandler);
    signal(SIGCHLD, sigHandler);

    if(sig_num == SIGTSTP)
    {
        if(last_executed != -1)
        {
            printf("SIGSTP enviado para processo %d\n", last_executed);
            int sent = kill(last_executed, 19);
            last_executed = -1;
        }
        else
        {
            cerr << "Nenhum processo para parar." << endl;
        }
        fflush(stdout);
    }
    /*else if(sig_num == SIGCHLD)
    {
        pid_t pid;

        pid = wait(NULL);
        int it = 0;
        while(it < jobs.size() && jobs[it].pid != pid) it++;
        if(it != jobs.size())
        {
            jobs[it].active = 0;
        }
        fflush(stdout);
    }*/
    return;

}


void init_shell () // Função principal
{
    stop_shell = 0;
    cout << "Shell iniciado" << endl;

    signal(SIGINT, SIG_IGN);
    signal (SIGTSTP, sigHandler);


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

        //char *result = linenoise(prompt); // todo: implementar signals no linenoise

        cout << prompt;

        char *result = (char *) malloc(1024 * sizeof(char));
        size_t bufsize = 1024;
        getline(&result, &bufsize, stdin); // Leitura do comando


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
