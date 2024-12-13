# #!/bin/bash
# for ((i=1;i<=5;i+=1))
# do
# mpirun -n 5 -f nodes /home/thb/distribute_extension/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 0
# cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/FULL-twitter-hash-${i}.txt

# # mpirun -n 5 -f nodes /home/thb/distribute_extension_mem/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 0
# # cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/SUB-twitter-hash-${i}.txt

# mpirun -n 5 -f nodes /home/thb/distribute_extension/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 1
# cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/FULL-twitter-range-${i}.txt

# # mpirun -n 5 -f nodes /home/thb/distribute_extension_mem/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 1
# # cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/SUB-twitter-range-${i}.txt

# mpirun -n 5 -f nodes /home/thb/distribute_extension/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 2
# cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/FULL-twitter-random-${i}.txt

# # mpirun -n 5 -f nodes /home/thb/distribute_extension_mem/main -g "/home/thb/local_graph/twitter.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 2
# # cp /home/thb/distribute_extension_mem/LOG.txt data/dismem/SUB-twitter-random-${i}.txt
# done


# make && mpirun -n 5 -f nodes ./main -g "/home/thb/local_graph/web-google.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 0 -o 100

make && mpirun -n 5 -f nodes ./main -g "/home/thb/local_graph/gsh-2015.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 0 -o 10000
cp LOG.txt gsh-2015.txt
make && mpirun -n 5 -f nodes ./main -g "/home/thb/local_graph/clueweb12.bin" -n "network/Amazon5.txt" -t 10 -l 1 -r 0.4 -s -w 0 -o 10000
cp LOG.txt clueweb12.txt