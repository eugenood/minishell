#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void executeCommand(const std::vector<std::string>& args)
{
    std::vector<char*> argsPointers{};
    for (auto& arg : args)
        argsPointers.push_back(const_cast<char*>(arg.c_str()));
    argsPointers.push_back(nullptr);

    execvp(argsPointers[0], argsPointers.data());
    perror("execvp failed");
    std::_Exit(EXIT_FAILURE);
}

void handleStandardCommand(std::string&& command)
{
    if (fork() == 0)
    {
        std::vector<std::string> args{};
        std::stringstream ss{std::move(command)};
        std::string arg{};
        while (ss >> arg)
            args.push_back(arg);
        executeCommand(args);
    }

    wait(nullptr);
}

void handlePipedCommands(std::string&& leftCommand, std::string&& rightCommand)
{
    int pipeFd[2];
    if (pipe(pipeFd) == -1)
    {
        perror("pipe");
        return;
    }

    if (fork() == 0)
    {
        dup2(pipeFd[1], STDOUT_FILENO);
        close(pipeFd[0]);
        close(pipeFd[1]);

        std::vector<std::string> args{};
        std::stringstream ss{std::move(leftCommand)};
        std::string arg{};
        while (ss >> arg)
            args.push_back(arg);
        executeCommand(args);
    }

    if (fork() == 0)
    {
        dup2(pipeFd[0], STDIN_FILENO);
        close(pipeFd[0]);
        close(pipeFd[1]);

        std::vector<std::string> args{};
        std::stringstream ss{std::move(rightCommand)};
        std::string arg{};
        while (ss >> arg)
            args.push_back(arg);
        executeCommand(args);
    }

    close(pipeFd[0]);
    close(pipeFd[1]);
    wait(nullptr);
    wait(nullptr);
}

int main()
{
    std::string input{};

    while (true)
    {
        std::cout << "minishell> ";
        if ((!std::getline(std::cin, input)) || input == "exit")
            break;

        size_t pipePosition{input.find('|')};

        if (pipePosition == std::string::npos)
            handleStandardCommand(std::move(input));
        else
            handlePipedCommands(input.substr(0, pipePosition), input.substr(pipePosition + 1));
    }

    return 0;
}