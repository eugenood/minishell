#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

int execCommand(const std::vector<std::string>& args, int prevReadFd, bool hasNext)
{
    int pipeFd[2];
    if (pipe(pipeFd) == -1)
    {
        perror("pipe failed");
        std::exit(EXIT_FAILURE);
    }

    if (fork() == 0)
    {
        if (hasNext)
            dup2(pipeFd[1], STDOUT_FILENO);
        dup2(prevReadFd, STDIN_FILENO);

        close(prevReadFd);
        close(pipeFd[0]);
        close(pipeFd[1]);

        std::vector<char*> argsPointers{};
        for (auto& arg : args)
            argsPointers.push_back(const_cast<char*>(arg.c_str()));
        argsPointers.push_back(nullptr);

        execvp(argsPointers[0], argsPointers.data());
        perror("execvp failed");
        std::_Exit(EXIT_FAILURE);
    }

    close(prevReadFd);
    close(pipeFd[1]);

    if (hasNext)
        return pipeFd[0];

    close(pipeFd[0]);
    return -1;
}

int main()
{
    std::string input{};

    while (true)
    {
        std::cout << "minishell> ";
        if ((!std::getline(std::cin, input)) || input == "exit")
            break;

        int prevReadFd{-1};
        int numCommands{0};

        std::stringstream stream{std::move(input)};
        std::string token{};
        std::vector<std::string> tokens{};
        while (stream >> token)
        {
            if (token == "|")
            {
                prevReadFd = execCommand(tokens, prevReadFd, true);
                tokens.clear();
                ++numCommands;
                continue;
            }

            tokens.push_back(std::move(token));
        }

        prevReadFd = execCommand(tokens, prevReadFd, false);
        tokens.clear();
        ++numCommands;

        for (int i{0}; i < numCommands; ++i)
            wait(nullptr);
    }

    return 0;
}