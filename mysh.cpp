#include "parser.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <map>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// @author Panagiotis Kazakos sdi1900067
// My custom shell

static bool ignore_sig = true;
static pid_t child_pid = -1;

void signal_handler(int signal) {
  if (signal == SIGCHLD) {
    // parent receives signal that a child finished
    int status;
    waitpid(-1, &status, WNOHANG);
  } else if (ignore_sig) {
    std::cout << "\nin-mysh-now:> ";
    fflush(stdout);
  } else {
    kill(child_pid, signal);
    std::cout << std::endl;
  }
}

int main() {

  std::list<std::string> history;
  std::map<std::string, std::vector<std::string>> aliases;

  // register signal handler with sigaction
  struct sigaction sa;
  sa.sa_handler = &signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) == -1 ||
      sigaction(SIGTSTP, &sa, NULL) == -1 ||
      sigaction(SIGCHLD, &sa, NULL) == -1) {
    std::cerr << "error: signal handler register" << std::endl;
    return 1;
  }

  while (true) {
    // Per line logic
    // Prompt
    std::cout << "in-mysh-now:> ";
    fflush(stdout);

    // Input
    std::string input;
    std::getline(std::cin, input);

    // Parse
    Parser parser(input);

    const std::string &status = parser.getStatus();
    if (status == "OVERMAX") {
      std::cout << "Commands must be less than " << MAX_COMMANDS << std::endl;
      continue;
    }

    Command **const &tokens = parser.getTokens();

    const int &num_tokens = parser.getNumTokens();
    const int &num_commands = parser.getNumCommands();
    const int &num_pipes = parser.getNumPipes();

    // new line handle
    if (num_tokens == 0) {
      continue;
    }

    // add line to history
    if (history.size() == 20) {
      history.pop_front();
    }
    history.push_back(input);

    // handle keywords for all commands
    for (int i = 0; i < num_commands; i++) {
      Command *command = tokens[i]; // copy by reference
      if (command->empty)
        continue;
      // history handle
      parser.history(history, i);
      // createalias and destroyalias
      parser.alias(aliases, i);
      // handle cd keyword
      parser.cd(i);
    }

    int pipe_fd[num_pipes][2];
    for (int i = 0; i < num_pipes; i++) {
      if (pipe(pipe_fd[i]) == -1) {
        std::cerr << "Could not create pipe" << std::endl;
      }
    }

    int pipe_counter = 0;
    int num_pipepids = 0;

    // Execute commands
    for (int i = 0; i < num_commands; i++) {
      Command *command = tokens[i]; // copy by reference
      // exit handle
      if (command->exec == "exit") {
        return 0;
      }
      if (command->empty)
        continue;

      pid_t pid = fork();
      ignore_sig = false;
      child_pid = pid;

      if (pid < 0) {
        std::cerr << "fork failed: " << std::endl;
        return 1;
      } else if (pid == 0) {
        // child

        // command
        const char *exec_name = command->exec.c_str();
        // convert all arguments from std::string to char *
        std::vector<char *> argv;
        for (auto &str : *command->args) {
          argv.push_back(&str[0]);
        }
        argv.push_back(nullptr);

        // i/o redirection
        int fdInput = open(command->fileIn.c_str(), O_RDONLY);
        int fdOutput =
            open(command->fileOut.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
        int fdApnd = open(command->fileApnd.c_str(),
                          O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (!command->fileIn.empty()) {
          dup2(fdInput, STDIN_FILENO);
          close(fdInput);
        }
        if (!command->fileOut.empty()) {
          dup2(fdOutput, STDOUT_FILENO);
          close(fdOutput);
        }
        if (!command->fileApnd.empty()) {
          dup2(fdApnd, STDOUT_FILENO);
          close(fdApnd);
        }
        if (command->pipeIn) {
          dup2(pipe_fd[pipe_counter][0], STDIN_FILENO);
          close(pipe_fd[pipe_counter][0]);
          close(pipe_fd[pipe_counter][1]);
          pipe_counter++;
        }
        if (command->pipeOut) {
          dup2(pipe_fd[pipe_counter][1], STDOUT_FILENO);
          close(pipe_fd[pipe_counter][0]);
          close(pipe_fd[pipe_counter][1]);
        }
        execvp(exec_name, argv.data());
        std::cerr << exec_name << " is not a command" << std::endl;
        return 1;
      }
      // parent

      if (command->pipeIn || command->pipeOut) {
        num_pipepids++; // increment number of proceses of pipeline
        if (command->pipeIn) {
          close(pipe_fd[pipe_counter][0]);
          close(pipe_fd[pipe_counter][1]);
          pipe_counter++;
        }
      }
      if (!command->background && !command->pipeOut) {
        if (command->pipeIn) {
          // gather all commands of current pipeline
          for (int i = 0; i < num_pipepids; i++) {
            int status;
            wait(&status);
          }
        } else {
          // commands that do not belong to a pipeline
          int status;
          waitpid(child_pid, &status, WUNTRACED);
        }
      }

      ignore_sig = true;
    }
  }

  return 0;
}