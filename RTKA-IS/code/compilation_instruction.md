Scalar mode

$ gcc -O3 -march=native rtka_u.c -lm -o rtka_scalar

Parallel mode

$ gcc -O3 -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_parallel

Thread safety check

$ gcc -fsanitize=thread -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan \
$ gcc -fsanitize=thread -mavx -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan

Execute

$ ./rtka_scalar 2>&1 | cat \
$ ./rtka_parallel 2>&1 | cat \
$ ./rtka_tsan 2>&1 | cat
