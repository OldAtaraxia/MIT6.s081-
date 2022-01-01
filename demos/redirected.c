int main() {
    int pid;

    pid = fork();
    if(pid == 0) {
        // 把文件描述符1变成output.txt
        close(1);
        open("output.txt", O_WRONLY | O_CREATE); // open会返回未使用的最小文件描述符号

        char* argv[] = { "echo", "this", "is", "echo", 0}; // 0标记了数组的结尾...
   		exec("echo", argv); // echo根本不知道发生了什么
    	printf("exec failed!\n");
    	exit(1);
    } else {
        wait((int *) 0); 
    }
    exit(0);
}