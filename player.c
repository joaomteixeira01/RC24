#include "client.h"

int main (int argc, char** argv){
    char *gs_ip = NULL;
    int gs_port = 58053;

    int opt;
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                gs_ip = optarg;
                break;
            
            case 'p':
                gs_port = atoi(optarg);
                break;
            
            default:
                fprintf(stderr, "Usage: %s [-n GSIP] [-n GSport]\n", argv[0]);
                exit(1);
        }
    }
    return 0;
}