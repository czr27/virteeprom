
examine: examine.c ../eeprom.c ../rbtree.c flash_simulation.c testcases/gen_testcases.c
	gcc -DVEEPROM_DEBUG -Wall -std=c99 -g3 -I. -I.. -I./testcases/ ../eeprom.c ../rbtree.c ../errmsg.c flash_simulation.c examine.c testcases/gen_testcases.c -o examine


clean:
	rm ./examine
