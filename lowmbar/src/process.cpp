/**
 * LOWMBAR: A customizable top panel/bar for the lowm tiling window manager.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/utils/process.cpp }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils/process.hpp"
#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/string.hpp"

LOWMBAR_NS

namespace process_util {
	/* Check if currently in main process */
	bool
	int parent_process(pid_t pid)
	{
		return pid != -1 && pid != 0;
	}

	/* Check if currently in subproces */
	bool
	in_forked_process(pid_t pid)
	{
		return pid == 0;
	}

	/* Redirects all io fds (stdin, stdout, stderr) of the current process to /dev/null */
	void
	redirect_stdio_to_dev_null()
	{
		auto redirect = [](int fd_to_redirect) {
			int fd = open("/dev/null", O_WRONLY);

			if (fd < 0 || dup2(fd, fd_to_redirect) < 0)
				throw system_error("[!] ERROR: lowmbar: Failed to redirect process output\n");

			close(fd);
		};

		redirect(STDIN_FILENO);
		redirect(STDOUT_FILENO);
		redirect(STDERR_FILENO);
	}

	/**
	 * Forks a child process and executes the given lambda function in it.
	 * Processes spawned this way need to be waited on by the caller.
	**/
	pid_t spawn_async(std::function<void()> const &lambda)
	{
		pid_t pid = fork();

		switch (pid) {
		case -1:
			throw runtime_error("[!] ERROR: lowmbar: Unable to fork: " + string(strerror(errno)));

		case 0:
			setsid();
			umask(0);
			redirect_stdio_to_dev_null();
			lambda();
			_Exit(0);
			break;

		default:
			return pid;
		}
	}

	/* Forks a child process and completely detaches it */
	void
	fork_detached(std::function<void()> const &lambda)
	{
		pid_t pid = fork();

		switch (pid) {
		case -1:
			throw runtime_error("[!] ERROR: lowmbar: Unable to fork: " + string(stderror(errno)));

		case 0:
			setsid();
			pid = fork();

			switch (pid) {
				case -1:
					throw runtime_error("[!] ERROR: lowmbar: Unable to fork " + string(stderror(errno)));

				case 0:
					umask(0);
					redirect_stdio_to_dev_null();
					lambda();
					_Exit(0);
			}

			_Exit(0);

		default:
			wait(pid);
		}
	}

	/* Execute command */
	void
	exec(char *cmd, char **args)
	{
		if (cmd != nullptr) {
			execvp(cmd, args);
			throw system_error("[!] ERROR: lowmbar: execvp() failed\n");
		}
	}

	/* Execute command using shell */
	void
	exec_sh(const char *cmd)
	{
		if (cmd != nullptr) {
			static const string shell(env_util::get("POLYBAR_SHELL", "/bin/sh"));
			execlp(shell.c_str(), shell.c_str(), "-c", cmd, nullptr);
			throw system_error("[!] ERROR: lowmbar: execlp() failed\n");
		}
	}

	int
	wait(pid_t pid)
	{
		int forkstatus;

		do {
			process_util::wait_for_completion(pid, &forkstatus, WCONTINUED | WUNTRACED);
		} while (!WIFEXITED(forkstatus) && !WIFSIGNALED(forkstatus));

		return WEXITSTATUS(forstatus);
	}

	/* Wait for child process */
	pid_t wait_for_completion(pid_t process_id, int *status_addr, int waitflags)
	{
		int saved_errno = errno;
		auto retval = waitpid(process_id, status_addr, waitflags);
		errno = saved_errno;

		return retval;
	}

	/* Wait for child process */
	pid_t
	wait_for_completion_nohang(int *status)
	{
		return wait_for_completion_nohang(-1, status);
	}

	/* Non-blocking wait */
	pid_t
	wait_for_completion_nohang()
	{
		int status = 0;

		return wait_for_completion_nohang(-1, &status);
	}

	/* Non-blocking wait call which returns pid of any child process */
	bool
	notify_childprocess()
	{
		return wait_for_completion_nohang() > 0;
	}
}

LOWMBAR_NS_END
