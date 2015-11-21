
DIR = ../..

main: main.c ${DIR}/eeprom.c ${DIR}/rbtree.c flash_simulation.c testcases/gen_testcases.c
	gcc -DVEEPROM_DEBUG -Wall -std=c99 -g3 -I. -I${DIR} -I./testcases/ ${DIR}/eeprom.c ${DIR}/rbtree.c ${DIR}/errmsg.c flash_simulation.c main.c testcases/gen_testcases.c -o main


clean:
	rm ./main
