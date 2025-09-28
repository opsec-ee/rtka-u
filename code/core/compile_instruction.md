
### Scalar mode
_$ gcc -O3 -march=native rtka_u.c -lm -o rtka_scalar_

### Parallel mode
_$ gcc -O3 -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_parallel_

### Thread safety check
_$ gcc -fsanitize=thread -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan_ \
_$ gcc -fsanitize=thread -mavx -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan_

### Execute
_$ ./rtka_scalar 2>&1 | cat_ \
_$ ./rtka_parallel 2>&1 | cat_ \
_$ ./rtka_tsan 2>&1 | cat_
