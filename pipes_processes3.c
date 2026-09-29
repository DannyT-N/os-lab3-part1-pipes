#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/**
 * Executes the command "cat scores | grep <arg> | sort".  In this quick-and-dirty
 * implementation the parent doesn't wait for the children to finish and
 * so the command prompt may reappear before they terminate.
 *
 */

int main(int argc, char **argv)
{
  // check that a grep argument was given
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <grep argument>\n", argv[0]);
    return 1;
  }

  int pipefd[2];   // cat -> grep
  int pipefd2[2];  // grep -> sort
  int pid;

  char *cat_args[] = {"cat", "scores", NULL};
  char *grep_args[] = {"grep", argv[1], NULL};
  char *sort_args[] = {"sort", NULL};

  // make the pipes
  if (pipe(pipefd) == -1 || pipe(pipefd2) == -1)
  {
    fprintf(stderr, "Pipe Failed\n");
    return 1;
  }

  pid = fork();
  if (pid < 0)
  {
    fprintf(stderr, "Fork Failed\n");
    return 1;
  }

  if (pid == 0)
  {
    // child: handles grep, and forks a grandchild for sort
    int pid2 = fork();
    if (pid2 < 0)
    {
      fprintf(stderr, "Fork Failed\n");
      return 1;
    }

    if (pid2 == 0)
    {
      // grandchild: handles "sort"
      dup2(pipefd2[0], 0);
      close(pipefd[0]);
      close(pipefd[1]);
      close(pipefd2[0]);
      close(pipefd2[1]);
      execvp("sort", sort_args);
      perror("sort failed");
    }
    else
    {
      // child: handles "grep <arg>"
      dup2(pipefd[0], 0);
      dup2(pipefd2[1], 1);
      close(pipefd[0]);
      close(pipefd[1]);
      close(pipefd2[0]);
      close(pipefd2[1]);
      execvp("grep", grep_args);
      perror("grep failed");
    }
  }
  else
  {
    // parent: handles "cat scores"
    dup2(pipefd[1], 1);
    close(pipefd[0]);
    close(pipefd[1]);
    close(pipefd2[0]);
    close(pipefd2[1]);
    execvp("cat", cat_args);
    perror("cat failed");
  }

  return 0;
}