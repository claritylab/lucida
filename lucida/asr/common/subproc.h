/*Original Heading of the file(spawn.cpp)*/
// 
// Example of communication with a subprocess via stdin/stdout
// Author: Konstantin Tretyakov
// License: MIT
//

//Found on github: https://gist.github.com/konstantint/d49ab683b978b3d74172

//New Heading
/* 
* Example of communication with a subprocess via stdin/stdout 
* using a class.
*
*
*	Will work with Apache Thrift if you need to execute a binary (via exec() 
* family functions) on top of a running server(SEE NOTE)	 
*
* NOTE: do not compile with "using namepace std" in your 
*				program otherwise there will be some  
*				errors in the compiling process if combined with 
*				the BOOST Library. Use "--std=c++11"  
*
* 
* Original Author: Konstantin Tretyakov
*
* Modified: Moeiz Riaz (06/11/2015)
* 
*/
#ifndef SUBPROC_H_
#define SUBPROC_H_


#include <ext/stdio_filebuf.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <exception>

// Wrapping pipe in a class makes sure they are closed when we leave scope
class cpipe {
private:
    int fd[2];
public:
    const inline int read_fd() const { return fd[0]; }
    const inline int write_fd() const { return fd[1]; }
    cpipe() { if (pipe(fd)) throw std::runtime_error("Failed to create pipe"); }
    void close() { ::close(fd[0]); ::close(fd[1]); }
    ~cpipe() { close(); }
};


class cmd_subproc {
private:
		//Variables
    cpipe write_pipe;
    cpipe read_pipe;
public:
		//Variables 
    int child_pid;
    std::unique_ptr<__gnu_cxx::stdio_filebuf<char> > write_buf = NULL; 
    std::unique_ptr<__gnu_cxx::stdio_filebuf<char> > read_buf = NULL;
    std::ostream stdin;
    std::istream stdout;
	
		//Constructors	
    cmd_subproc();

		//Methods Declarations
		void cmd_exe(const char* const argv[], bool with_path, const char* const envp[] = 0);
    void send_eof();
    int wait(); 
    
};
	//Constructors
	cmd_subproc::cmd_subproc(): stdin(NULL),stdout(NULL), child_pid(-1){}

	//Methods
 	void cmd_subproc::cmd_exe(const char* const argv[], bool with_path, const char* const envp[]){
        child_pid = fork();
        if (child_pid == -1) throw std::runtime_error("Failed to start child process"); 
        if (child_pid == 0) {   // In child process
            dup2(write_pipe.read_fd(), STDIN_FILENO);
            dup2(read_pipe.write_fd(), STDOUT_FILENO);
            write_pipe.close(); read_pipe.close();
            int result;
            if (with_path) {
                if (envp != 0) result = execvpe(argv[0], const_cast<char* const*>(argv), const_cast<char* const*>(envp));
                else result = execvp(argv[0], const_cast<char* const*>(argv));
            }
            else {
                if (envp != 0) result = execve(argv[0], const_cast<char* const*>(argv), const_cast<char* const*>(envp));
                else result = execv(argv[0], const_cast<char* const*>(argv));
            }
            if (result == -1) {
               // Note: no point writing to stdout here, it has been redirected
               std::cerr << "Error: Failed to launch program" << std::endl;
               exit(1);
            }
        }
        else {
            close(write_pipe.read_fd());
            close(read_pipe.write_fd());
            write_buf = std::unique_ptr<__gnu_cxx::stdio_filebuf<char> >(new __gnu_cxx::stdio_filebuf<char>(write_pipe.write_fd(), std::ios::out));
            read_buf = std::unique_ptr<__gnu_cxx::stdio_filebuf<char> >(new __gnu_cxx::stdio_filebuf<char>(read_pipe.read_fd(), std::ios::in));
            stdin.rdbuf(write_buf.get());
            stdout.rdbuf(read_buf.get());
        }
    }

	void cmd_subproc::send_eof() { 
		write_buf->close(); 
	}
    
	int cmd_subproc::wait() {
    int status;
    waitpid(child_pid, &status, 0);
		return status;
	}

#endif
