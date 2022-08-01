
ar rvs liboauthcpp.a liboauth/*.o
gcc -c -DMG_ENABLE_OPENSSL -lcrypto -lssl -I/usr/local/occlum/openssl/include -L/usr/local/occlum/openssl/lib  mongoose.c
g++ -g -ggdb -W -Wall -Werror  -std=c++11 -lssl -lcrypto -I/usr/local/mongoose/openssl/include -L/usr/local/mongoose/openssl/lib -DMG_ENABLE_OPENSSL -loauthcpp -Iliboauth -L. mongoose.o http_server.cpp main.cpp -o main 
