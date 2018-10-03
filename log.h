#ifndef LOG_H
#define LOG_H


#ifdef DEBUG_LOG
    #define LOG_FILE "log.txt"
    #define log_msg(msg) _log_msg((msg))
    #define log_msgf(format, __VA_ARGS__)   \
            do {                                \
                _log_msg("");                   \
                FILE *f = fopen(LOG_FILE, "a"); \
                if (f){                         \
                    fprintf(f,(format), __VA_ARGS__);  \
                    fclose(f);                  \
                }                               \
            }while(0)
    
        void _log_msg(char *msg);
#else
    #define log_msg(msg)
    #define log_msgf(format, __VA_ARGS__) 
#endif

    void panic_exit(char *msg);

#endif
