
gcc -c common/mongoose/mongoose.c
g++ -g -W -Wall -Werror -std=c++11 http_server.cpp
g++ -g -W -Wall -Werror -std=c++11 mongoose.o http_server.o main.cpp -o main

