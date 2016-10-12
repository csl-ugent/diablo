extern int putc(unsigned int);
extern int exit(int);

main(){
    char msg[]="Hello World!\n";
    char *ptr;
    int res;

    ptr = msg;
    while (*ptr)
      res = putc(*ptr++);
    exit(0);
}
