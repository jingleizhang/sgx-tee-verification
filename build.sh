gcc -c common/slre/slre.c
gcc -c common/mongoose/mongoose.c
g++ -g -W -Wall -Werror -std=c++11 mongoose.o main.cpp http_server.cpp  -o http_server

