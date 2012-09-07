if [ -d ../../data/phoneless ]
then rm -r ../../data/phoneless
fi
gcc -c *.c -I. -I.. -I../porting_layer/include -I../../include
gcc sort_word.o phoneless_util.o ../common/*.o ../porting_layer/src/*.o -o a.out
gcc sort_dic.o ../common/*.o ../porting_layer/src/*.o -lm -o b.out
gcc maketree.o -o c.out

